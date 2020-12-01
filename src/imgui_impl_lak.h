#ifndef IMGUI_IMPL_LAK_H
#define IMGUI_IMPL_LAK_H

#include <glm/mat4x4.hpp>

#include <imgui/imgui.h>

#include "await.hpp"

#include <lak/span.hpp>
#include <lak/window.hpp>

namespace ImGui
{
  struct _ImplContext;
  using ImplContext = _ImplContext *;

  ImplContext ImplCreateContext(lak::graphics_mode mode);

  void ImplDestroyContext(ImplContext context);

  // Run once at startup
  void ImplInit();

  // Run once per context
  void ImplInitContext(ImplContext context, const lak::window &window);

  // Run once per context
  void ImplShutdownContext(ImplContext context);

  void ImplSetCurrentContext(ImplContext context);

  void ImplSetTransform(ImplContext context, const glm::mat4x4 &transform);

  void ImplNewFrame(ImplContext context,
                    const lak::window &window,
                    const float delta_time,
                    const bool call_base_new_frame = true);

  bool ImplProcessEvent(ImplContext context, const lak::event &event);

  void ImplRender(ImplContext context, const bool call_base_render = true);

  void ImplRenderData(ImplContext context, ImDrawData *draw_data);

  void ImplSetClipboard(void *, const char *text);

  const char *ImplGetClipboard(char **clipboard);

  ImTextureID ImplGetFontTexture(ImplContext context);

  template<typename T>
  lak::span<T> ToSpan(ImVector<T> &vec)
  {
    return lak::span<T>(vec.Data, vec.Size);
  }

  template<typename T>
  lak::span<const T> ToSpan(const ImVector<T> &vec)
  {
    return lak::span<const T>(vec.Data, vec.Size);
  }
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