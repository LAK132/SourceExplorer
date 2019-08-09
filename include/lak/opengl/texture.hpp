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
#ifndef LAK_OPENGL_TEXTURE_H
#define LAK_OPENGL_TEXTURE_H

#include "../image.h"

#include <GL/gl3w.h>

#include <memory>

namespace lak
{
    namespace opengl
    {
        struct texture
        {
        private:
            GLuint _texture = 0; // 0 is reserved, use it as null texture
            GLenum _target = GL_TEXTURE_2D;
            vec2<GLsizei> _size = {0, 0};
        public:
            texture() = default;
            texture(GLenum target);
            texture(texture &&other);
            ~texture();
            texture &operator=(texture &&other);

            texture &init(GLenum target);
            texture &clear();

            inline GLuint get() const { return _texture; }
            inline const vec2<GLsizei> &size() const { return _size; }

            texture &bind();
            const texture &bind() const;

            texture &apply(GLenum pname, GLint value);
            texture &apply(GLenum pname, GLint *value);

            texture &applyi(GLenum pname, GLint *value);
            texture &applyi(GLenum pname, GLuint *value);

            texture &apply(GLenum pname, GLfloat value);
            texture &apply(GLenum pname, GLfloat *value);

            texture &store_mode(GLenum pname, GLint value);
            texture &store_mode(GLenum pname, GLfloat value);

            texture &build(GLint level, GLint format, vec2<GLsizei> size, GLint border, GLenum pixel_format, GLenum color_type, const GLvoid *pixels);
        };
    }
}

#endif