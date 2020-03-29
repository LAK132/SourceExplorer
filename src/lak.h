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

#ifndef LAK_LAK_H
#define LAK_LAK_H

#include <lak/string.hpp>
#include <lak/vec.h>

#define SDL_MAIN_HANDLED
#include <SDL.h>

#include <atomic>
#include <filesystem>
#include <memory>
#include <thread>
#include <tuple>
#include <vector>

namespace lak
{
  namespace fs = std::filesystem;

  std::vector<uint8_t> read_file(const fs::path &path);
  bool save_file(const fs::path &path, const std::vector<uint8_t> &data);
  bool save_file(const fs::path &path, const std::string &string);

  template<typename R, typename... T, typename... D>
  bool await(std::unique_ptr<std::thread> &thread,
             std::atomic<bool> &finished,
             R (*func)(T...),
             const std::tuple<D...> &data)
  {
    if (!thread)
    {
      finished.store(false);
      void (*functor)(
        std::atomic<bool> *, R(*)(T...), const std::tuple<D...> *) =
        [](std::atomic<bool> *finished,
           R (*f)(T...),
           const std::tuple<D...> *data) {
          try
          {
            std::apply(f, *data);
          }
          catch (...)
          {
          }
          finished->store(true);
        };

      thread = std::make_unique<std::thread>(
        std::thread(functor, &finished, func, &data));
    }
    else if (finished.load())
    {
      thread->join();
      thread.reset();
      return true;
    }
    return false;
  }

  enum struct graphics_mode
  {
    ERROR    = 0,
    OPENGL   = 1,
    SOFTWARE = 2,
  };

  union graphics_context_t {
    void *ptr;
    SDL_GLContext sdl_gl;
  };

  struct window_t
  {
    SDL_DisplayMode display_mode;
    SDL_Window *window         = nullptr;
    graphics_context_t context = {nullptr};
    graphics_mode mode         = graphics_mode::OPENGL;
    vec2u32_t size;
  };

  struct window_settings_t
  {
    std::string title;
    vec2i_t size;
    bool double_buffered = false;
    int display          = 0;

    // OpenGL
    uint8_t depth_size   = 24;
    uint8_t colour_size  = 8;
    uint8_t stencil_size = 8;
  };

  void init_graphics();
  void quit_graphics();

  bool create_software_window(window_t &window,
                              const window_settings_t &settings);
  void destroy_software_window(window_t &window);

  bool create_opengl_window(window_t &window,
                            const window_settings_t &settings);
  void destroy_opengl_window(window_t &window);
}

#endif
