#pragma once
#include <iostream>
#include <queue>
#include <vector>
#include <exception>
#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>


#define THREADPOOL_AUTO_GROW    //设置线程池是否自动增长

class ThreadPool
{
private:
    using Task = std::function<void()>;
    std::vector<std::thread> _pool;     /*线程池*/
    std::queue<Task> _tasks;        /*任务队列*/
    std::mutex _lock;       /*互斥锁*/
    std::condition_variable _task_condcv;       /*条件变量*/
    std::atomic<bool> _run{true};   /*线程池是否在执行（未关闭）*/
    std::atomic<int> _idle_thread_num{0};       /*空闲线程数量*/
    const int THREAD_MAX_NUM = 16;
public:
    ThreadPool(unsigned short size = 6){ addThread(size); }
    ~ThreadPool()
    {
        _run = false;
        _task_condcv.notify_all();
        for (std::thread& thread:_pool)
        {
            if (thread.joinable())
            {
                thread.join();  /*等待任务结束，前提：线程一定指向完*/
            }
            
        }
        
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool(const ThreadPool&&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&&) = delete;
    
public:
    /*提交一个任务*/
    //future对象调用.get()将会阻塞等待任务执行完毕并获取返回值
    /*
    两种方法可以实现调用类成员函数
    1. std::bind() -> .commit(std::bind(&T::func, &t));
    2. std::mem_fn -> .commit(std::mem_fn(&T::func), this);
    */
    template<class F, class... Args>    //可变参数模板
    auto commit(F&& f, Args&&... args) -> std::future<decltype(f(args...))> //返回一个future对象
    {
        if(!_run)   /*如果关闭了线程池，抛出异常*/
            throw std::exception();
        using RetType = decltype(f(args...));   //函数f的返回类型
        auto task_ptr = std::make_shared<std::packaged_task<RetType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );  //把函数入口及参数打包（绑定）

        std::future<RetType> future = task_ptr->get_future();   //异步得到返回结果
        {   //添加到任务队列，当前语句块访问任务队列，需要加锁
            std::lock_guard<std::mutex> lock{_lock}; 
            _tasks.emplace([task_ptr]{(*task_ptr)();}); /*将任务函数入口包装成lambda表达式（函数对象）加入队列*/
        }
#ifdef THREADPOOL_AUTO_GROW
        //若设置了自动增长，没有空闲线程，并且满足条件，则自动增加一个线程
        if(_idle_thread_num < 1 && _pool.size() < THREAD_MAX_NUM)
            addThread(1);
#endif // !THREADPOOL_AUTO_GROW
        _task_condcv.notify_one();  //唤醒一个工作线程来执行
        return future;
    }

    /*返回空闲线程数量*/
    int idleCount(){return _idle_thread_num;}
    /*返回线程数量*/
    int thrCount(){return _pool.size();};
#ifndef THREADPOOL_AUTO_GROW
private:
#endif // !THREADPOOL_AUTO_GROW
    /*添加指定数量的线程*/
    void addThread(unsigned short size)
    {
        for (; _pool.size() < THREAD_MAX_NUM && size > 0; --size)
        {
            auto worker = [this] { //工作线程函数
					while (_run)
					{
						Task task; // 获取一个待执行的 task
						{
							// unique_lock 相比 lock_guard 的好处是：可以随时 unlock() 和 lock()
							std::unique_lock<std::mutex> lock{ _lock };
							_task_condcv.wait(lock, [this] {
								return !_run || !_tasks.empty();
								}); // wait 直到有 task
							if (!_run && _tasks.empty())
								return;
							task = move(_tasks.front()); // 按先进先出从队列取一个 task
							_tasks.pop();
						}
						_idle_thread_num--;
						task();//执行任务
						_idle_thread_num++;
					}
				};
            _pool.emplace_back(worker);
			_idle_thread_num++;
        }
    }
};



