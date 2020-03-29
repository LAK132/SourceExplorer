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
#include "shader.hpp"

namespace lak
{
    namespace opengl
    {
        shader::shader(const std::string &code, GLenum shader_type)
        {
            _type = shader_type;
            _shader = glCreateShader(_type);
            const GLchar *cstr = code.c_str();
            glShaderSource(_shader, 1, &cstr, nullptr);
            glCompileShader(_shader);
            GLint compiled;
            glGetShaderiv(_shader, GL_COMPILE_STATUS, &compiled);
            if (!compiled)
            {
                GLint msgSize = 0;
                glGetShaderiv(_shader, GL_INFO_LOG_LENGTH, &msgSize);
                std::vector<char> msg; msg.resize(msgSize);
                glGetShaderInfoLog(_shader, msgSize, nullptr, msg.data());
                fprintf(stderr, "Shader failed to compile\n%s\n", msg.data());

                glDeleteShader(_shader);
                _shader = 0;
            }
        }

        shader::shader(shader &&other)
        {
            std::swap(_shader, other._shader);
        }

        shader::~shader()
        {
            clear();
        }

        shader &shader::operator=(shader &&other)
        {
            std::swap(_shader, other._shader);
            return *this;
        }

        shader &shader::clear()
        {
            if (_shader != 0)
            {
                glDeleteProgram(_shader);
                _shader = 0;
            }
            return *this;
        }

        namespace literals
        {
            shader operator""_vertex_shader(const char *str, size_t)
            {
                return shader(str, GL_VERTEX_SHADER);
            }

            shader operator""_fragment_shader(const char *str, size_t)
            {
                return shader(str, GL_FRAGMENT_SHADER);
            }

            shader operator""_tess_control_shader(const char *str, size_t)
            {
                return shader(str, GL_TESS_CONTROL_SHADER);
            }

            shader operator""_tess_eval_shader(const char *str, size_t)
            {
                return shader(str, GL_TESS_EVALUATION_SHADER);
            }

            shader operator""_geometry_shader(const char *str, size_t)
            {
                return shader(str, GL_GEOMETRY_SHADER);
            }

            shader operator""_compute_shader(const char *str, size_t)
            {
                return shader(str, GL_COMPUTE_SHADER);
            }
        }

        program::program(const shader &vertex, const shader &fragment)
        : _program(0)
        {
            if (!vertex || !fragment) return;
            init();
            attach(vertex);
            attach(fragment);
            link();
        }

        program::program(program &&other)
        {
            std::swap(_program, other._program);
        }

        program::~program()
        {
            clear();
        }

        program &program::operator=(program &&other)
        {
            std::swap(_program, other._program);
            return *this;
        }

        program &program::init()
        {
            if (_program != 0) clear();
            _program = glCreateProgram();
            return *this;
        }

        program &program::attach(const shader &shader)
        {
            if (shader.get() != 0)
                glAttachShader(_program, shader.get());
            return *this;
        }

        program &program::link()
        {
            glLinkProgram(_program);
            return *this;
        }

        std::optional<std::string> program::link_error() const
        {
            GLint linked;
            glGetProgramiv(_program, GL_LINK_STATUS, &linked);
            if (!linked)
            {
                GLint msgSize = 0;
                glGetProgramiv(_program, GL_INFO_LOG_LENGTH, &msgSize);
                std::vector<char> msg; msg.resize(msgSize + 1);
                glGetProgramInfoLog(_program, msgSize, nullptr, msg.data());
                msg[msgSize] = 0;
                return std::make_optional(std::string(msg.data()));
            }
            return std::nullopt;
        }

        program &program::clear()
        {
            if (_program != 0)
            {
                glDeleteProgram(_program);
                _program = 0;
            }
            return *this;
        }

        GLint program::uniform_location(const GLchar *name) const
        {
            return glGetUniformLocation(_program, name);
        }

        GLint program::attrib_location(const GLchar *name) const
        {
            return glGetAttribLocation(_program, name);
        }
    }
}