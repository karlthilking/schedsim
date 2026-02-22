#ifndef SCHEDSIM_TYPES_H
#define SCHEDSIM_TYPES_H

#include <chrono>
#include <cstdint>

// for literal time quantites e.g. 10ms
using namespace std::literals;
// for high_resolution_clock, duration_cast, etc
using namespace std::chrono;

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
