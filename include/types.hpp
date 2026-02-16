#ifndef SCHEDSIM_TYPES_H
#define SCHEDSIM_TYPES_H

#include <chrono>
#include <cstdint>

// for literal time quantites e.g. 10ms
using namespace std::literals;

// std::chrono time library aliases
using hrclock_t     = std::chrono::high_resolution_clock;
using time_point    = std::chrono::time_point<hrclock_t>;
using sec_t         = std::chrono::seconds;
using ms_t          = std::chrono::milliseconds;
using us_t          = std::chrono::microseconds;

// integer type aliases
using i8    = int8_t;
using u8    = uint8_t;
using i16   = int16_t;
using u16   = uint16_t;
using i32   = int32_t;
using u32   = uint32_t;
using i64   = int64_t;
using u64   = uint64_t;
#endif
