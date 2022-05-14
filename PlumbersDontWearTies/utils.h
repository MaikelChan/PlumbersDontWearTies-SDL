#pragma once

#include <cstdint>

class utils
{
public:
    static int32_t swap_i32(const int32_t value);
    static uint32_t swap_u32(const uint32_t value);
    static int16_t swap_i16(const int16_t value);
    static uint16_t swap_u16(const uint16_t value);
    static float swap_float(const float value);
    static uint16_t align(const uint16_t value, const uint16_t alignment);
};