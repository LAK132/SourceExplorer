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
#ifndef IMGUI_IMPL_LAK_H
#define IMGUI_IMPL_LAK_H

#include "lak.h"

#include <imgui/examples/imgui_impl_softraster.h>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#ifdef _WIN32
#  include "SDL_syswm.h"
#endif

#include <lak/image.h>
#include <lak/opengl/shader.hpp>
#include <lak/opengl/texture.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace ImGui
{
  enum class GraphicsMode
  {
    SOFTWARE = 0,
    OPENGL   = 1,
    VULKAN   = 2
  };

  typedef struct _ImplSRContext
  {
    SDL_Window *window;
    SDL_Surface *screenSurface;
    texture_alpha8_t atlasTexture;
    texture_color16_t screenTexture;
    static const Uint32 screenFormat = SDL_PIXELFORMAT_RGB565;
    // texture_color24_t screenTexture;
    // static const Uint32 screenFormat = SDL_PIXELFORMAT_RGB888;
    // texture_color32_t screenTexture;
    // static const Uint32 screenFormat = SDL_PIXELFORMAT_ABGR8888;
  } * ImplSRContext;

  typedef struct _ImplGLContext
  {
    GLint attribTex;
    GLint attribViewProj;
    GLint attribPos;
    GLint attribUV;
    GLint attribCol;
    GLuint elements;
    GLuint arrayBuffer;
    GLuint vertexArray;
    lak::opengl::program shader;
    lak::opengl::texture font;
  } * ImplGLContext;

  typedef struct _ImplVkContext
  {
  } * ImplVkContext;

  typedef struct _ImplContext
  {
    ImGuiContext *imContext;
    SDL_Cursor *mouseCursors[ImGuiMouseCursor_COUNT];
    bool mouseRelease[3];
    GraphicsMode mode;
    union {
      void *vdContext;
      ImplSRContext srContext;
      ImplGLContext glContext;
      ImplVkContext vkContext;
    };
  } * ImplContext;

  ImplContext ImplCreateContext(GraphicsMode mode);

  void ImplDestroyContext(ImplContext context);

  // Run once at startup
  void ImplInit();

  // Run once per context
  void ImplInitContext(ImplContext context, const lak::window_t &window);

  // Run once per context
  void ImplShutdownContext(ImplContext context);

  void ImplSetCurrentContext(ImplContext context);

  void ImplNewFrame(ImplContext context,
                    SDL_Window *window,
                    const float deltaTime,
                    const bool callBaseNewFrame = true);

  bool ImplProcessEvent(ImplContext context, const SDL_Event &event);

  void ImplRender(ImplContext context, const bool callBaseRender = true);

  void ImplRender(ImplContext context, ImDrawData *drawData);

  void ImplSetClipboard(void *, const char *text);

  const char *ImplGetClipboard(char **clipboard);
}

namespace lak
{
  template<typename R, typename... T, typename... D>
  bool AwaitPopup(const char *str_id,
                  bool &open,
                  std::thread *&staticThread,
                  std::atomic<bool> &staticFinished,
                  R (*callback)(T...),
                  const std::tuple<D...> &callbackData)
  {
    if (ImGui::BeginPopup(str_id, ImGuiWindowFlags_AlwaysAutoResize))
    {
      if (lak::await(staticThread, &staticFinished, callback, callbackData))
      {
        ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
        open = false;
        return false;
      }
      open = true;
    }
    else
    {
      open = false;
      ImGui::OpenPopup(str_id);
    }

    return true;
  }

  bool VertSplitter(float &left,
                    float &right,
                    float width,
                    float leftMin  = 8.0f,
                    float rightMin = 8.0f,
                    float length   = -1.0f);

  bool HoriSplitter(float &top,
                    float &bottom,
                    float width,
                    float topMin    = 8.0f,
                    float bottomMin = 8.0f,
                    float length    = -1.0f);

  bool TreeNode(const char *fmt, ...);
}

#endif