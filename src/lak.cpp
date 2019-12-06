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

#include <GL/gl3w.h>

#include <iostream>
#include <fstream>

namespace lak
{
  std::vector<uint8_t> read_file(const fs::path &path)
  {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    std::vector<uint8_t> result;
    if (!file.is_open()) return result;
    std::streampos fileSize = file.tellg();
    if (fileSize == std::streampos(-1)) return result;
    file.seekg(0);
    result.resize(fileSize);
    file.read(reinterpret_cast<char*>(result.data()), result.size());
    return result;
  }

  bool save_file(const fs::path &path, const std::vector<uint8_t> &data)
  {
    std::ofstream file(path,
                       std::ios::binary | std::ios::out | std::ios::trunc);
    if (!file.is_open()) return false;
    file.write(reinterpret_cast<const char*>(data.data()), data.size());
    return true;
  }

  bool save_file(const fs::path &path, const std::string &string)
  {
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) return false;
    file << string;
    return true;
  }

  void init_graphics()
  {
    SDL_SetMainReady();
    ASSERTF(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) == 0,
            "Failed to initialise SDL. Make sure you system has SDL2 "
            "installed or the SDL dll is in the same directory as Source "
            "Explorer");
  }

  void quit_graphics()
  {
    SDL_Quit();
  }

  void create_common_window(window_t &window,
                            const window_settings_t &settings,
                            Uint32 flags)
  {
    ASSERT(settings.size.x > 0);
    ASSERT(settings.size.y > 0);
    window.size = vec2u32_t(settings.size);

    SDL_GetCurrentDisplayMode(settings.display, &window.display_mode);

    window.window = SDL_CreateWindow(settings.title.c_str(),
                                     SDL_WINDOWPOS_CENTERED,
                                     SDL_WINDOWPOS_CENTERED,
                                     settings.size.x,
                                     settings.size.y,
                                     flags);
  }

  void create_software_window(window_t &window,
                              const window_settings_t &settings)
  {
    create_common_window(window, settings, SDL_WINDOW_RESIZABLE);
  }

  void destroy_software_window(window_t &window)
  {
    SDL_DestroyWindow(window.window);
  }

  void create_opengl_window(window_t &window,
                            const window_settings_t &settings)
  {
    ASSERT(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS,
                                SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG));
    ASSERT(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                                SDL_GL_CONTEXT_PROFILE_CORE));
    ASSERT(!SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,
                                settings.double_buffered));
    ASSERT(!SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE,   settings.depth_size));
    ASSERT(!SDL_GL_SetAttribute(SDL_GL_RED_SIZE,     settings.colour_size));
    ASSERT(!SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE,   settings.colour_size));
    ASSERT(!SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE,    settings.colour_size));
    ASSERT(!SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, settings.stencil_size));
    ASSERT(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3));
    ASSERT(!SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2));

    create_common_window(window,
                         settings,
                         SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);

    window.context.sdl_gl = SDL_GL_CreateContext(window.window);

    // context must be created before calling this
    ASSERTF(gl3wInit() == GL3W_OK,
            "Failed to initialise OpenGL. It is possible that your computer "
            "does not support OpenGL 3.2 which Source Explorer requires to "
            "run.\n!!THIS IS NOT A BUG!! (unless you are certain that your "
            "computer does actually support OpenGL 3.2)");

    SDL_GL_MakeCurrent(window.window, window.context.sdl_gl);

    if (SDL_GL_SetSwapInterval(-1) == -1)
      ASSERT(SDL_GL_SetSwapInterval(1) == 0);
  }

  void destroy_opengl_window(window_t &window)
  {
    ASSERT(window.context.sdl_gl != nullptr);
    SDL_GL_DeleteContext(window.context.sdl_gl);
    SDL_DestroyWindow(window.window);
  }
}