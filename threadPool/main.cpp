//
// main.cpp
//

#include <iostream>
#include<vector>
#include<string>
#include<future>
#include<thread>
#include<chrono>    //chrono::seconds

#include "ThreadPool.h"

using namespace std;

int main()
{
    //创建一个能够并发执行四个线程的线程池
    ThreadPool pool(4);
    //创建并发执行线程的结果列表
    vector<future<string>> results;

    //启动八个需要执行的线程任务
    for (int i = 0; i < 8; ++i) {
        //将并发执行任务的返回值添加到结果列表中
        results.emplace_back(
            //将下面的打印任务添加到线程池中并发执行，该任务是一个正则表达式
            pool.enqueue([i] {
                         cout << "hello " << i << endl;
                         //上一行输出后，该线程会等待1秒钟
                         this_thread::sleep_for(chrono::seconds(1));
                         //然后再继续输出并返回执行情况
                         cout << "world " << i << endl;
                         return string("---thread ") + to_string(i) + string(" finished.---");
                         })
                             );
    }

    //输出线程任务的结果
    for (auto && result : results)
        cout << result.get() << "   ";

    cout << endl;

    return 0;
}
