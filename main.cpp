#include <chrono>
#include <thread>
#define NANOLOG_IMPLEMENTATION
#define RB_IMPLEMENTATION
#define RB_ENABLE
#define RB_THREAD_COUNT 8
#define RB_THREAD_ID 0
#include "rb.h"

void my_thread_0()
{
    for(int i = 0; i < 1000; i++)
    {
        rb_bench_c(0, .orderbook_id = 0, .location = 'X', .flags = (uint8_t)(i % 64));
        std::this_thread::sleep_for(std::chrono::microseconds(150));
    }
}

void my_thread_1()
{
    for(int i = 0; i < 1000; i++)
    {
        rb_bench_c(1, .orderbook_id = 1, .location = 'A', .flags = (uint8_t)(i % 64));
        std::this_thread::sleep_for(std::chrono::microseconds(150));
    }
}

void my_thread_2()
{
    for(int i = 0; i < 1000; i++)
    {
        rb_bench(2, 'B', 0, (uint8_t)(i % 64));
        std::this_thread::sleep_for(std::chrono::microseconds(30));
    }
}


int main(int argc, char **argv)
{
    rb_init("test");

    std::thread t0(my_thread_0);
    std::thread t1(my_thread_1);
    std::thread t2(my_thread_2);


    t0.join();
    t1.join();
    t2.join();

    rb_log();

    return 0;
}
