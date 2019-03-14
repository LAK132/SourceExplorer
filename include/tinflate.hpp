/*
 * tinflate.c -- tiny inflate library
 *
 * Written by Andrew Church <achurch@achurch.org>
 *
 *
 * tinflate.hpp -- modern C++ rewrite of tinflate.c
 *
 * Written by LAK132 <kicodora.com>
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
    enum tinflate_state_t {
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

    enum tinflate_error_t
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
        tinflate_state_t state;

        std::vector<uint8_t>::const_iterator begin;
        std::vector<uint8_t>::const_iterator end;

        uint32_t crc;
        uint32_t bitAccum;
        uint32_t numBits;
        bool final;

        uint8_t firstByte;
        uint8_t blockType;
        uint32_t counter;
        uint32_t symbol;
        uint32_t lastValue;
        uint32_t repeatLength;

        uint32_t len;
        uint32_t ilen;
        uint32_t nread;

        int16_t literalTable[288*2-2];
        int16_t distanceTable[32*2-2];
        uint32_t literalCount;
        uint32_t distanceCount;
        uint32_t codelenCount;
        int16_t codelenTable[19*2-2];
        uint8_t literalLen[288], distanceLen[32], codelenLen[19];
    };

    tinflate_error_t tinflate(
        const std::vector<uint8_t> &compressed,
        std::deque<uint8_t> &output,
        uint32_t *crc
    );

    tinflate_error_t tinflate(
        const std::vector<uint8_t> &compressed,
        std::deque<uint8_t> &output,
        uint32_t *crc,
        decompression_state_t &state
    );

    tinflate_error_t tinflate_block(
        std::deque<uint8_t> &output,
        decompression_state_t &state
    );

    tinflate_error_t gen_huffman_table(
        uint32_t symbols,
        const uint8_t *lengths,
        bool allow_no_symbols,
        int16_t *table
    );
}

#endif
