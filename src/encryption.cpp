#include "encryption.h"

bool GenerateTable(decode_buffer_t &decodeBuffer, const std::vector<uint8_t> &magic_key,
    const m128i_t &xmmword, const char magic_char)
{
    __m128i *bufferPtr = decodeBuffer._m128i; // edx@1
    __m128i offset = _mm_load_si128(&xmmword.m128i); // xmm1@1
    for (int i = 0; i < 64; ++i) // eax@1
    {
        _mm_storeu_si128(bufferPtr, _mm_add_epi32(_mm_shuffle_epi32(_mm_cvtsi32_si128(i * 4), 0), offset));
        bufferPtr++;
    }

    uint8_t accum = magic_char; // ch@1
    uint8_t hash = magic_char; // cl@1

    bool v15 = true; // v11 eax@4
    bool rtn = false; // ebx@1

    uint8_t i2 = 0;
    auto key = magic_key.begin();
    for (uint32_t i = 0; i < 256; ++i, ++key) // v16
    {
        hash = (hash << 7) + (hash >> 1);
        if (v15)
            accum += ((hash & 1) | 2) * *key;

        if (hash == *key)
        {
            if (v15)
                rtn = (accum == *(key + 1));

            if (!rtn) break;

            hash = (magic_char >> 1) + (magic_char << 7);
            key = magic_key.begin();
            v15 = false;
        }

        i2 += (hash ^ *key) + decodeBuffer.m128i_i32[i];

        std::swap(decodeBuffer.m128i_i32[i], decodeBuffer.m128i_i32[i2]);
    }
    return rtn;
}

void DecodeWithTable(std::vector<uint8_t> &chunk, decode_buffer_t &decodeBuffer)
{
    uint8_t i = 0;
    uint8_t i2 = 0;
    for (uint8_t &elem : chunk)
    {
        ++i; i2 += (uint8_t)decodeBuffer.m128i_i32[i];

        std::swap(decodeBuffer.m128i_i32[i], decodeBuffer.m128i_i32[i2]);

        elem ^= decodeBuffer.m128i_u8[4 * (uint8_t)(decodeBuffer.m128i_i32[i2] + decodeBuffer.m128i_i32[i])];
    }
}

bool DecodeChunk(std::vector<uint8_t> &chunk, const std::vector<uint8_t> &magic_key,
    const m128i_t &xmmword, const char magic_char)
{
    decode_buffer_t decodeBuffer;
    if(magic_key.size() < 256) ERROR("Magic Key Too Small: 0x" << magic_key.size());
    if (GenerateTable(decodeBuffer, magic_key, xmmword, magic_char))
    {
        DecodeWithTable(chunk, decodeBuffer);
        return true;
    }
    DEBUG("Failed To Generate Decode Table");
    return false;
}

std::vector<uint8_t> KeyString(const std::u16string &str)
{
    std::vector<uint8_t> result;
    result.reserve(str.size() * 2);
    for (const char16_t code : str)
    {
        if (code & 0xFF)
            result.emplace_back(code & 0xFF);
        if ((code >> 8) & 0xFF)
            result.emplace_back((code >> 8) & 0xFF);
    }
    return result;
}
