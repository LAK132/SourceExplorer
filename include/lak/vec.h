/*
MIT License

Copyright (c) 2019 LAK132

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

#ifndef LAK_VEC_H
#define LAK_VEC_H

#ifdef __has_include
#   if __has_include(<imgui.h>)
#       include <imgui.h>
#       define LAK_VEC_H_HAS_IMGUI
#   elif __has_include("imgui.h")
#       include "imgui.h"
#       define LAK_VEC_H_HAS_IMGUI
#   elif __has_include(<imgui/imgui.h>)
#       include <imgui/imgui.h>
#       define LAK_VEC_H_HAS_IMGUI
#   elif __has_include("imgui/imgui.h")
#       include "imgui/imgui.h"
#       define LAK_VEC_H_HAS_IMGUI
#   endif

#   if __has_include(<glm/vec2.hpp>) && \
       __has_include(<glm/vec3.hpp>) && \
       __has_include(<glm/vec4.hpp>)
#       include <glm/vec2.hpp>
#       include <glm/vec3.hpp>
#       include <glm/vec4.hpp>
#       define LAK_VEC_H_HAS_GLM
#   endif
#endif

#include <cstdint>
#include <cstddef>

namespace lak
{
    template<typename T> struct vec2;
    template<typename T> struct vec3;
    template<typename T> struct vec4;

    template<typename T>
    struct vec2
    {
        union { T x; T r; };
        union { T y; T g; };

        vec2()
        : x(0), y(0) {}

        vec2(const vec2 &other)
        : x(other.x), y(other.y) {}

        vec2(const T (&values)[2])
        : x(values[0]), y(values[1]) {}

        vec2(const T X, const T Y)
        : x(X), y(Y) {}

        vec2 &operator=(const vec2 &rhs)
        { x = (T)rhs.x; y = (T)rhs.y; return *this; }

        inline T &operator[](const size_t index)
        { return (&x)[index]; }

        inline const T &operator[](const size_t index) const
        { return (&x)[index]; }

        template<typename L> explicit operator vec2<L>() const
        { return vec2<L>{static_cast<L>(x), static_cast<L>(y)}; }

        template<typename L> explicit operator vec3<L>() const
        { return vec3<L>{static_cast<L>(x), static_cast<L>(y), 0}; }

        template<typename L> explicit operator vec4<L>() const
        { return vec4<L>{static_cast<L>(x), static_cast<L>(y), 0, 0}; }

        #ifdef LAK_VEC_H_HAS_IMGUI
        explicit operator ImVec2() const
        { return ImVec2(static_cast<float>(x), static_cast<float>(y)); }

        explicit operator ImVec4() const
        { return ImVec4(static_cast<float>(x), static_cast<float>(y), 0.0f, 0.0f); }
        #endif

        #ifdef LAK_VEC_H_HAS_GLM
        template<glm::precision GLMP> vec2(const glm::tvec2<T, GLMP> &vec2)
        { x = vec2.x; y = vec2.y; }

        template<glm::precision GLMP> vec2(const glm::tvec3<T, GLMP> &vec3)
        { x = vec3.x; y = vec3.y; }

        template<glm::precision GLMP> vec2(const glm::tvec4<T, GLMP> &vec4)
        { x = vec4.x; y = vec4.y; }

        template<glm::precision GLMP> operator glm::tvec2<T, GLMP>() const
        { return {x, y}; }

        template<glm::precision GLMP> operator glm::tvec3<T, GLMP>() const
        { return {x, y, 0}; }

        template<glm::precision GLMP> operator glm::tvec4<T, GLMP>() const
        { return {x, y, 0, 0}; }

        template<typename GLMT, glm::precision GLMP>
        explicit vec2(const glm::tvec2<GLMT, GLMP> &vec2)
        { x = static_cast<T>(vec2.x); y = static_cast<T>(vec2.y); }

        template<typename GLMT, glm::precision GLMP>
        explicit vec2(const glm::tvec3<GLMT, GLMP> &vec3)
        { x = static_cast<T>(vec3.x); y = static_cast<T>(vec3.y); }

        template<typename GLMT, glm::precision GLMP>
        explicit vec2(const glm::tvec4<GLMT, GLMP> &vec4)
        { x = static_cast<T>(vec4.x); y = static_cast<T>(vec4.y); }

        template<typename GLMT, glm::precision GLMP>
        explicit operator glm::tvec4<GLMT, GLMP>() const
        { return {static_cast<GLMT>(x), static_cast<GLMT>(y), GLMT(0), GLMT(0)}; }

        template<typename GLMT, glm::precision GLMP>
        explicit operator glm::tvec3<GLMT, GLMP>() const
        { return {static_cast<GLMT>(x), static_cast<GLMT>(y), GLMT(0)}; }

        template<typename GLMT, glm::precision GLMP>
        explicit operator glm::tvec2<GLMT, GLMP>() const
        { return {static_cast<GLMT>(x), static_cast<GLMT>(y)}; }
        #endif
    };

    using vec2f_t = vec2<float>;
    using vec2i_t = vec2<int>;
    using vec2s_t = vec2<size_t>;
    using vec2i8_t = vec2<int8_t>;
    using vec2u8_t = vec2<uint8_t>;
    using color2_t = vec2<uint8_t>;
    using vec2i16_t = vec2<int16_t>;
    using vec2u16_t = vec2<uint16_t>;
    using vec2i32_t = vec2<int32_t>;
    using vec2u32_t = vec2<uint32_t>;
    using vec2i64_t = vec2<int64_t>;
    using vec2u64_t = vec2<uint64_t>;

    template<typename T>
    struct vec3
    {
        union { T x; T r; };
        union { T y; T g; };
        union { T z; T b; };

        vec3()
        : x(0), y(0), z(0) {}

        vec3(const vec3 &other)
        : x(other.x), y(other.y), z(other.z) {}

        vec3(const T (&values)[3])
        : x(values[0]), y(values[1]), z(values[2]) {}

        vec3(const T X, const T Y, const T Z)
        : x(X), y(Y), z(Z) {}

        vec3 &operator=(const vec3 &rhs)
        { x = rhs.x; y = rhs.y; z = rhs.z; return *this; }

        inline T &operator[](const size_t index)
        { return (&x)[index]; }

        inline const T &operator[](const size_t index) const
        { return (&x)[index]; }

        template<typename L> explicit operator vec2<L>() const
        { return vec2<L>{static_cast<L>(x), static_cast<L>(y)}; }

        template<typename L> explicit operator vec3<L>() const
        { return vec3<L>{static_cast<L>(x), static_cast<L>(y), static_cast<L>(z)}; }

        template<typename L> explicit operator vec4<L>() const
        { return vec4<L>{static_cast<L>(x), static_cast<L>(y), static_cast<L>(z), 0}; }

        #ifdef LAK_VEC_H_HAS_IMGUI
        operator ImVec2() const
        { return ImVec2(static_cast<float>(x), static_cast<float>(y)); }

        operator ImVec4() const
        { return ImVec4(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), 0.0f); }
        #endif

        #ifdef LAK_VEC_H_HAS_GLM
        template<glm::precision GLMP> vec3(const glm::tvec2<T, GLMP> &vec2)
        { x = vec2.x; y = vec2.y; z = 0; }

        template<glm::precision GLMP> vec3(const glm::tvec3<T, GLMP> &vec3)
        { x = vec3.x; y = vec3.y; z = vec3.z; }

        template<glm::precision GLMP> vec3(const glm::tvec4<T, GLMP> &vec4)
        { x = vec4.x; y = vec4.y; z = vec4.z; }

        template<glm::precision GLMP> operator glm::tvec2<T, GLMP>() const
        { return {x, y}; }

        template<glm::precision GLMP> operator glm::tvec3<T, GLMP>() const
        { return {x, y, z}; }

        template<glm::precision GLMP> operator glm::tvec4<T, GLMP>() const
        { return {x, y, z, 0}; }

        template<typename GLMT, glm::precision GLMP>
        explicit vec3(const glm::tvec2<GLMT, GLMP> &vec2)
        { x = static_cast<T>(vec2.x); y = static_cast<T>(vec2.y); z = 0; }

        template<typename GLMT, glm::precision GLMP>
        explicit vec3(const glm::tvec3<GLMT, GLMP> &vec3)
        { x = static_cast<T>(vec3.x); y = static_cast<T>(vec3.y); z = static_cast<T>(vec3.z); }

        template<typename GLMT, glm::precision GLMP>
        explicit vec3(const glm::tvec4<GLMT, GLMP> &vec4)
        { x = static_cast<T>(vec4.x); y = static_cast<T>(vec4.y); z = static_cast<T>(vec4.z); }

        template<typename GLMT, glm::precision GLMP>
        explicit operator glm::tvec4<GLMT, GLMP>() const
        { return {static_cast<GLMT>(x), static_cast<GLMT>(y), static_cast<GLMT>(z), GLMT(0)}; }

        template<typename GLMT, glm::precision GLMP>
        explicit operator glm::tvec3<GLMT, GLMP>() const
        { return {static_cast<GLMT>(x), static_cast<GLMT>(y), static_cast<GLMT>(z)}; }

        template<typename GLMT, glm::precision GLMP>
        explicit operator glm::tvec2<GLMT, GLMP>() const
        { return {static_cast<GLMT>(x), static_cast<GLMT>(y)}; }
        #endif
    };

    using vec3f_t = vec3<float>;
    using vec3i_t = vec3<int>;
    using vec3s_t = vec3<size_t>;
    using vec3i8_t = vec3<int8_t>;
    using vec3u8_t = vec3<uint8_t>;
    using color3_t = vec3<uint8_t>;
    using vec3i16_t = vec3<int16_t>;
    using vec3u16_t = vec3<uint16_t>;
    using vec3i32_t = vec3<int32_t>;
    using vec3u32_t = vec3<uint32_t>;
    using vec3i64_t = vec3<int64_t>;
    using vec3u64_t = vec3<uint64_t>;

    template<typename T>
    struct vec4
    {
        union { T x; T r; };
        union { T y; T g; };
        union { T z; T b; };
        union { T w; T a; };

        vec4()
        : x(0), y(0), z(0), w(0) {}

        vec4(const vec4 &other)
        : x(other.x), y(other.y), z(other.z), w(other.w) {}

        vec4(const T (&values)[4])
        : x(values[0]), y(values[1]), z(values[2]), w(values[3]) {}

        vec4(const T X, const T Y, const T Z, const T W)
        : x(X), y(Y), z(Z), w(W) {}

        vec4 &operator=(const vec4 &rhs)
        { x = rhs.x; y = rhs.y; z = rhs.z; w = rhs.w; return *this; }

        inline T &operator[](const size_t index)
        { return (&x)[index]; }

        inline const T &operator[](const size_t index) const
        { return (&x)[index]; }

        template<typename L> explicit operator vec2<L>() const
        { return vec2<L>{static_cast<L>(x), static_cast<L>(y)}; }

        template<typename L> explicit operator vec3<L>() const
        { return vec3<L>{static_cast<L>(x), static_cast<L>(y), static_cast<L>(z)}; }

        template<typename L> explicit operator vec4<L>() const
        { return vec4<L>{static_cast<L>(x), static_cast<L>(y), static_cast<L>(z), static_cast<L>(w)}; }

        #ifdef LAK_VEC_H_HAS_IMGUI
        operator ImVec2() const
        { return ImVec2(static_cast<float>(x), static_cast<float>(y)); }

        operator ImVec4() const
        { return ImVec4(static_cast<float>(x), static_cast<float>(y), static_cast<float>(z), static_cast<float>(w)); }
        #endif

        #ifdef LAK_VEC_H_HAS_GLM
        template<glm::precision GLMP> vec4(const glm::tvec2<T, GLMP> &vec2)
        { x = vec2.x; y = vec2.y; z = 0; w = 0; }

        template<glm::precision GLMP> vec4(const glm::tvec3<T, GLMP> &vec3)
        { x = vec3.x; y = vec3.y; z = vec3.z; w = 0; }

        template<glm::precision GLMP> vec4(const glm::tvec4<T, GLMP> &vec4)
        { x = vec4.x; y = vec4.y; z = vec4.z; w = vec4.w; }

        template<glm::precision GLMP> operator glm::tvec2<T, GLMP>() const
        { return {x, y}; }

        template<glm::precision GLMP> operator glm::tvec3<T, GLMP>() const
        { return {x, y, z}; }

        template<glm::precision GLMP> operator glm::tvec4<T, GLMP>() const
        { return {x, y, z, w}; }

        template<typename GLMT, glm::precision GLMP>
        explicit vec4(const glm::tvec2<GLMT, GLMP> &vec2)
        { x = static_cast<T>(vec2.x); y = static_cast<T>(vec2.y); z = 0; w = 0;}

        template<typename GLMT, glm::precision GLMP>
        explicit vec4(const glm::tvec3<GLMT, GLMP> &vec3)
        { x = static_cast<T>(vec3.x); y = static_cast<T>(vec3.y); z = static_cast<T>(vec3.z); w = 0; }

        template<typename GLMT, glm::precision GLMP>
        explicit vec4(const glm::tvec4<GLMT, GLMP> &vec4)
        { x = static_cast<T>(vec4.x); y = static_cast<T>(vec4.y); z = static_cast<T>(vec4.z); w = static_cast<T>(vec4.w); }

        template<typename GLMT, glm::precision GLMP>
        explicit operator glm::tvec4<GLMT, GLMP>() const
        { return {static_cast<GLMT>(x), static_cast<GLMT>(y), static_cast<GLMT>(z), static_cast<GLMT>(w)}; }

        template<typename GLMT, glm::precision GLMP>
        explicit operator glm::tvec3<GLMT, GLMP>() const
        { return {static_cast<GLMT>(x), static_cast<GLMT>(y), static_cast<GLMT>(z)}; }

        template<typename GLMT, glm::precision GLMP>
        explicit operator glm::tvec2<GLMT, GLMP>() const
        { return {static_cast<GLMT>(x), static_cast<GLMT>(y)}; }
        #endif
    };

    using vec4f_t = vec4<float>;
    using vec4i_t = vec4<int>;
    using vec4s_t = vec4<size_t>;
    using vec4i8_t = vec4<int8_t>;
    using vec4u8_t = vec4<uint8_t>;
    using color4_t = vec4<uint8_t>;
    using vec4i16_t = vec4<int16_t>;
    using vec4u16_t = vec4<uint16_t>;
    using vec4i32_t = vec4<int32_t>;
    using vec4u32_t = vec4<uint32_t>;
    using vec4i64_t = vec4<int64_t>;
    using vec4u64_t = vec4<uint64_t>;
}

// dot product

template<typename T>
T dot(const lak::vec2<T> &lhs, const lak::vec2<T> &rhs)
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y);
}

template<typename T>
T dot(const lak::vec3<T> &lhs, const lak::vec3<T> &rhs)
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z);
}

template<typename T>
T dot(const lak::vec4<T> &lhs, const lak::vec4<T> &rhs)
{
    return (lhs.x * rhs.x) + (lhs.y * rhs.y) + (lhs.z * rhs.z) + (lhs.w * rhs.w);
}

// hammard product

template<typename T>
lak::vec2<T> operator*(lak::vec2<T> lhs, const lak::vec2<T> &rhs)
{
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    return lhs;
}

template<typename T>
lak::vec2<T> operator*(lak::vec2<T> lhs, T rhs)
{
    lhs.x *= rhs;
    lhs.y *= rhs;
    return lhs;
}

template<typename T>
lak::vec2<T> operator*(T lhs, lak::vec2<T> rhs)
{
    rhs.x *= lhs;
    rhs.y *= lhs;
    return rhs;
}

template<typename T>
lak::vec3<T> operator*(lak::vec3<T> lhs, const lak::vec3<T> &rhs)
{
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    lhs.z *= rhs.z;
    return lhs;
}

template<typename T>
lak::vec3<T> operator*(lak::vec3<T> lhs, T rhs)
{
    lhs.x *= rhs;
    lhs.y *= rhs;
    lhs.z *= rhs;
    return lhs;
}

template<typename T>
lak::vec3<T> operator*(T lhs, lak::vec3<T> rhs)
{
    rhs.x *= lhs;
    rhs.y *= lhs;
    rhs.z *= lhs;
    return rhs;
}

template<typename T>
lak::vec4<T> operator*(lak::vec4<T> lhs, const lak::vec4<T> &rhs)
{
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    lhs.z *= rhs.z;
    lhs.w *= rhs.w;
    return lhs;
}

template<typename T>
lak::vec4<T> operator*(lak::vec4<T> lhs, T rhs)
{
    lhs.x *= rhs;
    lhs.y *= rhs;
    lhs.z *= rhs;
    lhs.w *= rhs;
    return lhs;
}

template<typename T>
lak::vec4<T> operator*(T lhs, lak::vec4<T> rhs)
{
    rhs.x *= lhs;
    rhs.y *= lhs;
    rhs.z *= lhs;
    rhs.w *= lhs;
    return rhs;
}

template<typename T>
lak::vec2<T> &operator*=(lak::vec2<T> &lhs, const lak::vec2<T> &rhs)
{
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    return lhs;
}

template<typename T>
lak::vec2<T> &operator*=(lak::vec2<T> &lhs, T rhs)
{
    lhs.x *= rhs;
    lhs.y *= rhs;
    return lhs;
}

template<typename T>
lak::vec3<T> &operator*=(lak::vec3<T> &lhs, const lak::vec3<T> &rhs)
{
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    lhs.z *= rhs.z;
    return lhs;
}

template<typename T>
lak::vec3<T> &operator*=(lak::vec3<T> &lhs, T rhs)
{
    lhs.x *= rhs;
    lhs.y *= rhs;
    lhs.z *= rhs;
    return lhs;
}

template<typename T>
lak::vec4<T> &operator*=(lak::vec4<T> &lhs, const lak::vec4<T> &rhs)
{
    lhs.x *= rhs.x;
    lhs.y *= rhs.y;
    lhs.z *= rhs.z;
    lhs.w *= rhs.w;
    return lhs;
}

template<typename T>
lak::vec4<T> &operator*=(lak::vec4<T> &lhs, T rhs)
{
    lhs.x *= rhs;
    lhs.y *= rhs;
    lhs.z *= rhs;
    lhs.w *= rhs;
    return lhs;
}

// division

template<typename T>
lak::vec2<T> operator/(lak::vec2<T> lhs, T rhs)
{
    lhs.x /= rhs;
    lhs.y /= rhs;
    return lhs;
}

template<typename T>
lak::vec3<T> operator/(lak::vec3<T> lhs, T rhs)
{
    lhs.x /= rhs;
    lhs.y /= rhs;
    lhs.z /= rhs;
    return lhs;
}

template<typename T>
lak::vec4<T> operator/(lak::vec4<T> lhs, T rhs)
{
    lhs.x /= rhs;
    lhs.y /= rhs;
    lhs.z /= rhs;
    lhs.w /= rhs;
    return lhs;
}

template<typename T>
lak::vec2<T> &operator/=(lak::vec2<T> &lhs, T rhs)
{
    lhs.x /= rhs;
    lhs.y /= rhs;
    return lhs;
}

template<typename T>
lak::vec3<T> &operator/=(lak::vec3<T> &lhs, T rhs)
{
    lhs.x /= rhs;
    lhs.y /= rhs;
    lhs.z /= rhs;
    return lhs;
}

template<typename T>
lak::vec4<T> &operator/=(lak::vec4<T> &lhs, T rhs)
{
    lhs.x /= rhs;
    lhs.y /= rhs;
    lhs.z /= rhs;
    lhs.w /= rhs;
    return lhs;
}

// addition

template<typename T>
lak::vec2<T> operator+(lak::vec2<T> lhs, const lak::vec2<T> &rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}

template<typename T>
lak::vec3<T> operator+(lak::vec3<T> lhs, const lak::vec3<T> &rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    return lhs;
}

template<typename T>
lak::vec4<T> operator+(lak::vec4<T> lhs, const lak::vec4<T> &rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    lhs.w += rhs.w;
    return lhs;
}

template<typename T>
lak::vec2<T> &operator+=(lak::vec2<T> &lhs, const lak::vec2<T> &rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    return lhs;
}

template<typename T>
lak::vec3<T> &operator+=(lak::vec3<T> &lhs, const lak::vec3<T> &rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    return lhs;
}

template<typename T>
lak::vec4<T> &operator+=(lak::vec4<T> &lhs, const lak::vec4<T> &rhs)
{
    lhs.x += rhs.x;
    lhs.y += rhs.y;
    lhs.z += rhs.z;
    lhs.w += rhs.w;
    return lhs;
}

// subtraction

template<typename T>
lak::vec2<T> operator-(lak::vec2<T> lhs, const lak::vec2<T> &rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}

template<typename T>
lak::vec3<T> operator-(lak::vec3<T> lhs, const lak::vec3<T> &rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;
    return lhs;
}

template<typename T>
lak::vec4<T> operator-(lak::vec4<T> lhs, const lak::vec4<T> &rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;
    lhs.w -= rhs.w;
    return lhs;
}

template<typename T>
lak::vec2<T> &operator-=(lak::vec2<T> &lhs, const lak::vec2<T> &rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    return lhs;
}

template<typename T>
lak::vec3<T> &operator-=(lak::vec3<T> &lhs, const lak::vec3<T> &rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;
    return lhs;
}

template<typename T>
lak::vec4<T> &operator-=(lak::vec4<T> &lhs, const lak::vec4<T> &rhs)
{
    lhs.x -= rhs.x;
    lhs.y -= rhs.y;
    lhs.z -= rhs.z;
    lhs.w -= rhs.w;
    return lhs;
}

template<typename T>
bool operator==(const lak::vec2<T> &lhs, const lak::vec2<T> &rhs)
{
    return (lhs.x == rhs.x) && (lhs.y == rhs.y);
}

template<typename T>
bool operator==(const lak::vec3<T> &lhs, const lak::vec3<T> &rhs)
{
    return (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z);
}

template<typename T>
bool operator==(const lak::vec4<T> &lhs, const lak::vec4<T> &rhs)
{
    return (lhs.x == rhs.x) && (lhs.y == rhs.y) && (lhs.z == rhs.z) && (lhs.w == rhs.w);
}

template<typename T>
bool operator!=(const lak::vec2<T> &lhs, const lak::vec2<T> &rhs)
{
    return !(lhs == rhs);
}

template<typename T>
bool operator!=(const lak::vec3<T> &lhs, const lak::vec3<T> &rhs)
{
    return !(lhs == rhs);
}

template<typename T>
bool operator!=(const lak::vec4<T> &lhs, const lak::vec4<T> &rhs)
{
    return !(lhs == rhs);
}

#endif // LAK_VEC_H