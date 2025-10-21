#pragma once

#include<queue>
#include<mutex>
#include<thread>
#include<condition_variable>

//异步写日志队列
template<typename T>
class LockQueue
{
public:
    void Push(const T &data);
    T Pop();
private:
    std::queue<T> _que;
    std::mutex _mutex;
    std::condition_variable _cv;
};


// 多个worker线程都会写日志queue 
template<typename T>
void LockQueue<T>::Push(const T &data)
{   
    std::lock_guard<std::mutex> lock(_mutex);
    _que.push(data);
    //只有一个线程在队列写日志文件
    _cv.notify_one();
}

// 一个线程读日志queue，写日志文件
template<typename T>
T LockQueue<T>::Pop()
{
    std::unique_lock<std::mutex> lock(_mutex);
    while(_que.empty())
    {
        //为空等待
        _cv.wait(lock);
    }
    T data = _que.front();
    _que.pop();
    return data;
}