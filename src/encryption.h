#include <stdint.h>
#include <vector>
#include <iostream>
#include <assert.h>
#include <cstring>
#include <utility>
#include <emmintrin.h>
#include <cstring>
#include "lak.h"

#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include "explorer.h"

#ifdef _WIN32
typedef __m128i m128i_t;
#else
union alignas(__m128i) m128i_t
{
    int8_t      m128i_i8[16];
    int16_t     m128i_i16[8];
    int32_t     m128i_i32[4];
    int64_t     m128i_i64[2];
    uint8_t     m128i_u8[16];
    uint16_t    m128i_u16[8];
    uint32_t    m128i_u32[4];
    uint64_t    m128i_u64[2];
    operator __m128i() const { return *reinterpret_cast<const __m128i*>(this); }
    operator __m128i&() { return *reinterpret_cast<__m128i*>(this); }
    operator const __m128i&() const { return *reinterpret_cast<const __m128i*>(this); }
};
#endif

using decode_buffer_t = m128i_t[64];

bool GenerateTable(
    decode_buffer_t &decodeBuffer,
    const std::vector<uint8_t> &magic_key,
    const __m128i &xmmword,
    const char magic_char
);

void DecodeWithTable(
    std::vector<uint8_t> &chunk,
    decode_buffer_t &decodeBuffer
);

bool DecodeChunk(
    std::vector<uint8_t> &chunk,
    const std::vector<uint8_t> &magic_key,
    const m128i_t &xmmword,
    const char magic_char = '6'
);

std::vector<uint8_t> KeyString(const std::u16string &str);

// void MakeDecryptionKey(
//     const char *a1,
//     const char *a3,
//     const bool unicode
// );

#endif