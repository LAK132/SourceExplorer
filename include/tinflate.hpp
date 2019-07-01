/*
 * tinflate.c -- tiny inflate library
 *
 * Written by Andrew Church <achurch@achurch.org>
 *
 * Re-written for C++ by LAK132 <https://github.com/LAK132>
 *
 * This source code is public domain.
 */

#ifndef TINFLATE_H
#define TINFLATE_H

#include <cstdint>
#include <cstddef>
#include <vector>
#include <deque>

namespace lak
{
    namespace tinf
    {
        enum class state_t : uint8_t
        {
            INITIAL = 0,
            PARTIAL_ZLIB_HEADER,
            HEADER,
            UNCOMPRESSED_LEN,
            UNCOMPRESSED_ILEN,
            UNCOMPRESSED_DATA,
            LITERAL_COUNT,
            DISTANCE_COUNT,
            CODELEN_COUNT,
            READ_CODE_LENGTHS,
            READ_LENGTHS,
            READ_LENGTHS_16,
            READ_LENGTHS_17,
            READ_LENGTHS_18,
            READ_SYMBOL,
            READ_LENGTH,
            READ_DISTANCE,
            READ_DISTANCE_EXTRA
        };

        enum class error_t : uint8_t
        {
            OK                          = 0x0,

            NO_DATA                     = 0x1,

            INVALID_PARAMETER           = 0x2,
            CUSTOM_DICTIONARY           = 0x3,

            INVALID_STATE               = 0x4,
            INVALID_BLOCK_CODE          = 0x5,
            OUT_OF_DATA                 = 0x6,
            CORRUPT_STREAM              = 0x7,
            HUFFMAN_TABLE_GEN_FAILED    = 0x8,
            INVALID_SYMBOL              = 0x9,
            INVALID_DISTANCE            = 0xA,

            NO_SYMBOLS                  = 0xB,
            TOO_MANY_SYMBOLS            = 0xC,
            INCOMPLETE_TREE             = 0xD
        };

        struct decompression_state_t
        {
            std::vector<uint8_t>::const_iterator begin;
            std::vector<uint8_t>::const_iterator end;

            state_t state       = state_t::INITIAL;
            uint32_t crc        = 0;
            uint32_t bitAccum   = 0;
            uint32_t numBits    = 0;
            bool final          = false;
            bool anaconda       = false;

            uint8_t firstByte;
            uint8_t blockType;
            uint32_t counter;
            uint32_t symbol;
            uint32_t lastValue;
            uint32_t repeatLength;
            uint32_t repeatCount;
            uint32_t distance;

            uint32_t len;
            uint32_t ilen;
            uint32_t nread;

            int16_t literalTable[0x23E]; // (288 * 2) - 2
            int16_t distanceTable[0x3E]; // (32 * 2) - 2
            uint32_t literalCount;
            uint32_t distanceCount;
            uint32_t codelenCount;
            int16_t codelenTable[0x24]; // (19 * 2) - 2
            uint8_t literalLen[0x120]; // 288
            uint8_t distanceLen[0x20]; // 32
            uint8_t codelenLen[0x13]; // 19

            template<typename T>
            bool peek_bits(const size_t n, T &var)
            {
                while (numBits < n)
                {
                    if (begin >= end) return true;
                    bitAccum |= ((uint32_t) *begin) << numBits;
                    numBits += 8;
                    ++begin;
                }
                var = bitAccum & ((1UL << n) - 1);
                return false;
            }

            template<typename T>
            bool get_bits(const size_t n, T &var)
            {
                while (numBits < n)
                {
                    if (begin >= end) return true;
                    bitAccum |= ((uint32_t) *begin) << numBits;
                    numBits += 8;
                    ++begin;
                }
                var = bitAccum & ((1UL << n) - 1);
                bitAccum >>= n;
                numBits -= n;
                return false;
            }

            template<typename T>
            bool get_huff(T &var, short *table)
            {
                uint32_t bitsUsed = 0;
                uint32_t index = 0;
                for (;;)
                {
                    if (numBits <= bitsUsed)
                    {
                        if (begin >= end) return true;
                        bitAccum |= ((uint32_t) *begin) << numBits;
                        numBits += 8;
                        ++begin;
                    }
                    index += (bitAccum >> bitsUsed) & 1;
                    ++bitsUsed;
                    if (table[index] >= 0) break;
                    index = ~(table[index]);
                }
                bitAccum >>= bitsUsed;
                numBits -= bitsUsed;
                var = table[index];
                return false;
            }
        };
    }

    tinf::error_t tinflate(
        const std::vector<uint8_t> &compressed,
        std::deque<uint8_t> &output,
        uint32_t *crc = nullptr
    );

    tinf::error_t tinflate(
        typename std::vector<uint8_t>::const_iterator begin,
        typename std::vector<uint8_t>::const_iterator end,
        std::deque<uint8_t> &output,
        uint32_t *crc = nullptr
    );

    tinf::error_t tinflate(
        const std::vector<uint8_t> &compressed,
        std::deque<uint8_t> &output,
        tinf::decompression_state_t &state,
        uint32_t *crc = nullptr
    );

    tinf::error_t tinflate(
        typename std::vector<uint8_t>::const_iterator begin,
        typename std::vector<uint8_t>::const_iterator end,
        std::deque<uint8_t> &output,
        tinf::decompression_state_t &state,
        uint32_t *crc = nullptr
    );

    tinf::error_t tinflate_block(
        std::deque<uint8_t> &output,
        tinf::decompression_state_t &state
    );

    tinf::error_t gen_huffman_table(
        uint32_t symbols,
        const uint8_t *lengths,
        bool allow_no_symbols,
        int16_t *table
    );
}

#endif
