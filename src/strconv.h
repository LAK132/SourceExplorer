/*
MIT License

Copyright (c) 2019 Lucas Kleiss (LAK132)

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <string>
#include <type_traits>

#ifndef STRCONV_H
#define STRCONV_H

// char8_t typdef for C++ < 20
#if __cplusplus <= 201703L
namespace lak {
    typedef uint_least8_t char8_t;
}
namespace std {
    typedef basic_string<lak::char8_t> u8string;
}
#endif

namespace lak
{
    enum encoding_t
    {
        UCS = 0,
        UTF,
        WTF,
        CESU
    };

    template<typename T>
    struct code_stream
    {
        const std::basic_string<T> &mem;
        size_t index = 0;
        code_stream(const std::basic_string<T> &str) : mem(str), index(0) {}
        char32_t getNext(encoding_t encoding);
    };

    // TODO: implement an iterator class

    using ascii_code_stream_t = code_stream<char>;
    using wide_code_stream_t = code_stream<wchar_t>;

    using utf8_code_stream_t = code_stream<char8_t>;
    using utf16_code_stream_t = code_stream<char16_t>;
    using utf32_code_stream_t = code_stream<char32_t>;

    template<>
    char32_t ascii_code_stream_t::getNext(encoding_t encoding)
    {
        return mem[index++];
    }

    template<>
    char32_t utf8_code_stream_t::getNext(encoding_t encoding)
    {
        char32_t code;
        const char8_t c = mem[index++];
        if (c >= 0x00 && c <= 0x7F)
        {
            code = c;
        }
        else if (c >= 0xC0 && c <= 0xDF)
        {
            code = ((c - 0xC0) << 6) | (mem[index++] - 0x80);
        }
        else if (c >= 0xE0 && c <= 0xEF)
        {
            code = (c - 0xE0) << 12;
            code |= (mem[index++] - 0x80) << 6;
            code |= mem[index++] - 0x80;
        }
        else if (c >= 0xF0 && c <= 0xF7)
        {
            code = (c - 0xF0) << 18;
            code |= (mem[index++] - 0x80) << 12;
            code |= (mem[index++] - 0x80) << 6;
            code |= mem[index++] - 0x80;
        }
        return code;
    }

    template<>
    char32_t utf16_code_stream_t::getNext(encoding_t encoding)
    {
        char32_t code;
        const char16_t c = mem[index++];
        if ((c >= 0x0000 && c <= 0xD7FF) || (c >= 0xE000 && c <= 0xFFFF))
        {
            code = c;
        }
        else
        {
            code = 0x010000 | (0xD800 + (c << 10));
            code |= 0xDC00 + mem[index++];
        }
        return code;
    }

    template<>
    char32_t utf32_code_stream_t::getNext(encoding_t encoding)
    {
        return mem[index++];
    }

    template<>
    char32_t wide_code_stream_t::getNext(encoding_t encoding)
    {
        return mem[index++];
    }

    inline void append_code(std::string &str, const char32_t code, const encoding_t encoding = UCS)
    {
        switch (encoding)
        {
            case encoding_t::UCS:
            default: {
                if (code <= 0x7F)
                    str += (char8_t)code;
            } break;
        }
    }

    inline void append_code(std::wstring &str, const char32_t code, const encoding_t encoding = UCS)
    {
        switch (encoding)
        {
            case encoding_t::UCS:
            default: {
                if (code < 256 * sizeof(wchar_t))
                    str += (wchar_t)code;
            } break;
        }
    }

    inline void append_code(std::u8string &str, const char32_t code, const encoding_t encoding = UTF)
    {
        switch (encoding)
        {
            case encoding_t::UTF:
            default: {
                if (code <= 0x7F)
                {
                    str += (char8_t)code;
                }
                else if (code >= 0x80 && code <= 0x7FF)
                {
                    str += 0xC0 + (char8_t)(code >> 6);
                    str += 0x80 + (char8_t)(code & 0x3F);
                }
                else if (code >= 0x800 && code <= 0xFFFF)
                {
                    str += 0xE0 + (char8_t)(code >> 12);
                    str += 0x80 + (char8_t)((code >> 6) & 0x3F);
                    str += 0x80 + (char8_t)(code & 0x3F);
                }
                else if (code >= 0x10000 && code <= 0x10FFFF)
                {
                    str += 0xF0 + (char8_t)(code >> 18);
                    str += 0x80 + (char8_t)((code >> 12) & 0x3F);
                    str += 0x80 + (char8_t)((code >> 6) & 0x3F);
                    str += 0x80 + (char8_t)(code & 0x3F);
                }
            } break;
        }
    }

    inline void append_code(std::u16string &str, char32_t code, const encoding_t encoding = UTF)
    {
        switch (encoding)
        {
            case encoding_t::UTF:
            default: {
                if (code <= 0xFFFF)
                {
                    str += (char16_t)code;
                }
                else
                {
                    code -= 0x01000;
                    str += (char16_t)(0xD800 + ((code >> 10) & 0x3FF));
                    str += (char16_t)(0xDC00 + (code & 0x3FF));
                }
            } break;
        }
    }

    inline void append_code(std::u32string &str, const char32_t code, const encoding_t encoding = UTF)
    {
        switch (encoding)
        {
            case encoding_t::UTF:
            default: {
                str += code;
            } break;
        }
    }

    template<typename TO, typename FROM, encoding_t TENC = UTF, encoding_t FENC = UTF>
    std::basic_string<TO> strconv(const std::basic_string<FROM> &str)
    {
        std::basic_string<TO> result;
        code_stream<FROM> strm(str);

        for (char32_t code = strm.getNext(FENC); code != 0; code = strm.getNext(FENC))
            append_code(result, code, TENC);

        return result;
    }
}

#endif