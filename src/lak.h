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
#include <SDL_audio.h>
extern "C" {
#include <GL/gl3w.h>
}

#include <lak/vec.h>
#include <memory/memory.hpp>

#ifndef LAK_H
#define LAK_H

namespace lak
{
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
                    catch (...) { }
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

    struct window_t
    {
        SDL_DisplayMode displayMode;
        SDL_Window *window = nullptr;
        SDL_GLContext glContext = nullptr;
        vec2u32_t size;
    };

    void InitSR(
        window_t &wnd,
        const char *title,
        vec2i_t screenSize,
        bool doubleBuffered = false,
        int display = 0
    );

    void ShutdownSR(
        window_t &wnd
    );

    void InitGL(
        window_t &wnd,
        const char *title,
        vec2i_t screenSize,
        bool doubleBuffered = false,
        uint8_t depthSize = 24,
        uint8_t colorSize = 8,
        uint8_t stencilSize = 8,
        int display = 0
    );

    void ShutdownGL(
        window_t &wnd
    );

    void InitVk(
        window_t &wnd,
        const char *title,
        vec2i_t screenSize,
        bool doubleBuffered = false,
        int display = 0
    );

    void ShutdownVk(
        window_t &wnd
    );
}

#endif
