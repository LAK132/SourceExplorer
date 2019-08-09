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
#ifndef LAK_OPENGL_SHADER_H
#define LAK_OPENGL_SHADER_H

#include "texture.hpp"

#include <string>
#include <optional>

namespace lak
{
    namespace opengl
    {
        struct shader
        {
        private:
            GLuint _shader = 0; // 0 is reserved
            GLenum _type = 0;
        public:
            shader() = default;
            shader(const std::string &code, GLenum shader_type);
            shader(shader &&other);
            ~shader();
            shader &operator=(shader &&other);

            shader &clear();

            inline operator bool() const { return _shader != 0; }
            inline GLuint get() const { return _shader; }
            inline GLenum type() const { return _type; }
        };

        namespace literals
        {
            shader operator""_vertex_shader(const char *str, size_t);
            shader operator""_fragment_shader(const char *str, size_t);
            shader operator""_tess_control_shader(const char *str, size_t);
            shader operator""_tess_eval_shader(const char *str, size_t);
            shader operator""_geometry_shader(const char *str, size_t);
            shader operator""_compute_shader(const char *str, size_t);
        }

        struct program
        {
        private:
            GLuint _program = 0; // 0 is reserved
        public:
            program() = default;
            program(const shader &vertex, const shader &fragment);
            program(program &&other);
            ~program();
            program &operator=(program &&other);

            program &init();
            program &attach(const shader &shader);
            program &link();
            program &clear();

            inline operator bool() const { return _program != 0; }
            inline GLuint get() const { return _program; }
            std::optional<std::string> link_error() const;

            GLint uniform_location(const GLchar *name) const;
            GLint attrib_location(const GLchar *name) const;
        };
    }
}

#endif