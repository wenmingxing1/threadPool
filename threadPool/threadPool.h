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
    //���캯����ֻ��������һ�������Ĺ����߳�(worker)
    //���̳߳��д���threads�������߳�
    ThreadPool(size_t threads) : stop (false) {
        //����threads�����Ĺ����߳�worker
        for (size_t i = 0; i < threads; ++i)
            workers.emplace_back(
                //�˴���lambda���ʽ����this�����̳߳�ʵ��
                [this] {
                    //ѭ��������ٻ���
                    for(;;){
                        //���庯��������������洢�κη�������Ϊvoid������Ϊ�յĺ���
                        function<void()> task;

                        //�ٽ���
                        {
                            //����������
                            unique_lock<mutex> lock(this->queue_mutex);

                            //������ǰ�̣߳�ֱ��condition_variable������
                            this->condition.wait(lock, [this] {return this->stop || !this->tasks.empty();});

                            //�����ǰ�̳߳��Ѿ������ҵȴ�����Ϊ�գ���ֱ�ӷ���
                            if (this->stop && this->tasks.empty())
                                return;

                            //�������������еĶ���������Ϊ��Ҫִ�е��������
                            task = move(this->tasks.front());
                            this->tasks.pop();
                        }

                        //ִ�е�ǰ����
                        task();
                    }
                }
                                 );
    }

    //���̳߳��������߳�
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args)     //ʹ��β�÷������ͣ���result_of���ƶϷ�������
        -> future<typename result_of<F(Args...)>::type>
    {
        //�Ƶ����񷵻�����
        using return_type = typename result_of<F(Args...)>::type;

        //��õ�ǰ����
        auto task = make_shared<packaged_task<return_type()>> (
            bind(forward<F>(f), forward<Args>(args)...)
        );

        //���future�����Թ�ʵʩ�߳�ͬ��
        future<return_type> res = task->get_future();

        //�ٽ���
        {
            unique_lock<mutex> lock(queue_mutex);

            //��ֹ���̳߳�ֹͣ������µ��߳�
            if (stop)
                throw runtime_error("enqueue on stopped ThreadPool");

            //���߳���ӵ�ִ�����������
            tasks.emplace([task]{(*task)();});
        }

        //֪ͨһ�����ڵȴ����߳�
        condition.notify_one();
        return res;
    }

    //���������̳߳��д������߳�
    ~ThreadPool() {
        //�ٽ���
        {
            //����������
            unique_lock<mutex> lock(queue_mutex);

            //�����̳߳�״̬
            stop = true;
        }

        //֪ͨ���еȴ��߳�
        condition.notify_all();

        //ʹ�����첽�߳�ת��Ϊͬ���߳�
        for (thread &worker:workers)
            worker.join();
    }

private:
    //��Ҫ����׷���߳�����֤����ʹ��join
    vector<thread> workers;
    //�������
    queue<function<void()>> tasks;

    //ͬ�����
    mutex queue_mutex;  //������
    condition_variable condition;   //������������

    //ֹͣ���
    bool stop;


};

#endif // THREADPOOL_H_INCLUDED
