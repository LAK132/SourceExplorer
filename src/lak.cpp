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

#include "lak.h"

namespace lak
{
    float mod(const float A, const float B)
    {
        const int64_t frac = (int64_t)(A / B);
        const float result = A - ((float)frac * B);
        return result;
    }

    std::vector<uint8_t> LoadFile(const fs::path &path)
    {
        std::ifstream file(path, std::ios::binary | std::ios::ate);

        if (!file.is_open())
            return {};

        size_t fileSize = file.tellg();

        file.seekg(0);

        std::vector<uint8_t> result;
        result.reserve(fileSize);

        while (fileSize --> 0)
            result.emplace_back(file.get());

        file.close();

        return result;
    }

    bool SaveFile(const fs::path &path, const std::vector<uint8_t> &data)
    {
        std::ofstream file(path, std::ios::binary | std::ios::out | std::ios::trunc);

        if (!file.is_open())
            return false;

        for (auto d : data)
            file << d;

        file.close();

        return true;
    }

    glWindow_t InitGL(const char *title, vec2i_t screenSize, bool doubleBuffered,
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

        glWindow_t window;

        window.size = screenSize;

        SDL_GetCurrentDisplayMode(display, &window.displayMode);

        window.window = SDL_CreateWindow(title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            screenSize.x, screenSize.y, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);

        window.context = SDL_GL_CreateContext(window.window);

        assert(gl3wInit() == GL3W_OK); // context must be created before calling this

        SDL_GL_MakeCurrent(window.window, window.context);

        if (SDL_GL_SetSwapInterval(-1) == -1)
            assert(SDL_GL_SetSwapInterval(1) == 0);

        return window;
    }

    void ShutdownGL(glWindow_t &window)
    {
        SDL_GL_DeleteContext(window.context);
        SDL_DestroyWindow(window.window);
        SDL_Quit();
    }

    void glState_t::backup()
    {
        glGetIntegerv(GL_CURRENT_PROGRAM, &program);
        glGetIntegerv(GL_TEXTURE_BINDING_2D, (GLint*)&texture);
        glGetIntegerv(GL_ACTIVE_TEXTURE, (GLint*)&activeTexture);
        glGetIntegerv(GL_ARRAY_BUFFER_BINDING, (GLint*)&arrayBuffer);
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, (GLint*)&vertexArray);
        GLint port[4];
        glGetIntegerv(GL_VIEWPORT, port);
        viewport.x = port[0];
        viewport.y = port[1];
        viewport.w = (GLsizei)port[2];
        viewport.h = (GLsizei)port[3];
        GLint box[4];
        glGetIntegerv(GL_SCISSOR_BOX, box);
        scissorBox.x = box[0];
        scissorBox.y = box[1];
        scissorBox.w = (GLsizei)box[2];
        scissorBox.h = (GLsizei)box[3];

        #ifdef GL_SAMPLER_BINDING
        glGetIntegerv(GL_SAMPLER_BINDING, (GLint*)&sampler);
        #endif

        #ifdef GL_POLYGON_MODE
        glGetIntegerv(GL_POLYGON_MODE, polygonMode);
        #endif

        #ifdef GL_CLIP_ORIGIN
        // Support for GL 4.5's glClipControl(GL_UPPER_LEFT);
        glGetIntegerv(GL_CLIP_ORIGIN, (GLint*)&clipOrigin);
        #endif

        glGetIntegerv(GL_BLEND_SRC_RGB, (GLint*)&blendRGB.source);
        glGetIntegerv(GL_BLEND_DST_RGB, (GLint*)&blendRGB.destination);
        glGetIntegerv(GL_BLEND_EQUATION_RGB, (GLint*)&blendRGB.equation);

        glGetIntegerv(GL_BLEND_SRC_ALPHA, (GLint*)&blendAlpha.source);
        glGetIntegerv(GL_BLEND_DST_ALPHA, (GLint*)&blendAlpha.destination);
        glGetIntegerv(GL_BLEND_EQUATION_ALPHA, (GLint*)&blendAlpha.equation);

        enableBlend = glIsEnabled(GL_BLEND);
        enableCullFace = glIsEnabled(GL_CULL_FACE);
        enableScissorTest = glIsEnabled(GL_SCISSOR_TEST);
    }

    void glState_t::restore()
    {
        glUseProgram(program);
        glBindTexture(GL_TEXTURE_2D, texture);

        #ifdef GL_SAMPLER_BINDING
        glBindSampler(0, sampler);
        #endif

        glActiveTexture(activeTexture);
        glBindVertexArray(vertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, arrayBuffer);
        glBlendEquationSeparate(blendRGB.equation, blendAlpha.equation);
        glBlendFuncSeparate(blendRGB.source, blendRGB.destination,
            blendAlpha.source, blendAlpha.destination);

        if (enableBlend) glEnable(GL_BLEND); else glDisable(GL_BLEND);
        if (enableCullFace) glEnable(GL_CULL_FACE); else glDisable(GL_CULL_FACE);
        if (enableDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
        if (enableScissorTest) glEnable(GL_SCISSOR_TEST); else glDisable(GL_SCISSOR_TEST);

        #ifdef GL_POLYGON_MODE
        glPolygonMode(GL_FRONT_AND_BACK, (GLenum)polygonMode[0]);
        #endif

        glViewport(viewport.x, viewport.y, viewport.w, viewport.h);
        glScissor(scissorBox.x, scissorBox.y, scissorBox.w, scissorBox.h);
    }

    glTexture_t::glTexture_t() {}

    glTexture_t::glTexture_t(const vec2s_t size)
    {
        _size = size;
        if (size.x > 0 && size.y > 0)
        glGenTextures(1, &_tex);
    }

    glTexture_t::glTexture_t(glTexture_t &&other)
    {
        _tex = other._tex;
        _size = other._size;
        other._size = {0, 0};
    }

    glTexture_t::glTexture_t(GLuint &&other, const vec2s_t size)
    {
        _tex = other;
        _size = size;
    }

    glTexture_t::~glTexture_t()
    {
        if (_size.x > 0 && _size.y > 0)
            glDeleteTextures(1, &_tex);
    }

    void glTexture_t::operator=(glTexture_t &&other)
    {
        if (_size.x > 0 && _size.y > 0)
            glDeleteTextures(1, &_tex);
        _tex = other._tex;
        _size = other._size;
        other._size = {0, 0};
    }

    bool glTexture_t::valid() const
    {
        return _size.x > 0 && _size.y > 0;
    }

    GLuint glTexture_t::get() const
    {
        return _tex;
    }

    vec2s_t glTexture_t::size() const
    {
        return _size;
    }
}

extern "C" {
#include <GL/gl3w.c>
}
