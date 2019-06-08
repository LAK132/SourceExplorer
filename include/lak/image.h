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

#include <vector>

#include <lak/vec.h>

#ifndef LAK_IMAGE_H
#define LAK_IMAGE_H

namespace lak
{
    template<typename T>
    struct image
    {
        using size_type = vec2s_t;
        using value_type = T;

        value_type &operator[](const size_type index)
        {
            return _value[index.x + (_size.x * index.y)];
        }

        const value_type &operator[](const size_type index) const
        {
            return _value[index.x + (_size.x * index.y)];
        }

        value_type &operator[](const size_t index)
        {
            return _value[index];
        }

        const value_type &operator[](const size_t index) const
        {
            return _value[index];
        }

        void resize(const size_type sz)
        {
            _size = sz;
            _value.resize(sz.x * sz.y);
        }

        size_type size() const
        {
            return _size;
        }

        size_t contig_size() const
        {
            return _value.size();
        }

    private:
        size_type _size;
        std::vector<value_type> _value;
    };

    using image3_t = image<color3_t>;
    using image4_t = image<color4_t>;

    image3_t ImageFromRGB24(uint8_t *pixels, vec2s_t size)
    {
        image3_t result;
        result.resize(size);
        std::memcpy(&result[0].x, pixels, size.x * size.y * 3);
        return result;
    }

    image4_t ImageFromRGBA32(uint8_t *pixels, vec2s_t size)
    {
        image4_t result;
        result.resize(size);
        std::memcpy(&result[0].x, pixels, size.x * size.y * 4);
        return result;
    }
}

#endif // LAK_IMAGE_H