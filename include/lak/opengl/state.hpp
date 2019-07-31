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
#ifndef LAK_OPENGL_STATE_HPP
#define LAK_OPENGL_STATE_HPP

#include <GL/gl3w.h>

#include <array>

namespace lak
{
    namespace opengl
    {
        template <size_t S>
        std::array<GLboolean, S> GetBoolean(GLenum target)
        {
            std::array<GLboolean, S> result;
            glGetBooleanv(target, result.data());
            return result;
        }

        template <size_t S>
        std::array<GLint, S> GetInt(GLenum target)
        {
            std::array<GLint, S> result;
            glGetIntegerv(target, result.data());
            return result;
        }

        template <size_t S>
        std::array<GLuint, S> GetUint(GLenum target)
        {
            std::array<GLuint, S> result;
            glGetIntegerv(target, (GLint*)result.data());
            return result;
        }

        template <size_t S>
        std::array<GLenum, S> GetEnum(GLenum target)
        {
            std::array<GLenum, S> result;
            glGetIntegerv(target, (GLint*)result.data());
            return result;
        }

        template <size_t S>
        std::array<GLint64, S> GetInt64(GLenum target)
        {
            std::array<GLint64, S> result;
            glGetInteger64v(target, result.data());
            return result;
        }

        template <size_t S>
        std::array<GLuint64, S> GetUint64(GLenum target)
        {
            std::array<GLuint64, S> result;
            glGetInteger64v(target, (GLint64*)result.data());
            return result;
        }

        template <size_t S>
        std::array<GLfloat, S> GetFloat(GLenum target)
        {
            std::array<GLfloat, S> result;
            glGetFloatv(target, result.data());
            return result;
        }

        template <size_t S>
        std::array<GLdouble, S> GetDouble(GLenum target)
        {
            std::array<GLdouble, S> result;
            glGetDoublev(target, result.data());
            return result;
        }
    }
}

#endif // LAK_OPENGL_STATE_HPP