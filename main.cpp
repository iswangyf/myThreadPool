#include <iostream>
#include <string>
#include <chrono>
#include "ThreadPool.h"
using namespace std;

//无返回值的函数
void re_void_fun(int slp)
{
    cout<<"hello, re_void_fun!"<<this_thread::get_id()<<endl;
    if (slp > 0)
    {
        cout<<"------re_void_fun sleep: "<<slp<<this_thread::get_id()<<endl;
        this_thread::sleep_for(chrono::milliseconds(slp));
    }
}

//有返回值的仿函数
struct re_int_fun
{
    int operator()(int n)
    {
        cout<<"--- "<<n<<" hello re_int_fun!"<<this_thread::get_id()<<endl;
        this_thread::sleep_for(chrono::milliseconds(5));
        return n*n;
    }
};

int main(int argc, char *argv[])
{
    try
    {
        ThreadPool executor{8};
        vector<future<int>> results;
        for (int i = 0; i < 30; i++)
        {
            cout<<"空闲的线程数："<<executor.idleCount()<<endl;
            results.emplace_back(executor.commit(re_int_fun{}, i));
        }
        
        cout<<"-------re_int_fun的返回结果---------"<<endl;
        for (auto&& result:results)
        {
            cout<<result.get()<<" ";
        }
        cout<<endl;
    
        cout<<" ------sleep (main thread) ------"<<this_thread::get_id()<<endl;
        this_thread::sleep_for(chrono::microseconds(30));

        return 0;
    }
    catch(const std::exception& e)
    {
        std::cerr << "some unhappy happended..."<< this_thread::get_id() << e.what() << '\n';
    }
    
    return 0;
}
