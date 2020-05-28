#include <lak/os.hpp>

#ifdef LAK_OS_WINDOWS
#  include "SDL_syswm.h"
#endif

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_impl_lak.h"

#include <imgui/examples/imgui_impl_softraster.h>
#include <imgui/imgui_internal.h>
#include <imgui/misc/cpp/imgui_stdlib.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat3x4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <lak/debug.hpp>
#include <lak/defer.hpp>
#include <lak/image.hpp>

#include <lak/opengl/shader.hpp>
#include <lak/opengl/state.hpp>
#include <lak/opengl/texture.hpp>

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace ImGui
{
  typedef struct _ImplSRContext
  {
    SDL_Window *window;
    SDL_Surface *screen_surface;
    texture_alpha8_t atlas_texture;
    texture_color16_t screen_texture;
    static const Uint32 screen_format = SDL_PIXELFORMAT_RGB565;
    // texture_color24_t screen_texture;
    // static const Uint32 screen_format = SDL_PIXELFORMAT_RGB888;
    // texture_color32_t screen_texture;
    // static const Uint32 screen_format = SDL_PIXELFORMAT_ABGR8888;
  } * ImplSRContext;

  typedef struct _ImplGLContext
  {
    GLint attrib_tex;
    GLint attrib_view_proj;
    GLint attrib_pos;
    GLint attrib_UV;
    GLint attrib_col;
    GLuint elements;
    GLuint array_buffer;
    GLuint vertex_array;
    lak::opengl::program shader;
    lak::opengl::texture font;
  } * ImplGLContext;

  typedef struct _ImplVkContext
  {
  } * ImplVkContext;

  typedef struct _ImplContext
  {
    ImGuiContext *imgui_context;
    SDL_Cursor *mouse_cursors[ImGuiMouseCursor_COUNT];
    bool mouse_release[3];
    lak::graphics_mode mode;
    glm::mat4x4 transform = glm::mat4x4(1.0f);
    union
    {
      void *vd_context;
      ImplSRContext sr_context;
      ImplGLContext gl_context;
      ImplVkContext vk_context;
    };
  } * ImplContext;

  ImplContext ImplCreateContext(lak::graphics_mode mode)
  {
    ImplContext result = new _ImplContext();
    result->mode       = mode;
    switch (mode)
    {
      case lak::graphics_mode::Software:
        result->sr_context = new _ImplSRContext();
        break;
      case lak::graphics_mode::OpenGL:
        result->gl_context = new _ImplGLContext();
        break;
      case lak::graphics_mode::Vulkan:
        result->vk_context = new _ImplVkContext();
        break;
      default: result->vd_context = nullptr; break;
    }
    result->imgui_context = CreateContext();
    return result;
  }

  void ImplDestroyContext(ImplContext context)
  {
    if (context != nullptr)
    {
      if (context->vd_context != nullptr)
      {
        switch (context->mode)
        {
          case lak::graphics_mode::Software: delete context->sr_context; break;
          case lak::graphics_mode::OpenGL: delete context->gl_context; break;
          case lak::graphics_mode::Vulkan: delete context->vk_context; break;
          default: FATAL("Invalid graphics mode"); break;
        }
      }
      delete context;
    }
  }

  inline void ImplUpdateDisplaySize(ImplContext context, SDL_Window *window)
  {
    ImGuiIO &io = ImGui::GetIO();

    int windW, windH;
    SDL_GetWindowSize(window, &windW, &windH);
    io.DisplaySize.x = windW;
    io.DisplaySize.y = windH;

    switch (context->mode)
    {
      case lak::graphics_mode::Software:
      {
        io.DisplayFramebufferScale.x = 1.0f;
        io.DisplayFramebufferScale.y = 1.0f;
        if ((size_t)windW != context->sr_context->screen_texture.w ||
            (size_t)windH != context->sr_context->screen_texture.h)
        {
          if (context->sr_context->screen_surface != nullptr)
            SDL_FreeSurface(context->sr_context->screen_surface);
          context->sr_context->screen_texture.init(windW, windH);
          context->sr_context->screen_surface =
            SDL_CreateRGBSurfaceWithFormatFrom(
              context->sr_context->screen_texture.pixels,
              context->sr_context->screen_texture.w,
              context->sr_context->screen_texture.h,
              context->sr_context->screen_texture.size * 8,
              context->sr_context->screen_texture.w *
                context->sr_context->screen_texture.size,
              context->sr_context->screen_format);
        }
      }
      break;
      case lak::graphics_mode::OpenGL:
      {
        int dispW, dispH;
        SDL_GL_GetDrawableSize(window, &dispW, &dispH);
        io.DisplayFramebufferScale.x =
          (windW > 0) ? (dispW / (float)windW) : 1.0f;
        io.DisplayFramebufferScale.y =
          (windH > 0) ? (dispH / (float)windH) : 1.0f;
      }
      break;
      case lak::graphics_mode::Vulkan:
      {
      }
      break;
      default: ASSERTF(false, "Invalid Context Mode"); break;
    }
  }

  char *ImplStaticClipboard = nullptr;
  void ImplInit()
  {
    ImGuiIO &io = ImGui::GetIO();

    io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
    io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
    io.BackendPlatformName = "imgui_impl_lak";
    io.BackendRendererName = "imgui_impl_lak";

    // SDL init

    io.KeyMap[ImGuiKey_Tab]        = SDL_SCANCODE_TAB;
    io.KeyMap[ImGuiKey_LeftArrow]  = SDL_SCANCODE_LEFT;
    io.KeyMap[ImGuiKey_RightArrow] = SDL_SCANCODE_RIGHT;
    io.KeyMap[ImGuiKey_UpArrow]    = SDL_SCANCODE_UP;
    io.KeyMap[ImGuiKey_DownArrow]  = SDL_SCANCODE_DOWN;
    io.KeyMap[ImGuiKey_PageUp]     = SDL_SCANCODE_PAGEUP;
    io.KeyMap[ImGuiKey_PageDown]   = SDL_SCANCODE_PAGEDOWN;
    io.KeyMap[ImGuiKey_Home]       = SDL_SCANCODE_HOME;
    io.KeyMap[ImGuiKey_End]        = SDL_SCANCODE_END;
    io.KeyMap[ImGuiKey_Insert]     = SDL_SCANCODE_INSERT;
    io.KeyMap[ImGuiKey_Delete]     = SDL_SCANCODE_DELETE;
    io.KeyMap[ImGuiKey_Backspace]  = SDL_SCANCODE_BACKSPACE;
    io.KeyMap[ImGuiKey_Space]      = SDL_SCANCODE_SPACE;
    io.KeyMap[ImGuiKey_Enter]      = SDL_SCANCODE_RETURN;
    io.KeyMap[ImGuiKey_Escape]     = SDL_SCANCODE_ESCAPE;
    io.KeyMap[ImGuiKey_A]          = SDL_SCANCODE_A;
    io.KeyMap[ImGuiKey_C]          = SDL_SCANCODE_C;
    io.KeyMap[ImGuiKey_V]          = SDL_SCANCODE_V;
    io.KeyMap[ImGuiKey_X]          = SDL_SCANCODE_X;
    io.KeyMap[ImGuiKey_Y]          = SDL_SCANCODE_Y;
    io.KeyMap[ImGuiKey_Z]          = SDL_SCANCODE_Z;

    io.SetClipboardTextFn = ImplSetClipboard;
    io.GetClipboardTextFn = (const char *(*)(void *))ImplGetClipboard;
    io.ClipboardUserData  = (void *)&ImplStaticClipboard;
  }

  void ImplInitSRContext(ImplSRContext context, const lak::window &window)
  {
    context->window = window.sdl_window();

    ImGuiIO &io = ImGui::GetIO();

    uint8_t *pixels;
    int width, height;
    io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
    context->atlas_texture.init(width, height, (alpha8_t *)pixels);
    io.Fonts->TexID = &context->atlas_texture;

    context->screen_texture.init(window.size().x, window.size().y);

    context->screen_surface = SDL_CreateRGBSurfaceWithFormatFrom(
      context->screen_texture.pixels,
      context->screen_texture.w,
      context->screen_texture.h,
      context->screen_texture.size * 8,
      context->screen_texture.w * context->screen_texture.size,
      context->screen_format);

    ImGui_ImplSoftraster_Init(&context->screen_texture);
  }

  void ImplInitGLContext(ImplGLContext context, const lak::window &window)
  {
    using namespace lak::opengl::literals;

    context->shader = lak::opengl::program::create(
      "#version 130\n"
      "uniform mat4 viewProj;\n"
      "in vec2 vPosition;\n"
      "in vec2 vUV;\n"
      "in vec4 vColor;\n"
      "out vec2 fUV;\n"
      "out vec4 fColor;\n"
      "void main()\n"
      "{\n"
      "   fUV = vUV;\n"
      "   fColor = vColor;\n"
      "   gl_Position = viewProj * vec4(vPosition.xy, 0, 1);\n"
      "}"_vertex_shader,
      "#version 130\n"
      "uniform sampler2D fTexture;\n"
      "in vec2 fUV;\n"
      "in vec4 fColor;\n"
      "out vec4 pColor;\n"
      "void main()\n"
      "{\n"
      "   pColor = fColor * texture(fTexture, fUV.st);\n"
      "}"_fragment_shader);

    context->attrib_tex       = *context->shader.uniform_location("fTexture");
    context->attrib_view_proj = *context->shader.uniform_location("viewProj");
    context->attrib_pos       = *context->shader.attrib_location("vPosition");
    context->attrib_UV        = *context->shader.attrib_location("vUV");
    context->attrib_col       = *context->shader.attrib_location("vColor");

    glGenBuffers(1, &context->array_buffer);
    glGenBuffers(1, &context->elements);

    ImGuiIO &io = ImGui::GetIO();

    // Create fonts texture
    uint8_t *pixels;
    lak::vec2i_t size;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &size.x, &size.y);

    auto old_texture = lak::opengl::get_uint(GL_TEXTURE_BINDING_2D);
    DEFER(glBindTexture(GL_TEXTURE_2D, old_texture));

    context->font.init(GL_TEXTURE_2D)
      .bind()
      .apply(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
      .apply(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
      .store_mode(GL_UNPACK_ROW_LENGTH, 0)
      .build(0, GL_RGBA, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    io.Fonts->TexID = (ImTextureID)(intptr_t)context->font.get();
  }

  void ImplInitVkContext(ImplVkContext context, const lak::window &window) {}

  void ImplInitContext(ImplContext context, const lak::window &window)
  {
    context->mouse_cursors[ImGuiMouseCursor_Arrow] =
      SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
    context->mouse_cursors[ImGuiMouseCursor_TextInput] =
      SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
    context->mouse_cursors[ImGuiMouseCursor_ResizeAll] =
      SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
    context->mouse_cursors[ImGuiMouseCursor_ResizeNS] =
      SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
    context->mouse_cursors[ImGuiMouseCursor_ResizeEW] =
      SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
    context->mouse_cursors[ImGuiMouseCursor_ResizeNESW] =
      SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
    context->mouse_cursors[ImGuiMouseCursor_ResizeNWSE] =
      SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
    context->mouse_cursors[ImGuiMouseCursor_Hand] =
      SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

    context->mouse_release[0] = false;
    context->mouse_release[1] = false;
    context->mouse_release[2] = false;

    switch (context->mode)
    {
      case lak::graphics_mode::Software:
        ImplInitSRContext(context->sr_context, window);
        break;
      case lak::graphics_mode::OpenGL:
        ImplInitGLContext(context->gl_context, window);
        break;
      case lak::graphics_mode::Vulkan:
        ImplInitVkContext(context->vk_context, window);
        break;
      default: ASSERTF(false, "Invalid Context Mode"); break;
    }

    ImplUpdateDisplaySize(context, window.sdl_window());

#ifdef LAK_OS_WINDOWS
    SDL_SysWMinfo wmInfo;
    SDL_VERSION(&wmInfo.version);
    SDL_GetWindowWMInfo(window.sdl_window(), &wmInfo);
    ImGui::GetIO().ImeWindowHandle = wmInfo.info.win.window;
#endif
  }

  void ImplShutdownSRContext(ImplSRContext context)
  {
    context->window = nullptr;

    ImGui_ImplSoftraster_Shutdown();

    SDL_FreeSurface(context->screen_surface);
    context->screen_surface = nullptr;

    context->screen_texture.init(0, 0);
    context->atlas_texture.init(0, 0);
  }

  void ImplShutdownGLContext(ImplGLContext context)
  {
    if (context->array_buffer) glDeleteBuffers(1, &context->array_buffer);
    context->array_buffer = 0;

    if (context->elements) glDeleteBuffers(1, &context->elements);
    context->elements = 0;

    context->shader.clear();

    context->font.clear();
    ImGui::GetIO().Fonts->TexID = (ImTextureID)(intptr_t)0;
  }

  void ImplShutdownVkContext(ImplVkContext context) {}

  void ImplShutdownContext(ImplContext context)
  {
    // SDL shutdown

    for (SDL_Cursor *&cursor : context->mouse_cursors)
    {
      SDL_FreeCursor(cursor);
      cursor = nullptr;
    }

    switch (context->mode)
    {
      case lak::graphics_mode::Software:
        ImplShutdownSRContext(context->sr_context);
        break;
      case lak::graphics_mode::OpenGL:
        ImplShutdownGLContext(context->gl_context);
        break;
      case lak::graphics_mode::Vulkan:
        ImplShutdownVkContext(context->vk_context);
        break;
      default: ASSERTF(false, "Invalid Context Mode"); break;
    }

    if (context->imgui_context != nullptr)
    {
      DestroyContext(context->imgui_context);
      context->imgui_context = nullptr;
    }

    std::free(context);
    context = nullptr;
  }

  void ImplSetCurrentContext(ImplContext context)
  {
    SetCurrentContext(context->imgui_context);
  }

  void ImplSetTransform(ImplContext context, const glm::mat4x4 &transform)
  {
    context->transform = transform;
  }

  void ImplNewFrame(ImplContext context,
                    SDL_Window *window,
                    const float delta_time,
                    const bool call_base_new_frame)
  {
    ImGuiIO &io = ImGui::GetIO();

    ASSERTF(io.Fonts->IsBuilt(), "Font atlas not built");

    ASSERT(delta_time > 0);
    io.DeltaTime = delta_time;

    // UpdateMousePosAndButtons()
    if (io.WantSetMousePos)
    {
      // ImGui enforces mouse position
      SDL_WarpMouseInWindow(window, (int)io.MousePos.x, (int)io.MousePos.y);
    }

    // UpdateMouseCursor()
    if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange) == 0)
    {
      ImGuiMouseCursor cursor = ImGui::GetMouseCursor();

      if (io.MouseDrawCursor || (cursor == ImGuiMouseCursor_None))
      {
        SDL_ShowCursor(SDL_FALSE);
      }
      else
      {
        SDL_SetCursor(context->mouse_cursors[cursor]);
        SDL_ShowCursor(SDL_TRUE);
      }
    }

    if (call_base_new_frame) ImGui::NewFrame();
  }

  bool ImplProcessEvent(ImplContext context, const SDL_Event &event)
  {
    ImGuiIO &io = ImGui::GetIO();

    switch (event.type)
    {
      case SDL_WINDOWEVENT:
        switch (event.window.event)
        {
          case SDL_WINDOWEVENT_RESIZED:
          case SDL_WINDOWEVENT_SIZE_CHANGED:
          {
            ImplUpdateDisplaySize(context,
                                  SDL_GetWindowFromID(event.window.windowID));
          }
            return true;
        }
        return false;

      case SDL_MOUSEWHEEL:
      {
        if (event.wheel.x > 0)
          io.MouseWheelH += 1;
        else if (event.wheel.x < 0)
          io.MouseWheelH -= 1;
        if (event.wheel.y > 0)
          io.MouseWheel += 1;
        else if (event.wheel.y < 0)
          io.MouseWheel -= 1;
      }
        return true;

      case SDL_MOUSEMOTION:
      {
        if (io.WantSetMousePos) return false;
        io.MousePos.x = (float)event.motion.x;
        io.MousePos.y = (float)event.motion.y;
        SDL_CaptureMouse(ImGui::IsAnyMouseDown() ? SDL_TRUE : SDL_FALSE);
      }
        return true;

      case SDL_MOUSEBUTTONDOWN:
      {
        switch (event.button.button)
        {
          case SDL_BUTTON_LEFT:
          {
            io.MouseDown[0]           = true;
            context->mouse_release[0] = false;
          }
          break;

          case SDL_BUTTON_RIGHT:
          {
            io.MouseDown[1]           = true;
            context->mouse_release[1] = false;
          }
          break;

          case SDL_BUTTON_MIDDLE:
          {
            io.MouseDown[2]           = true;
            context->mouse_release[2] = false;
          }
          break;

          default: return false;
        }
      }
        return true;
      case SDL_MOUSEBUTTONUP:
      {
        switch (event.button.button)
        {
          case SDL_BUTTON_LEFT:
          {
            context->mouse_release[0] = true;
          }
          break;

          case SDL_BUTTON_RIGHT:
          {
            context->mouse_release[1] = true;
          }
          break;

          case SDL_BUTTON_MIDDLE:
          {
            context->mouse_release[2] = true;
          }
          break;

          default: return false;
        }
      }
        return true;

      case SDL_TEXTINPUT:
      {
        io.AddInputCharactersUTF8(event.text.text);
      }
        return true;

      case SDL_KEYDOWN:
      case SDL_KEYUP:
      {
        const int key = event.key.keysym.scancode;
        ASSERT(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
        io.KeysDown[key] = (event.type == SDL_KEYDOWN);

        const SDL_Keymod mod = SDL_GetModState();
        io.KeyShift          = ((mod & KMOD_SHIFT) != 0);
        io.KeyCtrl           = ((mod & KMOD_CTRL) != 0);
        io.KeyAlt            = ((mod & KMOD_ALT) != 0);
        io.KeySuper          = ((mod & KMOD_GUI) != 0);
      }
        return true;
    }
    return false;
  }

  void ImplRender(ImplContext context, const bool call_base_render)
  {
    if (call_base_render) Render();
    ImplRenderData(context, ImGui::GetDrawData());
  }

  void ImplSRRender(ImplContext context, ImDrawData *draw_data)
  {
    ASSERT(context != nullptr);
    ASSERT(context->sr_context != nullptr);
    auto *sr_context = context->sr_context;
    ASSERT(sr_context->window != nullptr);

    ImGui_ImplSoftraster_RenderDrawData(draw_data);

    SDL_Surface *window = SDL_GetWindowSurface(sr_context->window);

    if (window != nullptr)
    {
      SDL_Rect clip;
      SDL_GetClipRect(window, &clip);
      SDL_FillRect(
        window, &clip, SDL_MapRGBA(window->format, 0x00, 0x00, 0x00, 0xFF));
      if (SDL_BlitSurface(
            sr_context->screen_surface, nullptr, window, nullptr))
        ERROR(SDL_GetError());
    }
  }

  void ImplGLRender(ImplContext context, ImDrawData *draw_data)
  {
    ASSERT(draw_data != nullptr);
    ASSERT(context->gl_context != nullptr);
    auto *gl_context = context->gl_context;

    ImGuiIO &io = ImGui::GetIO();

    lak::vec4f_t viewport;
    viewport.x = draw_data->DisplayPos.x;
    viewport.y = draw_data->DisplayPos.y;
    viewport.z = draw_data->DisplaySize.x * io.DisplayFramebufferScale.x;
    viewport.w = draw_data->DisplaySize.y * io.DisplayFramebufferScale.y;
    if (viewport.z <= 0 || viewport.w <= 0) return;

    draw_data->ScaleClipRects(io.DisplayFramebufferScale);

    auto old_program        = lak::opengl::get_uint(GL_CURRENT_PROGRAM);
    auto old_texture        = lak::opengl::get_uint(GL_TEXTURE_BINDING_2D);
    auto old_active_texture = lak::opengl::get_uint(GL_ACTIVE_TEXTURE);
    auto old_vertex_array   = lak::opengl::get_uint(GL_VERTEX_ARRAY_BINDING);
    auto old_array_buffer   = lak::opengl::get_uint(GL_ARRAY_BUFFER_BINDING);
    auto old_index_buffer =
      lak::opengl::get_uint(GL_ELEMENT_ARRAY_BUFFER_BINDING);
    auto old_blend_enabled        = glIsEnabled(GL_BLEND);
    auto old_cull_face_enabled    = glIsEnabled(GL_CULL_FACE);
    auto old_depth_test_enabled   = glIsEnabled(GL_DEPTH_TEST);
    auto old_scissor_test_enabled = glIsEnabled(GL_SCISSOR_TEST);
    auto old_viewport             = lak::opengl::get_int<4>(GL_VIEWPORT);
    auto old_scissor              = lak::opengl::get_int<4>(GL_SCISSOR_BOX);
    auto old_clip_origin          = lak::opengl::get_enum(GL_CLIP_ORIGIN);

    glGenVertexArrays(1, &gl_context->vertex_array);

    DEFER({
      glUseProgram(old_program);
      glActiveTexture(old_active_texture);
      glBindTexture(GL_TEXTURE_2D, old_texture);
      glBindVertexArray(old_vertex_array);
      glBindBuffer(GL_ARRAY_BUFFER, old_array_buffer);
      glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, old_index_buffer);
      glEnableDisable(GL_BLEND, old_blend_enabled);
      glEnableDisable(GL_CULL_FACE, old_cull_face_enabled);
      glEnableDisable(GL_DEPTH_TEST, old_depth_test_enabled);
      glEnableDisable(GL_SCISSOR_TEST, old_scissor_test_enabled);
      glViewport(
        old_viewport[0], old_viewport[1], old_viewport[2], old_viewport[3]);
      glScissor(
        old_scissor[0], old_scissor[1], old_scissor[2], old_scissor[3]);

      glDeleteVertexArrays(1, &gl_context->vertex_array);
      gl_context->vertex_array = 0;
    });

    glUseProgram(gl_context->shader.get());
    glBindTexture(GL_TEXTURE_2D, gl_context->font.get());
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(gl_context->vertex_array);
    glBindBuffer(GL_ARRAY_BUFFER, gl_context->array_buffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_context->elements);
    glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
    glBlendFuncSeparate(GL_SRC_ALPHA,
                        GL_ONE_MINUS_SRC_ALPHA,
                        GL_SRC_ALPHA,
                        GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glViewport(viewport.x, viewport.y, viewport.z, viewport.w);

    {
      const float &W = draw_data->DisplaySize.x;
      const float &H = draw_data->DisplaySize.y;
      const float O1 = ((draw_data->DisplayPos.x * 2) + W) / -W;
      const float O2 = ((draw_data->DisplayPos.y * 2) + H) / H;
      // // clang-format off
      // const float orthoProj[] = {
      //   2.0f / W, 0.0f,       0.0f,   0.0f,
      //   0.0f,     2.0f / -H,  0.0f,   0.0f,
      //   0.0f,     0.0f,       -1.0f,  0.0f,
      //   O1,       O2,         0.0f,   1.0f
      // };
      // // clang-format on
      // clang-format off
      [[maybe_unused]] const glm::mat4x4 orthoProj = {
        2.0f / W, 0.0f,       0.0f,   0.0f,
        0.0f,     2.0f / -H,  0.0f,   0.0f,
        0.0f,     0.0f,       -1.0f,  0.0f,
        O1,       O2,         0.0f,   1.0f
      };
      // clang-format on
      // const glm::mat4x4 transform = context->transform * orthoProj;
      // const glm::mat4x4 transform = glm::mat4x4(1.0f);
      const glm::mat4x4 transform = glm::scale(
        glm::translate(glm::mat4x4(1.0f), glm::vec3(-1.0, 1.0, 0.0)),
        glm::vec3(2.0 / draw_data->DisplaySize.x,
                  2.0 / -draw_data->DisplaySize.y,
                  1.0));
      // const glm::mat4x4 transform = orthoProj * glm::mat4x4(1.0f);
      glUniformMatrix4fv(
        gl_context->attrib_view_proj, 1, GL_FALSE, &transform[0][0]);
    }
    glUniform1i(gl_context->attrib_tex, 0);
    // #ifdef GL_SAMPLER_BINDING
    // glBindSampler(0, 0);
    // #endif

    glEnableVertexAttribArray(gl_context->attrib_pos);
    glVertexAttribPointer(gl_context->attrib_pos,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(ImDrawVert),
                          (GLvoid *)IM_OFFSETOF(ImDrawVert, pos));

    glEnableVertexAttribArray(gl_context->attrib_UV);
    glVertexAttribPointer(gl_context->attrib_UV,
                          2,
                          GL_FLOAT,
                          GL_FALSE,
                          sizeof(ImDrawVert),
                          (GLvoid *)IM_OFFSETOF(ImDrawVert, uv));

    glEnableVertexAttribArray(gl_context->attrib_col);
    glVertexAttribPointer(gl_context->attrib_col,
                          4,
                          GL_UNSIGNED_BYTE,
                          GL_TRUE,
                          sizeof(ImDrawVert),
                          (GLvoid *)IM_OFFSETOF(ImDrawVert, col));

    for (int n = 0; n < draw_data->CmdListsCount; ++n)
    {
      const ImDrawList *cmdList        = draw_data->CmdLists[n];
      const ImDrawIdx *idxBufferOffset = 0;

      glBufferData(GL_ARRAY_BUFFER,
                   (GLsizeiptr)cmdList->VtxBuffer.Size * sizeof(ImDrawVert),
                   (const GLvoid *)cmdList->VtxBuffer.Data,
                   GL_STREAM_DRAW);

      glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                   (GLsizeiptr)cmdList->IdxBuffer.Size * sizeof(ImDrawIdx),
                   (const GLvoid *)cmdList->IdxBuffer.Data,
                   GL_STREAM_DRAW);

      for (int cmdI = 0; cmdI < cmdList->CmdBuffer.Size; ++cmdI)
      {
        const ImDrawCmd &pcmd = cmdList->CmdBuffer[cmdI];
        if (pcmd.UserCallback)
        {
          pcmd.UserCallback(cmdList, &pcmd);
        }
        else if (true)
        {
          glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd.TextureId);
          glDrawElements(GL_TRIANGLES,
                         (GLsizei)pcmd.ElemCount,
                         sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT
                                                : GL_UNSIGNED_INT,
                         idxBufferOffset);
        }
        else
        {
          lak::vec4f_t clip;
          clip.x = pcmd.ClipRect.x - viewport.x;
          clip.y = pcmd.ClipRect.y - viewport.y;
          clip.z = pcmd.ClipRect.z - viewport.x;
          clip.w = pcmd.ClipRect.w - viewport.y;

          if (clip.x < viewport.z && clip.y < viewport.w && clip.z >= 0.0f &&
              clip.w >= 0.0f)
          {
#ifdef GL_CLIP_ORIGIN
            if (old_clip_origin == GL_UPPER_LEFT)
              // Support for GL 4.5's glClipControl(GL_UPPER_LEFT)
              glScissor(
                (GLint)clip.x, (GLint)clip.y, (GLint)clip.z, (GLint)clip.w);
            else
#endif
              glScissor((GLint)clip.x,
                        (GLint)(viewport.w - clip.w),
                        (GLint)(clip.z - clip.x),
                        (GLint)(clip.w - clip.y));

            glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd.TextureId);
            glDrawElements(GL_TRIANGLES,
                           (GLsizei)pcmd.ElemCount,
                           sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT
                                                  : GL_UNSIGNED_INT,
                           idxBufferOffset);
          }
        }
        idxBufferOffset += pcmd.ElemCount;
      }
    }
  }

  void ImplVkRender(ImplContext context, ImDrawData *draw_data) {}

  void ImplRenderData(ImplContext context, ImDrawData *draw_data)
  {
    ASSERT(context);
    ASSERT(draw_data);
    switch (context->mode)
    {
      case lak::graphics_mode::Software:
        ImplSRRender(context, draw_data);
        break;
      case lak::graphics_mode::OpenGL: ImplGLRender(context, draw_data); break;
      case lak::graphics_mode::Vulkan: ImplVkRender(context, draw_data); break;
      default: FATAL("Invalid context mode"); break;
    }

    ImGuiIO &io = ImGui::GetIO();
    for (size_t i = 0; i < 3; ++i)
      if (context->mouse_release[i]) io.MouseDown[i] = false;
  }

  void ImplSetClipboard(void *, const char *text)
  {
    SDL_SetClipboardText(text);
  }

  const char *ImplGetClipboard(char **clipboard)
  {
    if (*clipboard) SDL_free(*clipboard);
    *clipboard = SDL_GetClipboardText();
    return *clipboard;
  }

  ImTextureID ImplGetFontTexture(ImplContext context)
  {
    switch (context->mode)
    {
      case lak::graphics_mode::Software:
        return (ImTextureID)&context->sr_context->atlas_texture;
      case lak::graphics_mode::OpenGL:
        return (ImTextureID)(uintptr_t)context->gl_context->font.get();
      case lak::graphics_mode::Vulkan:
      default: FATAL("Invalid context mode"); break;
    }
  }
}

namespace lak
{
  bool HoriSplitter(float &top,
                    float &bottom,
                    float width,
                    float topMin,
                    float bottomMin,
                    float length)
  {
    const float thickness = ImGui::GetStyle().FramePadding.x * 2;
    const float maxLeft   = width - (bottomMin + thickness);
    if (top > maxLeft) top = maxLeft;
    bottom = width - (top + thickness);

    ImGuiContext &g     = *GImGui;
    ImGuiWindow *window = g.CurrentWindow;
    ImGuiID id          = window->GetID("##Splitter");
    ImRect bb;

    bb.Min.x = window->DC.CursorPos.x + top;
    bb.Min.y = window->DC.CursorPos.y;

    bb.Max = ImGui::CalcItemSize(ImVec2(thickness, length), 0.0f, 0.0f);
    bb.Max.x += bb.Min.x;
    bb.Max.y += bb.Min.y;

    return ImGui::SplitterBehavior(
      bb, id, ImGuiAxis_X, &top, &bottom, topMin, bottomMin);
  }

  bool VertSplitter(float &left,
                    float &right,
                    float width,
                    float leftMin,
                    float rightMin,
                    float length)
  {
    const float thickness = ImGui::GetStyle().FramePadding.x * 2;
    const float maxTop    = width - (rightMin + thickness);
    if (left > maxTop) left = maxTop;
    right = width - (left + thickness);

    ImGuiContext &g     = *GImGui;
    ImGuiWindow *window = g.CurrentWindow;
    ImGuiID id          = window->GetID("##Splitter");
    ImRect bb;

    bb.Min.x = window->DC.CursorPos.x;
    bb.Min.y = window->DC.CursorPos.y + left;

    bb.Max = ImGui::CalcItemSize(ImVec2(length, thickness), 0.0f, 0.0f);
    bb.Max.x += bb.Min.x;
    bb.Max.y += bb.Min.y;

    return ImGui::SplitterBehavior(
      bb, id, ImGuiAxis_Y, &left, &right, leftMin, rightMin);
  }

  bool TreeNode(const char *fmt, ...)
  {
    va_list args;
    va_start(args, fmt);

    ImGuiWindow *window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return false;

    ImGuiContext &g = *GImGui;

    ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);

    bool is_open =
      ImGui::TreeNodeBehavior(window->GetID(g.TempBuffer), 0, g.TempBuffer);

    va_end(args);
    return is_open;
  }
}

#include <imgui/imgui.cpp>
#include <imgui/imgui_demo.cpp>
#include <imgui/imgui_draw.cpp>
#include <imgui/imgui_widgets.cpp>
#include <imgui/misc/cpp/imgui_stdlib.cpp>
