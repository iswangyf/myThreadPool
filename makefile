CXX = g++
CFLAGS = -std=c++11 -g 

TARGET = app
OBJS = main.cpp

all: $(OBJS)
	$(CXX) $(CFLAGS) $(OBJS) -o $(TARGET)  -lpthread

clean:
	rm -rf $(OBJS) $(TARGET)

