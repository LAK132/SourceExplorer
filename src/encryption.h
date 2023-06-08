#ifndef ENCRYPTION_H
#define ENCRYPTION_H

#include <lak/compiler.hpp>

#if (defined(LAK_COMPILER_GNUC) || defined(LAK_COMPILER_CLANG) ||             \
     defined(LAK_COMPILER_MSVC)) &&                                           \
  defined(LAK_ARCH_X86_COMPAT)
#	define SE_HAS_INTRIN
#endif

#include "explorer.h"

#include <lak/span.hpp>

#include <assert.h>
#include <cstring>
#ifdef SE_HAS_INTRIN
#	include <emmintrin.h>
#endif
#include <iostream>
#include <stdint.h>
#include <utility>
#include <vector>

#ifdef SE_HAS_INTRIN
union alignas(__m128i) m128i_t
{
	__m128i m128i;
	int8_t m128i_i8[16];
	int16_t m128i_i16[8];
	int32_t m128i_i32[4];
	int64_t m128i_i64[2];
	uint8_t m128i_u8[16];
	uint16_t m128i_u16[8];
	uint32_t m128i_u32[4];
	uint64_t m128i_u64[2];
	operator __m128i() const { return m128i; }
	operator __m128i &() { return m128i; }
	operator const __m128i &() const { return m128i; }
};
#endif

struct encryption_table
{
	union decode_buffer_t
	{
#ifdef SE_HAS_INTRIN
		m128i_t m128i[64];
		__m128i _m128i[64];
#endif
		int8_t i8[64 * 16];
		int16_t i16[64 * 8];
		int32_t i32[64 * 4];
		int64_t i64[64 * 2];
		uint8_t u8[64 * 16];
		uint16_t u16[64 * 8];
		uint32_t u32[64 * 4];
		uint64_t u64[64 * 2];
	};
	decode_buffer_t decode_buffer;
	bool valid = false;

	bool init(lak::span<const uint8_t, 0x100> magic_key, const char magic_char);

	bool decode(lak::span<byte_t> chunk) const;
};

std::vector<uint8_t> KeyString(const std::u16string &str);

#endif