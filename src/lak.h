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

#include <assert.h>
#include <thread>
#include <vector>
#include <string>
#include <tuple>
#include <stdint.h>
#include <thread>
#include <atomic>
#include <iostream>
#include <fstream>
#include <filesystem>
namespace fs = std::filesystem;

#define SDL_MAIN_HANDLED
#include <SDL.h>
extern "C" {
#include <GL/gl3w.h>
}

#ifndef LAK_H
#define LAK_H

namespace lak
{
    float mod(const float A, const float B);

    template<typename T>
    struct vec2
    {
        union { T x; T r; };
        union { T y; T g; };
        inline T &operator[](const size_t index) { return (&x)[index]; }
        inline const T &operator[](const size_t index) const { return (&x)[index]; }
        template<typename R>
        vec2<T> &operator=(const vec2<R> &rhs) {
            x = (T)rhs.x;
            y = (T)rhs.y;
            return *this;
        }
        template<typename L>
        operator vec2<L>() { return vec2<L>{x, y}; }
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
        inline T &operator[](const size_t index) { return (&x)[index]; }
        inline const T &operator[](const size_t index) const { return (&x)[index]; }
        template<typename R>
        vec3<T> &operator=(const vec3<R> &rhs) {
            x = (T)rhs.x;
            y = (T)rhs.y;
            z = (T)rhs.z;
            return *this;
        }
        template<typename L>
        operator vec3<L>() { return vec3<L>{x, y, z}; }
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
        inline T &operator[](const size_t index) { return (&x)[index]; }
        inline const T &operator[](const size_t index) const { return (&x)[index]; }
        template<typename R>
        vec4<T> &operator=(const vec4<R> &rhs) {
            x = (T)rhs.x;
            y = (T)rhs.y;
            z = (T)rhs.z;
            w = (T)rhs.w;
            return *this;
        }
        template<typename L>
        operator vec4<L>() { return vec4<L>{x, y, z, w}; }
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

    struct memstrm_t
    {
        std::vector<uint8_t> memory = {};
        size_t position = 0;
        memstrm_t() {}
        memstrm_t(const std::vector<uint8_t> &mem) : memory(mem) {}
        memstrm_t(const memstrm_t &other) : memory(other.memory), position(other.position) {}
        memstrm_t(memstrm_t &&other) : memory(std::move(other.memory)), position(other.position) {}
        inline memstrm_t &operator=(const std::vector<uint8_t> &mem)
        {
            memory = mem;
            position = 0;
            return *this;
        }
        inline memstrm_t &operator=(const memstrm_t &other)
        {
            memory = other.memory;
            position = other.position;
            return *this;
        }
        inline memstrm_t &operator=(memstrm_t &&other)
        {
            memory = std::move(other.memory);
            position = other.position;
            return *this;
        }
        inline std::vector<uint8_t>::iterator begin()
        {
            return memory.begin();
        }
        inline std::vector<uint8_t>::const_iterator cbegin()
        {
            return memory.cbegin();
        }
        inline std::vector<uint8_t>::iterator end()
        {
            return memory.end();
        }
        inline std::vector<uint8_t>::const_iterator cend()
        {
            return memory.cend();
        }
        inline std::vector<uint8_t>::iterator cursor()
        {
            return memory.begin() + position;
        }
        inline size_t size() const
        {
            return memory.size();
        }
        inline size_t remaining() const
        {
            assert(position <= memory.size());
            return memory.size() - position;
        }
        inline memstrm_t &clear()
        {
            position = 0;
            memory.clear();
            return *this;
        }
        inline memstrm_t &seek(const size_t to)
        {
            position = to;
            return *this;
        }
        uint8_t *get()
        {
            assert(position < memory.size());
            return &(memory[position]);
        }
        vec4u8_t readRGBA()
        {
            vec4u8_t col;
            col.r = readInt<uint8_t>();
            col.g = readInt<uint8_t>();
            col.b = readInt<uint8_t>();
            col.a = readInt<uint8_t>();
            return col;
        }
        std::vector<uint8_t> readBytes(size_t count)
        {
            // assert(position < memory.size());
            if (position >= memory.size())
                return {};

            if (memory.size() < position + count)
                count = memory.size() - position;

            std::vector<uint8_t> result;
            result.assign(memory.begin() + position, memory.begin() + position + count);
            position += count;
            return result;
        }
        void writeBytes(const std::vector<uint8_t> &bytes)
        {
            if (memory.size() < position + bytes.size())
                memory.resize(position + bytes.size());

            for (const auto b : bytes)
                memory[position++] = b;
        }
        // Returns the position to where it was before this call
        std::vector<uint8_t> readBytesFrom(const size_t from, const size_t count)
        {
            size_t pos = position;
            position = from;
            auto result = readBytes(count);
            position = pos;
            return result;
        }
        // Returns the position to where it was before this call
        std::vector<uint8_t> readBytesRange(const size_t from, const size_t to)
        {
            assert(to > from);
            return readBytesFrom(from, to - from);
        }
        // Returns the position to where it was before this call
        std::vector<uint8_t> readBytesToCursor(const size_t from)
        {
            assert(position >= from);
            size_t count = position - from;
            position = from;
            return readBytes(count);
        }
        template<typename T>
        memstrm_t &readObj(T &obj)
        {
            obj = *reinterpret_cast<T*>(&(readBytes(sizeof(T))[0]));
            return *this;
        }
        template<typename T>
        T peekInt()
        {
            if (memory.size() < position + sizeof(T))
                return T{};

            return *reinterpret_cast<const T*>(&(memory[position]));
        }
        template<typename T>
        T readInt()
        {
            if (memory.size() < position + sizeof(T))
                return T{};

            T result = *reinterpret_cast<const T*>(&(memory[position]));
            position += sizeof(T);
            return result;
        }
        template<typename T>
        void writeInt(const T value)
        {
            if (memory.size() < position + sizeof(T))
                memory.resize(position + sizeof(T));

            *reinterpret_cast<T*>(&(memory[position])) = value;
            position += sizeof(T);
        }
        template<typename C>
        std::basic_string<C> readString()
        {
            std::basic_string<C> str;
            C c = readInt<C>();
            while (c != 0)
            {
                str += c;
                c = readInt<C>();
            }
            return str;
        }
        template<typename C>
        std::basic_string<C> readString(size_t maxCount)
        {
            if (maxCount == 0) return std::basic_string<C>();

            std::basic_string<C> str;
            C c;
            while (maxCount --> 0)
            {
                c = readInt<C>();
                if (c == 0) break;
                str += c;
            }

            return str;
        }
        template<typename C>
        std::basic_string<C> peekString(size_t maxCount)
        {
            if (maxCount == 0) return std::basic_string<C>();

            size_t start = position;
            std::basic_string<C> str;
            C c;
            while (maxCount --> 0)
            {
                c = readInt<C>();
                if (c == 0) break;
                str += c;
            }
            position = start;

            return str;
        }
        template<typename C>
        std::basic_string<C> readStringExact(size_t count)
        {
            if (count == 0) return std::basic_string<C>();

            size_t stop = position + (count * sizeof(C));
            std::basic_string<C> str;
            C c;
            for (size_t i = 0; i < count; ++i)
            {
                c = readInt<C>();
                if (c == 0) break;
                str += c;
            }

            position = stop;
            return str;
        }
        template<typename C>
        void writeString(std::basic_string<C> str, const bool terminate = true)
        {
            if (memory.size() < position + (sizeof(C) * str.size()))
                memory.resize(position + (sizeof(C) * str.size()));

            for (const auto c : str)
            {
                if (c == 0)
                    break;
                writeInt<C>(c);
            }

            if (terminate)
                writeInt<C>(0);
        }
    };

    std::vector<uint8_t> LoadFile(
        const fs::path &path
    );

    bool SaveFile(
        const fs::path &path,
        const std::vector<uint8_t> &data
    );

    template<typename R, typename ...T, typename ...D>
    bool Await(
        std::thread *&thread,
        std::atomic<bool> *finished,
        R(*func)(T...),
        const std::tuple<D...> &data
    )
    {
        if (thread == nullptr)
        {
            finished->store(false);
            thread = new std::thread(
                [](std::atomic<bool> *finished, R(*f)(T...), const std::tuple<D...> *data) {
                    try { std::apply(f, *data); }
                    catch (std::exception e) { (void)e; }
                    finished->store(true);
                },
                finished, func, &data
            );
        }
        else if (finished->load())
        {
            thread->join();
            delete thread;
            thread = nullptr;
            return true;
        }
        return false;
    }

    struct glRect_t
    {
        GLint x = 0, y = 0;
        GLsizei w = 0, h = 0;
    };

    // TODO: recheck default states
    struct glState_t
    {
        GLint program = 0;
        GLuint texture = 0;
        GLenum activeTexture = 0;
        GLuint sampler = 0;
        GLuint vertexArray = 0;
        GLuint arrayBuffer = 0;
        GLint polygonMode[2] = {0, 0};
        GLenum clipOrigin = 0;
        glRect_t viewport;
        glRect_t scissorBox;
        struct blend_t
        {
            GLenum source = GL_ONE;
            GLenum destination = GL_ZERO;
            GLenum equation = GL_FUNC_ADD;
        };
        blend_t blendRGB;
        blend_t blendAlpha;
        GLboolean enableBlend = GL_FALSE;
        GLboolean enableCullFace = GL_FALSE;
        GLboolean enableDepthTest = GL_FALSE;
        GLboolean enableScissorTest = GL_FALSE;

        void backup();
        void restore();
    };

    struct glTexture_t
    {
        glTexture_t();
        glTexture_t(const vec2s_t size);
        glTexture_t(glTexture_t &) = delete;
        glTexture_t(glTexture_t &&other);
        glTexture_t(GLuint &&other, const vec2s_t size);
        ~glTexture_t();
        void operator=(glTexture_t &&other);
        bool valid() const;
        GLuint get() const;
        vec2s_t size() const;
    private:
        GLuint _tex;
        vec2s_t _size = {0, 0};
    };

    struct glWindow_t
    {
        SDL_DisplayMode displayMode;
        SDL_Window *window;
        SDL_GLContext context;
        vec2u32_t size;
    };

    glWindow_t InitGL(const char *title, vec2i_t screenSize, bool doubleBuffered = false,
        uint8_t depthSize = 24, uint8_t colorSize = 8, uint8_t stencilSize = 8, int display = 0);

    void ShutdownGL(glWindow_t &window);
}

#endif
