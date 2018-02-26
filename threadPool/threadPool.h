#ifndef THREADPOOL_H_INCLUDED
#define THREADPOOL_H_INCLUDED

//
// ThreadPool.h
//

#include<iostream>
#include<vector>
#include<string>
#include<thread>
#include<future>        //future, packaged_task
#include<functional>
#include<condition_variable>    //condition_variable
#include<utility>   //move, forward
#include<queue>
#include<memory>    //make_shared
#include<stdexcept>     //runtime_error
#include<mutex>     //mutex, unique_lock

using namespace std;

class ThreadPool {
public:
    //构造函数，只负责启动一定数量的工作线程(worker)
    //在线程池中创建threads个工作线程
    ThreadPool(size_t threads) : stop (false) {
        //启动threads数量的工作线程worker
        for (size_t i = 0; i < threads; ++i)
            workers.emplace_back(
                //此处的lambda表达式捕获this，即线程池实例
                [this] {
                    //循环避免虚假唤醒
                    for(;;){
                        //定义函数对象的容器，存储任何返回类型为void参数表为空的函数
                        function<void()> task;

                        //临界区
                        {
                            //创建互斥锁
                            unique_lock<mutex> lock(this->queue_mutex);

                            //阻塞当前线程，直到condition_variable被唤醒
                            this->condition.wait(lock, [this] {return this->stop || !this->tasks.empty();});

                            //如果当前线程池已经结束且等待队列为空，则直接返回
                            if (this->stop && this->tasks.empty())
                                return;

                            //否则就让任务队列的队首任务作为需要执行的任务出队
                            task = move(this->tasks.front());
                            this->tasks.pop();
                        }

                        //执行当前任务
                        task();
                    }
                }
                                 );
    }

    //向线程池中增加线程
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args)     //使用尾置返回类型，以result_of来推断返回类型
        -> future<typename result_of<F(Args...)>::type>
    {
        //推导任务返回类型
        using return_type = typename result_of<F(Args...)>::type;

        //获得当前任务
        auto task = make_shared<packaged_task<return_type()>> (
            bind(forward<F>(f), forward<Args>(args)...)
        );

        //获得future对象以供实施线程同步
        future<return_type> res = task->get_future();

        //临界区
        {
            unique_lock<mutex> lock(queue_mutex);

            //禁止在线程池停止后假如新的线程
            if (stop)
                throw runtime_error("enqueue on stopped ThreadPool");

            //将线程添加到执行任务队列中
            tasks.emplace([task]{(*task)();});
        }

        //通知一个正在等待的线程
        condition.notify_one();
        return res;
    }

    //销毁所有线程池中创建的线程
    ~ThreadPool() {
        //临界区
        {
            //创建互斥锁
            unique_lock<mutex> lock(queue_mutex);

            //设置线程池状态
            stop = true;
        }

        //通知所有等待线程
        condition.notify_all();

        //使所有异步线程转化为同步线程
        for (thread &worker:workers)
            worker.join();
    }

private:
    //需要持续追踪线程来保证可以使用join
    vector<thread> workers;
    //任务队列
    queue<function<void()>> tasks;

    //同步相关
    mutex queue_mutex;  //互斥锁
    condition_variable condition;   //互斥条件变量

    //停止相关
    bool stop;


};

#endif // THREADPOOL_H_INCLUDED
