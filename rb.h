#define RB_MODE_MPMC
// #define RB_MODE_SPSC

#ifdef RB_MODE_SPSC
#define RB_INC_SPSC
#elif defined(RB_MODE_MPMC)
#define RB_INC_MDC
#else
#error RB_MODE must be defined
#endif

#define RB_INC_NANOLOG
#include "rb_deps.h"

#ifndef RAVEN_BENCH_H
#define RAVEN_BENCH_H

//format: X(type, name, printer)
#define RB_DATA_POINT_FEILDS \
    X(uint32_t, orderbook_id, PLAIN) \
    X(uint8_t, location, INT) \
    X(uint8_t, message_type, INT) \
    X(uint8_t, flags, BITSET8)

// ██████╗  █████╗ ██╗   ██╗███████╗███╗   ██╗
// ██╔══██╗██╔══██╗██║   ██║██╔════╝████╗  ██║
// ██████╔╝███████║██║   ██║█████╗  ██╔██╗ ██║
// ██╔══██╗██╔══██║╚██╗ ██╔╝██╔══╝  ██║╚██╗██║
// ██║  ██║██║  ██║ ╚████╔╝ ███████╗██║ ╚████║
// ╚═╝  ╚═╝╚═╝  ╚═╝  ╚═══╝  ╚══════╝╚═╝  ╚═══╝
//
// ██████╗ ███████╗███╗   ██╗ ██████╗██╗  ██╗
// ██╔══██╗██╔════╝████╗  ██║██╔════╝██║  ██║
// ██████╔╝█████╗  ██╔██╗ ██║██║     ███████║
// ██╔══██╗██╔══╝  ██║╚██╗██║██║     ██╔══██║
// ██████╔╝███████╗██║ ╚████║╚██████╗██║  ██║
// ╚═════╝ ╚══════╝╚═╝  ╚═══╝ ╚═════╝╚═╝  ╚═╝


#include <bitset>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>

#ifdef RB_ENABLE
#ifdef RB_MODE_SPSC
#define rb_bench_c(tid, ...) rb_bench_with_dp((rb_data_point_t){.thread_id = (tid), __VA_ARGS__});
#define rb_bench(tid, ...) rb_bench_with_dp(rb_data_point_t{0, (tid), __VA_ARGS__});
#else
#define rb_bench_c(...) rb_bench_with_dp((rb_data_point_t){__VA_ARGS__});
#define rb_bench(...) rb_bench_with_dp(rb_data_point_t{0,__VA_ARGS__});
#endif
#define rb_bench_dir_c(...) rb_bench_with_dp_dir((rb_data_point_t){__VA_ARGS__});
#define rb_bench_dir(...) rb_bench_with_dp_dir(rb_data_point_t{0,__VA_ARGS__});
#else
#define rb_bench_c(...)
#define rb_bench(...)
#define rb_bench_dir_c(...)
#define rb_bench_dir(...)
#endif

typedef struct rb_data_point
{
    uint64_t timestamp;
#ifdef RB_MODE_SPSC
    uint8_t thread_id;
#endif
#define X(type, name, printer) type name;
    RB_DATA_POINT_FEILDS
#undef X
} rb_data_point_t;

#ifdef RB_MODE_SPSC
#ifndef RB_LOGGER_QUEUE_SIZE
#define RB_LOGGER_QUEUE_SIZE 2048
#endif
extern RB_SPSCQueue<rb_data_point_t, RB_LOGGER_QUEUE_SIZE> rb_logger_queue_arr[];
#elif defined(RB_MODE_MPMC)
extern rb_moodycamel::ConcurrentQueue<rb_data_point_t> rb_logger_queue;
#endif

void rb_init(std::string log_file_name);
void rb_bench_with_dp(rb_data_point_t data_point);
void rb_bench_with_dp_dir(rb_data_point_t data_point);
void rb_write_log(rb_data_point_t data_point);
void rb_log();
void rb_log_block();

#endif

// #define RB_IMPLEMENTATION
#ifdef RB_IMPLEMENTATION

#ifdef RB_MODE_SPSC
#ifndef RB_THREAD_COUNT
#error RB_THREAD_COUNT must be defined
#endif
RB_SPSCQueue<rb_data_point_t, RB_LOGGER_QUEUE_SIZE> rb_logger_queue_arr[RB_THREAD_COUNT];
#else
rb_moodycamel::ConcurrentQueue<rb_data_point_t> rb_logger_queue;
#endif

static inline void rb_get_str_from_nanoseconds(uint64_t nanoseconds, std::string & timestamp)
{
    int x = nanoseconds / 1000000000;
    int nano = nanoseconds - x * 1000000000;
    time_t c = x;

    struct tm *ts;
    char buf[22];
    char buf2[30];
    ts = localtime(&c);

    strftime(buf, sizeof (buf), "%Y-%m-%d %H:%M:%S", ts);
    sprintf(buf2, "%s.%d", buf, nano);
    timestamp = std::string(buf2);
}

static inline std::string get_date_string()
{
    auto t = std::time(nullptr);
    auto tm = std::localtime(&t);
    std::string formatString = "%y%m%d_%H:%M:%S";

    std::string buffer;
    buffer.resize(formatString.size());
    int len = strftime(&buffer[0], buffer.size(), formatString.c_str(), tm);
    while (len == 0) {
        buffer.resize(buffer.size()*2);
        len = strftime(&buffer[0], buffer.size(), formatString.c_str(), tm);
    }
    
    buffer.erase(std::find(buffer.begin(), buffer.end(), '\0'), buffer.end());
    return buffer;
}

void rb_init(std::string log_file_name)
{
#ifdef RB_ENABLE
    std::cout << "WARNING RAVEN BENCH IS ENABLED!!!" << std::endl;
    rb_nanolog::GuaranteedLogger gl;
    rb_nanolog::initialize(gl, "./rb_logs/", "rb_" + log_file_name + get_date_string(), 10);
#endif
}

void rb_bench_with_dp(rb_data_point_t data_point)
{
#ifdef RB_MODE_SPSC
    data_point.timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    rb_logger_queue_arr[data_point.thread_id].tryPush([data_point](rb_data_point_t *d){
                d->timestamp = data_point.timestamp;
#define X(type, name, printer) d->name = data_point.name;
    RB_DATA_POINT_FEILDS
#undef X
            });
#else
    data_point.timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    rb_logger_queue.enqueue(data_point);
#endif
}

void rb_bench_with_dp_dir(rb_data_point_t data_point)
{
    data_point.timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    rb_write_log(data_point);
}

void rb_write_log(rb_data_point_t data_point)
{
    std::string timestamp_str;
    rb_get_str_from_nanoseconds(data_point.timestamp, timestamp_str);

    std::stringstream ss;

    ss << "RB_LOG"
        << " | timestamp_str: " << timestamp_str
        << " | timestamp: " << data_point.timestamp;
#ifdef RB_MODE_SPSC
    ss << " | thread_id: " << (int)data_point.thread_id;
#endif

#define RB_PRINT_PLAIN(type, name) ss << " | " #name ": " << (data_point.name);
#define RB_PRINT_INT(type, name) ss << " | " #name ": " << (int)(data_point.name);
#define RB_PRINT_BITSET8(type, name) ss << " | " #name ": " << std::bitset<8>(data_point.name);
#define X(type, name, printer) RB_PRINT_##printer(type, name)
    RB_DATA_POINT_FEILDS
#undef X
#undef RB_PRINT_PLAIN
#undef RB_PRINT_INT
#undef RB_PRINT_BITSET8

    LOG_INFO << ss.str();
}

void rb_log()
{
#ifdef RB_MODE_SPSC
    for(size_t i = 0; i < RB_THREAD_COUNT; i++)
    {
        while(rb_logger_queue_arr[i].tryPop([](rb_data_point_t *d){
            rb_write_log(*d);
        }));
    }
#else
    rb_data_point_t data_point;
    while(rb_logger_queue.try_dequeue(data_point))
    {
        rb_write_log(data_point);
    }
#endif
}

void rb_log_block()
{
    rb_data_point_t data_point;
    while(1)
    {
        rb_log();
    }
}

#endif
