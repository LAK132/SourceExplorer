/*
 * tinflate.c -- tiny inflate library
 *
 * Written by Andrew Church <achurch@achurch.org>
 *
 *
 * tinflate.cpp -- modern C++ rewrite of tinflate.c
 *
 * Written by Lucas "LAK132" Kleiss <kicodora.com>
 *
 * This source code is public domain.
 */

#include "tinflate.hpp"

namespace lak
{
    namespace tinf
    {
        static constexpr uint32_t crc32_table[256] = {
            0x00000000UL, 0x77073096UL, 0xEE0E612CUL, 0x990951BAUL, 0x076DC419UL,
            0x706AF48FUL, 0xE963A535UL, 0x9E6495A3UL, 0x0EDB8832UL, 0x79DCB8A4UL,
            0xE0D5E91EUL, 0x97D2D988UL, 0x09B64C2BUL, 0x7EB17CBDUL, 0xE7B82D07UL,
            0x90BF1D91UL, 0x1DB71064UL, 0x6AB020F2UL, 0xF3B97148UL, 0x84BE41DEUL,
            0x1ADAD47DUL, 0x6DDDE4EBUL, 0xF4D4B551UL, 0x83D385C7UL, 0x136C9856UL,
            0x646BA8C0UL, 0xFD62F97AUL, 0x8A65C9ECUL, 0x14015C4FUL, 0x63066CD9UL,
            0xFA0F3D63UL, 0x8D080DF5UL, 0x3B6E20C8UL, 0x4C69105EUL, 0xD56041E4UL,
            0xA2677172UL, 0x3C03E4D1UL, 0x4B04D447UL, 0xD20D85FDUL, 0xA50AB56BUL,
            0x35B5A8FAUL, 0x42B2986CUL, 0xDBBBC9D6UL, 0xACBCF940UL, 0x32D86CE3UL,
            0x45DF5C75UL, 0xDCD60DCFUL, 0xABD13D59UL, 0x26D930ACUL, 0x51DE003AUL,
            0xC8D75180UL, 0xBFD06116UL, 0x21B4F4B5UL, 0x56B3C423UL, 0xCFBA9599UL,
            0xB8BDA50FUL, 0x2802B89EUL, 0x5F058808UL, 0xC60CD9B2UL, 0xB10BE924UL,
            0x2F6F7C87UL, 0x58684C11UL, 0xC1611DABUL, 0xB6662D3DUL, 0x76DC4190UL,
            0x01DB7106UL, 0x98D220BCUL, 0xEFD5102AUL, 0x71B18589UL, 0x06B6B51FUL,
            0x9FBFE4A5UL, 0xE8B8D433UL, 0x7807C9A2UL, 0x0F00F934UL, 0x9609A88EUL,
            0xE10E9818UL, 0x7F6A0DBBUL, 0x086D3D2DUL, 0x91646C97UL, 0xE6635C01UL,
            0x6B6B51F4UL, 0x1C6C6162UL, 0x856530D8UL, 0xF262004EUL, 0x6C0695EDUL,
            0x1B01A57BUL, 0x8208F4C1UL, 0xF50FC457UL, 0x65B0D9C6UL, 0x12B7E950UL,
            0x8BBEB8EAUL, 0xFCB9887CUL, 0x62DD1DDFUL, 0x15DA2D49UL, 0x8CD37CF3UL,
            0xFBD44C65UL, 0x4DB26158UL, 0x3AB551CEUL, 0xA3BC0074UL, 0xD4BB30E2UL,
            0x4ADFA541UL, 0x3DD895D7UL, 0xA4D1C46DUL, 0xD3D6F4FBUL, 0x4369E96AUL,
            0x346ED9FCUL, 0xAD678846UL, 0xDA60B8D0UL, 0x44042D73UL, 0x33031DE5UL,
            0xAA0A4C5FUL, 0xDD0D7CC9UL, 0x5005713CUL, 0x270241AAUL, 0xBE0B1010UL,
            0xC90C2086UL, 0x5768B525UL, 0x206F85B3UL, 0xB966D409UL, 0xCE61E49FUL,
            0x5EDEF90EUL, 0x29D9C998UL, 0xB0D09822UL, 0xC7D7A8B4UL, 0x59B33D17UL,
            0x2EB40D81UL, 0xB7BD5C3BUL, 0xC0BA6CADUL, 0xEDB88320UL, 0x9ABFB3B6UL,
            0x03B6E20CUL, 0x74B1D29AUL, 0xEAD54739UL, 0x9DD277AFUL, 0x04DB2615UL,
            0x73DC1683UL, 0xE3630B12UL, 0x94643B84UL, 0x0D6D6A3EUL, 0x7A6A5AA8UL,
            0xE40ECF0BUL, 0x9309FF9DUL, 0x0A00AE27UL, 0x7D079EB1UL, 0xF00F9344UL,
            0x8708A3D2UL, 0x1E01F268UL, 0x6906C2FEUL, 0xF762575DUL, 0x806567CBUL,
            0x196C3671UL, 0x6E6B06E7UL, 0xFED41B76UL, 0x89D32BE0UL, 0x10DA7A5AUL,
            0x67DD4ACCUL, 0xF9B9DF6FUL, 0x8EBEEFF9UL, 0x17B7BE43UL, 0x60B08ED5UL,
            0xD6D6A3E8UL, 0xA1D1937EUL, 0x38D8C2C4UL, 0x4FDFF252UL, 0xD1BB67F1UL,
            0xA6BC5767UL, 0x3FB506DDUL, 0x48B2364BUL, 0xD80D2BDAUL, 0xAF0A1B4CUL,
            0x36034AF6UL, 0x41047A60UL, 0xDF60EFC3UL, 0xA867DF55UL, 0x316E8EEFUL,
            0x4669BE79UL, 0xCB61B38CUL, 0xBC66831AUL, 0x256FD2A0UL, 0x5268E236UL,
            0xCC0C7795UL, 0xBB0B4703UL, 0x220216B9UL, 0x5505262FUL, 0xC5BA3BBEUL,
            0xB2BD0B28UL, 0x2BB45A92UL, 0x5CB36A04UL, 0xC2D7FFA7UL, 0xB5D0CF31UL,
            0x2CD99E8BUL, 0x5BDEAE1DUL, 0x9B64C2B0UL, 0xEC63F226UL, 0x756AA39CUL,
            0x026D930AUL, 0x9C0906A9UL, 0xEB0E363FUL, 0x72076785UL, 0x05005713UL,
            0x95BF4A82UL, 0xE2B87A14UL, 0x7BB12BAEUL, 0x0CB61B38UL, 0x92D28E9BUL,
            0xE5D5BE0DUL, 0x7CDCEFB7UL, 0x0BDBDF21UL, 0x86D3D2D4UL, 0xF1D4E242UL,
            0x68DDB3F8UL, 0x1FDA836EUL, 0x81BE16CDUL, 0xF6B9265BUL, 0x6FB077E1UL,
            0x18B74777UL, 0x88085AE6UL, 0xFF0F6A70UL, 0x66063BCAUL, 0x11010B5CUL,
            0x8F659EFFUL, 0xF862AE69UL, 0x616BFFD3UL, 0x166CCF45UL, 0xA00AE278UL,
            0xD70DD2EEUL, 0x4E048354UL, 0x3903B3C2UL, 0xA7672661UL, 0xD06016F7UL,
            0x4969474DUL, 0x3E6E77DBUL, 0xAED16A4AUL, 0xD9D65ADCUL, 0x40DF0B66UL,
            0x37D83BF0UL, 0xA9BCAE53UL, 0xDEBB9EC5UL, 0x47B2CF7FUL, 0x30B5FFE9UL,
            0xBDBDF21CUL, 0xCABAC28AUL, 0x53B39330UL, 0x24B4A3A6UL, 0xBAD03605UL,
            0xCDD70693UL, 0x54DE5729UL, 0x23D967BFUL, 0xB3667A2EUL, 0xC4614AB8UL,
            0x5D681B02UL, 0x2A6F2B94UL, 0xB40BBE37UL, 0xC30C8EA1UL, 0x5A05DF1BUL,
            0x2D02EF8DUL
        };
    }

    tinf::error_t tinflate(
        const std::vector<uint8_t> &compressed,
        std::deque<uint8_t> &output,
        uint32_t *crc
    )
    {
        tinf::decompression_state_t state;
        return tinflate(compressed.cbegin(), compressed.cend(), output, state, crc);
    }

    tinf::error_t tinflate(
        typename std::vector<uint8_t>::const_iterator begin,
        typename std::vector<uint8_t>::const_iterator end,
        std::deque<uint8_t> &output,
        uint32_t *crc
    )
    {
        tinf::decompression_state_t state;
        return tinflate(begin, end, output, state, crc);
    }

    tinf::error_t tinflate(
        const std::vector<uint8_t> &compressed,
        std::deque<uint8_t> &output,
        tinf::decompression_state_t &state,
        uint32_t *crc
    )
    {
        return tinflate(compressed.begin(), compressed.end(), output, state, crc);
    }

    tinf::error_t tinflate(
        typename std::vector<uint8_t>::const_iterator begin,
        typename std::vector<uint8_t>::const_iterator end,
        std::deque<uint8_t> &output,
        tinf::decompression_state_t &state,
        uint32_t *crc
    )
    {
        using namespace tinf;
        state.begin = begin;
        state.end = end;
        const ptrdiff_t size = end - begin;
        if (size <= 0) return error_t::OUT_OF_DATA;

        if (state.state == tinf::state_t::INITIAL ||
            state.state == tinf::state_t::PARTIAL_ZLIB_HEADER)
        {
            uint16_t zlibHeader;
            if (size == 0)
            {
                return tinf::error_t::OUT_OF_DATA;
            }

            if (state.state == tinf::state_t::INITIAL && size == 1)
            {
                state.firstByte = *state.begin;
                state.state = tinf::state_t::PARTIAL_ZLIB_HEADER;
                return tinf::error_t::OUT_OF_DATA;
            }

            if (state.state == tinf::state_t::PARTIAL_ZLIB_HEADER)
            {
                zlibHeader = (state.firstByte << 8) | *state.begin;
            }
            else
            {
                zlibHeader = ((*state.begin) << 8) | *(state.begin + 1);
            }

            if ((zlibHeader & 0x8F00) == 0x0800 && (zlibHeader % 31) == 0)
            {
                if (zlibHeader & 0x0020)
                    return tinf::error_t::CUSTOM_DICTIONARY;

                state.begin += (state.state == tinf::state_t::PARTIAL_ZLIB_HEADER ? 1 : 2);
                if (state.begin >= state.end)
                    return tinf::error_t::OUT_OF_DATA;
            }
            else if (state.state == tinf::state_t::PARTIAL_ZLIB_HEADER)
            {
                state.bitAccum = state.firstByte;
                state.numBits = 8;
            }
            state.state = tinf::state_t::HEADER;
        }

        do {
            tinf::error_t res = tinflate_block(output, state);
            if (res != tinf::error_t::OK)
                return res;
        } while (!state.final);

        if (crc)
            *crc = state.crc;

        return tinf::error_t::OK;
    }

    tinf::error_t tinflate_block(std::deque<uint8_t> &output, tinf::decompression_state_t &state)
    {
        uint32_t icrc = ~state.crc;

        auto push = [&output, &icrc](const uint8_t val)
        {
            output.push_back(val);
            icrc = tinf::crc32_table[(icrc & 0xFF) ^ val] ^ ((icrc >> 8) & 0xFFFFFFUL);
        };

        switch (state.state)
        {
            #define JUMP_STATE(s) case tinf::state_t::s: goto state_##s
            JUMP_STATE(HEADER);
            JUMP_STATE(UNCOMPRESSED_LEN);
            JUMP_STATE(UNCOMPRESSED_ILEN);
            JUMP_STATE(UNCOMPRESSED_DATA);
            JUMP_STATE(LITERAL_COUNT);
            JUMP_STATE(DISTANCE_COUNT);
            JUMP_STATE(CODELEN_COUNT);
            JUMP_STATE(READ_CODE_LENGTHS);
            JUMP_STATE(READ_LENGTHS);
            JUMP_STATE(READ_LENGTHS_16);
            JUMP_STATE(READ_LENGTHS_17);
            JUMP_STATE(READ_LENGTHS_18);
            JUMP_STATE(READ_SYMBOL);
            JUMP_STATE(READ_LENGTH);
            JUMP_STATE(READ_DISTANCE);
            JUMP_STATE(READ_DISTANCE_EXTRA);
            #undef JUMP_STATE

            #define STATE(s) state.state = tinf::state_t::s; state_##s
            state_HEADER:
                if(state.get_bits(3, state.blockType)) goto out_of_data;
                state.final = state.blockType & 1;
                state.blockType >>= 1;

                if (state.blockType == 3)
                {
                    state.crc = ~icrc & 0xFFFFFFFFUL;
                    return tinf::error_t::INVALID_BLOCK_CODE;
                }

                if (state.blockType == 0)
                {
                    state.numBits = 0;

            STATE(UNCOMPRESSED_LEN):
                    if (state.get_bits(16, state.len)) goto out_of_data;
            STATE(UNCOMPRESSED_ILEN):
                    if (state.get_bits(16, state.ilen)) goto out_of_data;

                    if (state.ilen != (~state.len & 0xFFFF))
                    {
                        state.crc = ~icrc & 0xFFFFFFFFUL;
                        return tinf::error_t::CORRUPT_STREAM;
                    }
                    state.nread = 0;
            STATE(UNCOMPRESSED_DATA):
                    while (state.nread < state.len)
                    {
                        if (state.begin >= state.end) goto out_of_data;
                        push(*state.begin++);
                        ++state.nread;
                    }
                    state.crc = ~icrc & 0xFFFFFFFFUL;
                    state.state = tinf::state_t::HEADER;
                    return tinf::error_t::OK;
                }

                if (state.blockType == 2)
                {
                    static const uint8_t codelenOrder[19] = {
                        16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
                    };

            STATE(LITERAL_COUNT):
                    if (state.get_bits(5, state.literalCount)) goto out_of_data;
                    state.literalCount += 257;
            STATE(DISTANCE_COUNT):
                    if (state.get_bits(5, state.distanceCount)) goto out_of_data;
                    state.distanceCount += 1;
            STATE(CODELEN_COUNT):
                    if (state.get_bits(4, state.codelenCount)) goto out_of_data;
                    state.codelenCount += 4;
                    state.counter = 0;
            STATE(READ_CODE_LENGTHS):
                    for (;state.counter < state.codelenCount; ++state.counter)
                        if (state.get_bits(3, state.codelenLen[codelenOrder[state.counter]]))
                            goto out_of_data;

                    for (; state.counter < 19; ++state.counter)
                        state.codelenLen[codelenOrder[state.counter]] = 0;

                    if (gen_huffman_table(19, state.codelenLen, false, state.codelenTable) != tinf::error_t::OK)
                    {
                        state.crc = ~icrc & 0xFFFFFFFFUL;
                        return tinf::error_t::HUFFMAN_TABLE_GEN_FAILED;
                    }

                    {
                        uint32_t repeatCount;

                        state.lastValue = 0;
                        state.counter = 0;
            STATE(READ_LENGTHS):
                        repeatCount = 0;
                        while (state.counter < state.literalCount + state.distanceCount)
                        {
                            if (repeatCount == 0)
                            {
                                if (state.get_huff(state.symbol, state.codelenTable)) goto out_of_data;

                                if (state.symbol < 16)
                                {
                                    state.lastValue = state.symbol;
                                    repeatCount = 1;
                                }
                                else if (state.symbol == 16)
                                {
            STATE(READ_LENGTHS_16):
                                    if (state.get_bits(2, repeatCount)) goto out_of_data;
                                    repeatCount += 3;
                                }
                                else if (state.symbol == 17)
                                {
                                    state.lastValue = 0;
            STATE(READ_LENGTHS_17):
                                    if (state.get_bits(3, repeatCount)) goto out_of_data;
                                    repeatCount += 3;
                                }
                                else if (state.symbol)
                                {
                                    state.lastValue = 0;
            STATE(READ_LENGTHS_18):
                                    if (state.get_bits(7, repeatCount)) goto out_of_data;
                                    repeatCount += 11;
                                }
                            }

                            if (state.counter < state.literalCount)
                                state.literalLen[state.counter] = (uint8_t)state.lastValue;
                            else
                                state.distanceLen[state.counter - state.literalCount] = (uint8_t)state.lastValue;

                            ++state.counter;
                            --repeatCount;
                            state.state = tinf::state_t::READ_LENGTHS;
                        }
                    }

                    if (gen_huffman_table(state.literalCount, state.literalLen,
                            false, state.literalTable) !=tinf::error_t::OK ||
                        gen_huffman_table(state.distanceCount, state.distanceLen,
                            true, state.distanceTable) !=tinf::error_t::OK)
                    {
                        state.crc = ~icrc & 0xFFFFFFFFUL;
                        return tinf::error_t::HUFFMAN_TABLE_GEN_FAILED;
                    }

                }
                else
                {
                    int32_t nextFree = 2;
                    int32_t i;

                    for (i = 0; i < 0x7E; ++i)
                    {
                        state.literalTable[i] = ~nextFree;
                        nextFree += 2;
                    }

                    for (; i < 0x96; ++i)
                        state.literalTable[i] = (uint16_t)i + (256 - 0x7E);

                    for (; i < 0xFE; ++i)
                    {
                        state.literalTable[i] = ~nextFree;
                        nextFree += 2;
                    }

                    for (; i < 0x18E; ++i)
                        state.literalTable[i] = (uint16_t)i + (0 - 0xFE);

                    for (; i < 0x196; ++i)
                        state.literalTable[i] = (uint16_t)i + (280 - 0x18E);

                    for (; i < 0x1CE; ++i)
                    {
                        state.literalTable[i] = ~nextFree;
                        nextFree += 2;
                    }

                    for (; i < 0x23E; ++i)
                        state.literalTable[i] = (uint16_t)i + (144 - 0x1CE);

                    for (i = 0; i < 0x1E; ++i)
                        state.distanceTable[i] = ~(i * 2 + 2);

                    for (i = 0x1E; i < 0x3E; ++i)
                        state.distanceTable[i] = i - 0x1E;
                }

                for (;;)
                {
                    uint32_t distance;
            STATE(READ_SYMBOL):
                    if (state.get_huff(state.symbol, state.literalTable)) goto out_of_data;

                    if (state.symbol < 256)
                    {
                        push((const uint8_t)state.symbol);
                        continue;
                    }

                    if (state.symbol == 256)
                        break;

                    if (state.symbol <= 264)
                    {
                        state.repeatLength = (state.symbol - 257) + 3;
                    }
                    else if (state.symbol <= 284)
                    {
            STATE(READ_LENGTH):
                        {
                            const uint32_t lengthBits = (state.symbol - 261) / 4;
                            if (state.get_bits(lengthBits, state.repeatLength)) goto out_of_data;
                            state.repeatLength += 3 + ((4 + ((state.symbol - 265) & 3 )) << lengthBits);
                        }
                    }
                    else if (state.symbol == 285)
                    {
                        state.repeatLength = 258;
                    }
                    else
                    {
                        state.crc = ~icrc & 0xFFFFFFFFUL;
                        return tinf::error_t::INVALID_SYMBOL;
                    }

            STATE(READ_DISTANCE):
                    if (state.get_huff(state.symbol, state.distanceTable)) goto out_of_data;

                    if (state.symbol <= 3)
                    {
                        distance = state.symbol + 1;
                    }
                    else if (state.symbol <= 29)
                    {
            STATE(READ_DISTANCE_EXTRA):
                        {
                            const uint32_t distanceBits = (state.symbol - 2) / 2;
                            if (state.get_bits(distanceBits, distance)) goto out_of_data;
                            distance += 1 + ((2 + (state.symbol & 1)) << distanceBits);
                        }
                    }
                    else
                    {
                        state.crc = ~icrc & 0xFFFFFFFFUL;
                        return tinf::error_t::INVALID_SYMBOL;
                    }

                    if (distance > output.size())
                    {
                        state.crc = ~icrc & 0xFFFFFFFFUL;
                        return tinf::error_t::INVALID_DISTANCE;
                    }

                    while (state.repeatLength --> 0)
                        push(output[output.size() - distance]);
                    state.repeatLength = 0;
                }
                break;

            case tinf::state_t::INITIAL:
            case tinf::state_t::PARTIAL_ZLIB_HEADER:
            default:
                return tinf::error_t::INVALID_STATE;

            #undef STATE
        }

        state.crc = ~icrc & 0xFFFFFFFFUL;
        state.state = tinf::state_t::HEADER;
        return tinf::error_t::OK;

    out_of_data:
        state.crc = ~icrc & 0xFFFFFFFFUL;
        return tinf::error_t::OUT_OF_DATA;
    }

    tinf::error_t gen_huffman_table(
        uint32_t symbols,
        const uint8_t *lengths,
        bool allowNoSymbols,
        int16_t *table
    )
    {
        uint16_t lengthCount[16] = {0};
        uint16_t totalCount = 0;
        uint16_t firstCode[16];

        for (uint32_t i = 0; i < symbols; ++i)
            if (lengths[i] > 0)
                ++lengthCount[lengths[i]];

        for (uint32_t i = 1; i < 16; ++i)
            totalCount += lengthCount[i];

        if (totalCount == 0)
        {
            return allowNoSymbols ? tinf::error_t::OK : tinf::error_t::NO_SYMBOLS;
        }
        else if (totalCount == 1)
        {
            for (uint32_t i = 0; i < symbols; ++i)
                if (lengths[i] != 0)
                    table[0] = table[1] = (uint16_t)i;
            return tinf::error_t::OK;
        }

        firstCode[0] = 0;
        for (uint32_t i = 1; i < 16; ++i)
        {
            firstCode[i] = (firstCode[i-1] + lengthCount[i-1]) << 1;
            if (firstCode[i] + lengthCount[i] > (uint16_t)1 << i)
                return tinf::error_t::TOO_MANY_SYMBOLS;
        }
        if (firstCode[15] + lengthCount[15] != (uint16_t)1 << 15)
            return tinf::error_t::INCOMPLETE_TREE;

        for (uint32_t index = 0, i = 1; i < 16; ++i)
        {
            uint32_t codeLimit = 1U << i;
            uint32_t nextCode = firstCode[i] + lengthCount[i];
            uint32_t nextIndex = index + (codeLimit - firstCode[i]);

            for (uint32_t j = 0; j < symbols; ++j)
                if (lengths[j] == i)
                    table[index++] = (uint16_t)j;

            for (uint32_t j = nextCode; j < codeLimit; ++j)
            {
                table[index++] = ~nextIndex;
                nextIndex += 2;
            }
        }

        return tinf::error_t::OK;
    }
}