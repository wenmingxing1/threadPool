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
    //����һ���ܹ�����ִ���ĸ��̵߳��̳߳�
    ThreadPool pool(4);
    //��������ִ���̵߳Ľ���б�
    vector<future<string>> results;

    //�����˸���Ҫִ�е��߳�����
    for (int i = 0; i < 8; ++i) {
        //������ִ������ķ���ֵ��ӵ�����б���
        results.emplace_back(
            //������Ĵ�ӡ������ӵ��̳߳��в���ִ�У���������һ��������ʽ
            pool.enqueue([i] {
                         cout << "hello " << i << endl;
                         //��һ������󣬸��̻߳�ȴ�1����
                         this_thread::sleep_for(chrono::seconds(1));
                         //Ȼ���ټ������������ִ�����
                         cout << "world " << i << endl;
                         return string("---thread ") + to_string(i) + string(" finished.---");
                         })
                             );
    }

    //����߳�����Ľ��
    for (auto && result : results)
        cout << result.get() << "   ";

    cout << endl;

    return 0;
}
