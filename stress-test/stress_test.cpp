/* ************************************************************************
> File Name:     stress_test.cpp
> Author:        程序员Carl
> 微信公众号:    代码随想录
> Created Time:  Sun 16 Dec 2018 11:56:04 AM CST
> Description:   
 ************************************************************************/

#include <iostream>
#include <chrono>
#include <cstdlib>
#include <pthread.h>
#include <time.h>
#include "../skiplist.h"

#define NUM_THREADS 1  //线程数
#define TEST_COUNT 100000  //测试次数？
SkipList<int, std::string> skipList(18);  //实例化一个18层层高的跳表

//注意这里都是函数指针，因为线程执行的时候要传入函数地址
void *insertElement(void* threadid) {  //插入元素，参数是线程ID
    long tid; 
    tid = (long)threadid;  //把线程ID转成long型
    std::cout << tid << std::endl;  
    int tmp = TEST_COUNT/NUM_THREADS;  //是100000，表示插入1000次？
	for (int i=tid*tmp, count=0; count<tmp; i++) {  //这个i是用来干啥的？
        count++;
		skipList.insert_element(rand() % TEST_COUNT, "a");  //key随机，val是a
	}
    pthread_exit(NULL);
}

void *getElement(void* threadid) {
    long tid; 
    tid = (long)threadid;
    std::cout << tid << std::endl;  
    int tmp = TEST_COUNT/NUM_THREADS; 
	for (int i=tid*tmp, count=0; count<tmp; i++) {
        count++;
		skipList.search_element(rand() % TEST_COUNT);  //获取元素
	}
    pthread_exit(NULL);
}

int main() {
    srand (time(NULL));  //产生随机数种子
    {

        pthread_t threads[NUM_THREADS];  //线程池？
        int rc;
        int i;

        auto start = std::chrono::high_resolution_clock::now();  //获取当前时间

        for( i = 0; i < NUM_THREADS; i++ ) {
            std::cout << "main() : creating thread, " << i << std::endl;
            //创建线程，四个参数分别为指向线程标识符的指针，设置线程属性，线程运行执行的第一个函数的地址，函数的参数
            rc = pthread_create(&threads[i], NULL, insertElement, (void *)i);

            if (rc) {  //rc的值：创建成功返回0，创建失败返回错误编号
                std::cout << "Error:unable to create thread," << rc << std::endl;
                exit(-1);
            }
        }

        void *ret;
        for( i = 0; i < NUM_THREADS; i++ ) {
            //以阻塞的方式等待threads[i]指定的线程结束
            // ret是void指针，用来存储被等待线程的返回值
            if (pthread_join(threads[i], &ret) !=0 )  //成功返回0，失败返回错误编号
            {
                perror("pthread_create() error"); 
                exit(3);
            }
        }
        auto finish = std::chrono::high_resolution_clock::now(); 
        std::chrono::duration<double> elapsed = finish - start;  //计算所花时间
        std::cout << "insert elapsed:" << elapsed.count() << std::endl;
    }
    // skipList.displayList();

    // {
    //     pthread_t threads[NUM_THREADS];
    //     int rc;
    //     int i;
    //     auto start = std::chrono::high_resolution_clock::now();

    //     for( i = 0; i < NUM_THREADS; i++ ) {
    //         std::cout << "main() : creating thread, " << i << std::endl;
    //         rc = pthread_create(&threads[i], NULL, getElement, (void *)i);

    //         if (rc) {
    //             std::cout << "Error:unable to create thread," << rc << std::endl;
    //             exit(-1);
    //         }
    //     }

    //     void *ret;
    //     for( i = 0; i < NUM_THREADS; i++ ) {
    //         if (pthread_join(threads[i], &ret) !=0 )  {
    //             perror("pthread_create() error"); 
    //             exit(3);
    //         }
    //     }

    //     auto finish = std::chrono::high_resolution_clock::now(); 
    //     std::chrono::duration<double> elapsed = finish - start;
    //     std::cout << "get elapsed:" << elapsed.count() << std::endl;
    // }

	pthread_exit(NULL);
    return 0;

}
