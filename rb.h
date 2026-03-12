#ifndef RAVEN_BENCH_H
#define RAVEN_BENCH_H

//format: X(type, name, printer)
#define RB_DATA_POINT_FEILDS \
    X(uint32_t, orderbook_id, PLAIN) \
    X(uint32_t, seqnum, PLAIN) \
    X(uint8_t, message_type, INT) \
    X(uint8_t, flags, BITSET8)

////////////////////////////////
////////////////////////////////
/////______///////_______///////
/////__...__//////___...__//////
/////__...__//////___...__//////
/////______///////_______///////
/////___.___//////___...__//////
/////___..___/////___...__//////
/////___...___////_______///////
////////////////////////////////
////////////////////////////////

#include <bitset>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <sstream>
#include "rb_deps.h"

#define rb_bench(...) rb_bench_with_dp((rb_data_point_t){__VA_ARGS__});

typedef struct rb_data_point
{
    uint64_t timestamp;
#define X(type, name, printer) type name;
    RB_DATA_POINT_FEILDS
#undef X
} rb_data_point_t;

extern moodycamel::ConcurrentQueue<rb_data_point_t> rb_logger_queue;

void rb_init(std::string log_file_name);
void rb_bench_with_dp(rb_data_point_t data_point);
void rb_log();
void rb_log_block();

// #define RB_IMPLEMENTATION
#ifdef RB_IMPLEMENTATION

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
    auto tm = *std::localtime(&t);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%d-%m-%Y_%H-%M-%S");
    auto str = oss.str();
    
    return str;
}

moodycamel::ConcurrentQueue<rb_data_point_t> rb_logger_queue;

void rb_init(std::string log_file_name)
{
    nanolog::GuaranteedLogger gl;
    nanolog::initialize(gl, "./rb_logs/", "rb_" + log_file_name + get_date_string(), 10);
}

void rb_bench_with_dp(rb_data_point_t data_point)
{
    data_point.timestamp = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    rb_logger_queue.enqueue(data_point);
}

void rb_log()
{
    rb_data_point_t data_point;
    while(rb_logger_queue.try_dequeue(data_point))
    {
        std::string timestamp_str;
        rb_get_str_from_nanoseconds(data_point.timestamp, timestamp_str);

        std::stringstream ss;

        ss << "RB_LOG"
            << " | timestamp_str: " << timestamp_str
            << " | timestamp: " << data_point.timestamp;

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

#endif
