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

#include "lak.h"

namespace lak
{
    std::vector<uint8_t> LoadFile(const fs::path &path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);

        if (!file.is_open())
            return {};

        size_t fileSize = file.tellg();

        file.seekg(0);

        std::vector<uint8_t> result;

        result.resize(fileSize);

        file.read(reinterpret_cast<char*>(&result[0]), result.size());

        file.close();

        return result;
    }

    bool SaveFile(const fs::path &path, const std::vector<uint8_t> &data)
    {
        std::ofstream file(path, std::ios::binary | std::ios::out | std::ios::trunc);

        if (!file.is_open())
            return false;

        file.write((const char *)&data[0], data.size());

        file.close();

        return true;
    }

    void InitSR(window_t &wnd, const char *title, vec2i_t screenSize, bool doubleBuffered, int display)
    {
        SDL_SetMainReady();
        assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);

        wnd.size = (vec2u32_t)screenSize;

        SDL_GetCurrentDisplayMode(display, &wnd.displayMode);

        wnd.window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            screenSize.x, screenSize.y, SDL_WINDOW_RESIZABLE);

        // wnd.srContext = SDL_CreateRenderer(wnd.window, -1, 0);
    }

    void ShutdownSR(window_t &wnd)
    {
        // SDL_DestroyRenderer(wnd.srContext);
        SDL_DestroyWindow(wnd.window);
        SDL_Quit();
    }

    void InitGL(window_t &wnd, const char *title, vec2i_t screenSize, bool doubleBuffered,
        uint8_t depthSize, uint8_t colorSize, uint8_t stencilSize, int display)
    {
        SDL_SetMainReady();
        assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);

        assert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG) == 0);
        assert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE) == 0);
        assert(SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, doubleBuffered) == 0);
        assert(SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   depthSize) == 0);
        assert(SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     colorSize) == 0);
        assert(SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   colorSize) == 0);
        assert(SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    colorSize) == 0);
        assert(SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencilSize) == 0);
        assert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3) == 0);
        assert(SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2) == 0);

        wnd.size = (vec2u32_t)screenSize;

        SDL_GetCurrentDisplayMode(display, &wnd.displayMode);

        wnd.window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            screenSize.x, screenSize.y, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

        wnd.glContext = SDL_GL_CreateContext(wnd.window);

        assert(gl3wInit() == GL3W_OK); // context must be created before calling this

        SDL_GL_MakeCurrent(wnd.window, wnd.glContext);

        if (SDL_GL_SetSwapInterval(-1) == -1)
            assert(SDL_GL_SetSwapInterval(1) == 0);
    }

    void ShutdownGL(window_t &wnd)
    {
        assert(wnd.glContext != nullptr);
        SDL_GL_DeleteContext(wnd.glContext);
        SDL_DestroyWindow(wnd.window);
        SDL_Quit();
    }

    void InitVk(window_t &wnd, const char *title, vec2i_t screenSize, bool doubleBuffered, int display)
    {
        SDL_SetMainReady();
        assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0);

        wnd.size = (vec2u32_t)screenSize;

        SDL_GetCurrentDisplayMode(display, &wnd.displayMode);

        wnd.window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            screenSize.x, screenSize.y, SDL_WINDOW_RESIZABLE);

        // wnd.srContext = ;
    }

    void ShutdownVk(window_t &wnd)
    {
        SDL_DestroyWindow(wnd.window);
        SDL_Quit();
    }
}

extern "C" {
#include <GL/gl3w.c>
}
