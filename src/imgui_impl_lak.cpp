/*
MIT License

Copyright (c) 2019 Lucas Kleiss (LAK132)

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

#include "imgui_impl_lak.h"
#include "lak/opengl/state.hpp"
#include "lak/defer/defer.hpp"

namespace ImGui
{
    ImplContext ImplCreateContext(GraphicsMode mode)
    {
        ImplContext result = new _ImplContext();
        result->mode = mode;
        switch (mode)
        {
            case GraphicsMode::SOFTWARE:
                result->srContext = new _ImplSRContext();
                break;
            case GraphicsMode::OPENGL:
                result->glContext = new _ImplGLContext();
                break;
            case GraphicsMode::VULKAN:
                result->vkContext = new _ImplVkContext();
                break;
            default:
                result->vdContext = nullptr;
                break;
        }
        result->imContext = CreateContext();
        return result;
    }

    void ImplDestroyContext(ImplContext context)
    {
        if (context != nullptr)
        {
            if (context->vdContext != nullptr)
            {
                switch (context->mode)
                {
                    case GraphicsMode::SOFTWARE: delete context->srContext; break;
                    case GraphicsMode::OPENGL: delete context->glContext; break;
                    case GraphicsMode::VULKAN: delete context->vkContext; break;
                    default: ASSERTF(false, "Invalid Context Mode"); break;
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
            case GraphicsMode::SOFTWARE: {
                io.DisplayFramebufferScale.x = 1.0f;
                io.DisplayFramebufferScale.y = 1.0f;
                if ((size_t)windW != context->srContext->screenTexture.w ||
                    (size_t)windH != context->srContext->screenTexture.h)
                {
                    if (context->srContext->screenSurface != nullptr)
                        SDL_FreeSurface(context->srContext->screenSurface);
                    context->srContext->screenTexture.init(windW, windH);
                    context->srContext->screenSurface = SDL_CreateRGBSurfaceWithFormatFrom(
                        context->srContext->screenTexture.pixels,
                        context->srContext->screenTexture.w,
                        context->srContext->screenTexture.h,
                        context->srContext->screenTexture.size * 8,
                        context->srContext->screenTexture.w * context->srContext->screenTexture.size,
                        context->srContext->screenFormat
                    );
                }
            } break;
            case GraphicsMode::OPENGL: {
                int dispW, dispH;
                SDL_GL_GetDrawableSize(window, &dispW, &dispH);
                io.DisplayFramebufferScale.x = (windW > 0) ? (dispW / (float)windW) : 1.0f;
                io.DisplayFramebufferScale.y = (windH > 0) ? (dispH / (float)windH) : 1.0f;
            } break;
            case GraphicsMode::VULKAN: {

            } break;
            default:
                ASSERTF(false, "Invalid Context Mode");
                break;
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

        io.KeyMap[ImGuiKey_Tab]         = SDL_SCANCODE_TAB;
        io.KeyMap[ImGuiKey_LeftArrow]   = SDL_SCANCODE_LEFT;
        io.KeyMap[ImGuiKey_RightArrow]  = SDL_SCANCODE_RIGHT;
        io.KeyMap[ImGuiKey_UpArrow]     = SDL_SCANCODE_UP;
        io.KeyMap[ImGuiKey_DownArrow]   = SDL_SCANCODE_DOWN;
        io.KeyMap[ImGuiKey_PageUp]      = SDL_SCANCODE_PAGEUP;
        io.KeyMap[ImGuiKey_PageDown]    = SDL_SCANCODE_PAGEDOWN;
        io.KeyMap[ImGuiKey_Home]        = SDL_SCANCODE_HOME;
        io.KeyMap[ImGuiKey_End]         = SDL_SCANCODE_END;
        io.KeyMap[ImGuiKey_Insert]      = SDL_SCANCODE_INSERT;
        io.KeyMap[ImGuiKey_Delete]      = SDL_SCANCODE_DELETE;
        io.KeyMap[ImGuiKey_Backspace]   = SDL_SCANCODE_BACKSPACE;
        io.KeyMap[ImGuiKey_Space]       = SDL_SCANCODE_SPACE;
        io.KeyMap[ImGuiKey_Enter]       = SDL_SCANCODE_RETURN;
        io.KeyMap[ImGuiKey_Escape]      = SDL_SCANCODE_ESCAPE;
        io.KeyMap[ImGuiKey_A]           = SDL_SCANCODE_A;
        io.KeyMap[ImGuiKey_C]           = SDL_SCANCODE_C;
        io.KeyMap[ImGuiKey_V]           = SDL_SCANCODE_V;
        io.KeyMap[ImGuiKey_X]           = SDL_SCANCODE_X;
        io.KeyMap[ImGuiKey_Y]           = SDL_SCANCODE_Y;
        io.KeyMap[ImGuiKey_Z]           = SDL_SCANCODE_Z;

        io.SetClipboardTextFn = ImplSetClipboard;
        io.GetClipboardTextFn = (const char*(*)(void*))ImplGetClipboard;
        io.ClipboardUserData = (void *)&ImplStaticClipboard;
    }

    void ImplInitSRContext(ImplSRContext context, const lak::window_t &window)
    {
        context->window = window.window;

        ImGuiIO &io = ImGui::GetIO();

        uint8_t* pixels;
        int width, height;
        io.Fonts->GetTexDataAsAlpha8(&pixels, &width, &height);
        context->atlasTexture.init(width, height, (alpha8_t*)pixels);
        io.Fonts->TexID = &context->atlasTexture;

        context->screenTexture.init(window.size.x, window.size.y);

        context->screenSurface = SDL_CreateRGBSurfaceWithFormatFrom(
            context->screenTexture.pixels,
            context->screenTexture.w,
            context->screenTexture.h,
            context->screenTexture.size * 8,
            context->screenTexture.w * context->screenTexture.size,
            context->screenFormat
        );

        ImGui_ImplSoftraster_Init(&context->screenTexture);
    }

    void ImplInitGLContext(ImplGLContext context, const lak::window_t &window)
    {
        using namespace lak::opengl::literals;

        context->shader.init()
            .attach("#version 130\n"
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
                "}"_vertex_shader)
            .attach("#version 130\n"
                "uniform sampler2D fTexture;\n"
                "in vec2 fUV;\n"
                "in vec4 fColor;\n"
                "out vec4 pColor;\n"
                "void main()\n"
                "{\n"
                "   pColor = fColor * texture(fTexture, fUV.st);\n"
                "}"_fragment_shader)
            .link();

        context->attribTex      = context->shader.uniform_location("fTexture");
        context->attribViewProj = context->shader.uniform_location("viewProj");
        context->attribPos      = context->shader.attrib_location("vPosition");
        context->attribUV       = context->shader.attrib_location("vUV");
        context->attribCol      = context->shader.attrib_location("vColor");

        glGenBuffers(1, &context->arrayBuffer);
        glGenBuffers(1, &context->elements);

        ImGuiIO &io = ImGui::GetIO();

        // Create fonts texture
        uint8_t *pixels;
        lak::vec2i_t size;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &size.x, &size.y);

        auto [old_texture] = lak::opengl::GetUint<1>(GL_TEXTURE_BINDING_2D);
        DEFER(glBindTexture(GL_TEXTURE_2D, old_texture));

        context->font.init(GL_TEXTURE_2D).bind()
            .apply(GL_TEXTURE_MIN_FILTER, GL_LINEAR)
            .apply(GL_TEXTURE_MAG_FILTER, GL_LINEAR)
            .store_mode(GL_UNPACK_ROW_LENGTH, 0)
            .build(0, GL_RGBA, size, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        io.Fonts->TexID = (ImTextureID)(intptr_t)context->font.get();
    }

    void ImplInitVkContext(ImplVkContext context, const lak::window_t &window)
    {
    }

    void ImplInitContext(ImplContext context, const lak::window_t &window)
    {
        context->mouseCursors[ImGuiMouseCursor_Arrow]        = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_ARROW);
        context->mouseCursors[ImGuiMouseCursor_TextInput]    = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_IBEAM);
        context->mouseCursors[ImGuiMouseCursor_ResizeAll]    = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEALL);
        context->mouseCursors[ImGuiMouseCursor_ResizeNS]     = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENS);
        context->mouseCursors[ImGuiMouseCursor_ResizeEW]     = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZEWE);
        context->mouseCursors[ImGuiMouseCursor_ResizeNESW]   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENESW);
        context->mouseCursors[ImGuiMouseCursor_ResizeNWSE]   = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_SIZENWSE);
        context->mouseCursors[ImGuiMouseCursor_Hand]         = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);

        context->mouseRelease[0] = false;
        context->mouseRelease[1] = false;
        context->mouseRelease[2] = false;

        switch (context->mode)
        {
            case GraphicsMode::SOFTWARE:
                ImplInitSRContext(context->srContext, window);
                break;
            case GraphicsMode::OPENGL:
                ImplInitGLContext(context->glContext, window);
                break;
            case GraphicsMode::VULKAN:
                ImplInitVkContext(context->vkContext, window);
                break;
            default:
                ASSERTF(false, "Invalid Context Mode");
                break;
        }

        ImplUpdateDisplaySize(context, window.window);

        #ifdef _WIN32
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window.window, &wmInfo);
        ImGui::GetIO().ImeWindowHandle = wmInfo.info.win.window;
        #endif
    }

    void ImplShutdownSRContext(ImplSRContext context)
    {
        context->window = nullptr;

        ImGui_ImplSoftraster_Shutdown();

        SDL_FreeSurface(context->screenSurface);
        context->screenSurface = nullptr;

        context->screenTexture.init(0, 0);
        context->atlasTexture.init(0, 0);
    }

    void ImplShutdownGLContext(ImplGLContext context)
    {
        if (context->arrayBuffer) glDeleteBuffers(1, &context->arrayBuffer);
        context->arrayBuffer = 0;

        if (context->elements) glDeleteBuffers(1, &context->elements);
        context->elements = 0;

        context->shader.clear();

        context->font.clear();
        ImGui::GetIO().Fonts->TexID = (ImTextureID)(intptr_t)0;
    }

    void ImplShutdownVkContext(ImplVkContext context)
    {
    }

    void ImplShutdownContext(ImplContext context)
    {
        // SDL shutdown

        for (SDL_Cursor *&cursor : context->mouseCursors)
        {
            SDL_FreeCursor(cursor);
            cursor = nullptr;
        }

        switch (context->mode)
        {
            case GraphicsMode::SOFTWARE:
                ImplShutdownSRContext(context->srContext);
                break;
            case GraphicsMode::OPENGL:
                ImplShutdownGLContext(context->glContext);
                break;
            case GraphicsMode::VULKAN:
                ImplShutdownVkContext(context->vkContext);
                break;
            default:
                ASSERTF(false, "Invalid Context Mode");
                break;
        }

        if (context->imContext != nullptr)
        {
            DestroyContext(context->imContext);
            context->imContext = nullptr;
        }

        std::free(context);
        context = nullptr;
    }

    void ImplSetCurrentContext(ImplContext context)
    {
        SetCurrentContext(context->imContext);
    }

    void ImplNewFrame(ImplContext context, SDL_Window *window, const float deltaTime, const bool callBaseNewFrame)
    {
        ImGuiIO &io = ImGui::GetIO();

        ASSERTF(io.Fonts->IsBuilt(), "Font atlas not built");

        ASSERT(deltaTime > 0);
        io.DeltaTime = deltaTime;

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
                SDL_SetCursor(context->mouseCursors[cursor]);
                SDL_ShowCursor(SDL_TRUE);
            }
        }

        if (callBaseNewFrame)
            ImGui::NewFrame();
    }

    bool ImplProcessEvent(ImplContext context, const SDL_Event &event)
    {
        ImGuiIO &io = ImGui::GetIO();

        switch (event.type)
        {
            case SDL_WINDOWEVENT: switch (event.window.event)
            {
                case SDL_WINDOWEVENT_RESIZED:
                case SDL_WINDOWEVENT_SIZE_CHANGED: {
                    ImplUpdateDisplaySize(context, SDL_GetWindowFromID(event.window.windowID));
                } return true;
            } return false;

            case SDL_MOUSEWHEEL: {
                if      (event.wheel.x > 0) io.MouseWheelH += 1;
                else if (event.wheel.x < 0) io.MouseWheelH -= 1;
                if      (event.wheel.y > 0) io.MouseWheel += 1;
                else if (event.wheel.y < 0) io.MouseWheel -= 1;
            } return true;

            case SDL_MOUSEMOTION: {
                if (io.WantSetMousePos)
                    return false;
                io.MousePos.x = (float)event.motion.x;
                io.MousePos.y = (float)event.motion.y;
                SDL_CaptureMouse(ImGui::IsAnyMouseDown() ? SDL_TRUE : SDL_FALSE);
            } return true;

            case SDL_MOUSEBUTTONDOWN: {
                switch (event.button.button)
                {
                    case SDL_BUTTON_LEFT: {
                        io.MouseDown[0] = true;
                        context->mouseRelease[0] = false;
                    } break;

                    case SDL_BUTTON_RIGHT: {
                        io.MouseDown[1] = true;
                        context->mouseRelease[1] = false;
                    } break;

                    case SDL_BUTTON_MIDDLE: {
                        io.MouseDown[2] = true;
                        context->mouseRelease[2] = false;
                    } break;

                    default: return false;
                }
            } return true;
            case SDL_MOUSEBUTTONUP: {
                switch (event.button.button)
                {
                    case SDL_BUTTON_LEFT: {
                        context->mouseRelease[0] = true;
                    } break;

                    case SDL_BUTTON_RIGHT: {
                        context->mouseRelease[1] = true;
                    } break;

                    case SDL_BUTTON_MIDDLE: {
                        context->mouseRelease[2] = true;
                    } break;

                    default: return false;
                }
            } return true;

            case SDL_TEXTINPUT: {
                io.AddInputCharactersUTF8(event.text.text);
            } return true;

            case SDL_KEYDOWN:
            case SDL_KEYUP: {
                const int key = event.key.keysym.scancode;
                ASSERT(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
                io.KeysDown[key] = (event.type == SDL_KEYDOWN);

                const SDL_Keymod mod = SDL_GetModState();
                io.KeyShift = ((mod & KMOD_SHIFT) != 0);
                io.KeyCtrl  = ((mod & KMOD_CTRL) != 0);
                io.KeyAlt   = ((mod & KMOD_ALT) != 0);
                io.KeySuper = ((mod & KMOD_GUI) != 0);
            } return true;
        }
        return false;
    }

    void ImplRender(ImplContext context, const bool callBaseRender)
    {
        if (callBaseRender)
            Render();
        ImplRender(context, ImGui::GetDrawData());
    }

    void ImplSRRender(ImplSRContext context, ImDrawData *drawData)
    {
        // ERROR(context);
        ASSERT(context != nullptr);
        // ERROR(context->window);
        ASSERT(context->window != nullptr);

        ImGui_ImplSoftraster_RenderDrawData(drawData);
        // ERROR(drawData);

        [[maybe_unused]] SDL_Surface *window = SDL_GetWindowSurface(context->window);
        // ERROR(window);

        if (window != nullptr)
        {
            SDL_Rect clip;
            SDL_GetClipRect(window, &clip);
            // SDL_FillRect(window, &clip, SDL_MapRGBA(window->format, 0xFF, 0xFF, 0xFF, 0xFF));
            SDL_FillRect(window, &clip, SDL_MapRGBA(window->format, 0x00, 0x00, 0x00, 0xFF));
            // DEBUG("Pixels " << context->screenTexture.pixels <<
            // "\nWidth " << context->screenTexture.w <<
            // "\nHeight " << context->screenTexture.h <<
            // "\nDepth " << context->screenTexture.size * 8 <<
            // "\nPitch " << context->screenTexture.w * context->screenTexture.size);

            // SDL_Surface *surf = SDL_CreateRGBSurfaceWithFormatFrom(
            //     context->screenTexture.pixels,
            //     context->screenTexture.w,
            //     context->screenTexture.h,
            //     context->screenTexture.size * 8,
            //     context->screenTexture.w * context->screenTexture.align,
            //     SDL_PIXELFORMAT_RGB888
            // );
            // SDL_LockSurface(context->screenSurface);
            // ASSERT(SDL_LockSurface(surf) == 0);
            // ASSERT(SDL_LockSurface(window) == 0);
            if(SDL_BlitSurface(context->screenSurface, nullptr, window, nullptr))
            // if(SDL_BlitSurface(surf, nullptr, window, nullptr))
                ERROR(SDL_GetError());
            // SDL_UnlockSurface(window);
            // SDL_UnlockSurface(surf);
            // SDL_UnlockSurface(context->screenSurface);

            // SDL_FreeSurface(surf);
        }
    }

    void ImplGLRender(ImplGLContext context, ImDrawData *drawData)
    {
        ASSERT(drawData != nullptr);
        ImGuiIO &io = ImGui::GetIO();

        lak::vec4f_t viewport;
        viewport.x = drawData->DisplayPos.x;
        viewport.y = drawData->DisplayPos.y;
        viewport.z = drawData->DisplaySize.x * io.DisplayFramebufferScale.x;
        viewport.w = drawData->DisplaySize.y * io.DisplayFramebufferScale.y;
        if (viewport.z <= 0 || viewport.w <= 0) return;

        drawData->ScaleClipRects(io.DisplayFramebufferScale);

        auto [old_program] = lak::opengl::GetUint<1>(GL_CURRENT_PROGRAM);
        auto [old_texture] = lak::opengl::GetUint<1>(GL_TEXTURE_BINDING_2D);
        auto [old_active_texture] = lak::opengl::GetUint<1>(GL_ACTIVE_TEXTURE);
        auto [old_vertex_array] = lak::opengl::GetUint<1>(GL_VERTEX_ARRAY_BINDING);
        auto [old_array_buffer] = lak::opengl::GetUint<1>(GL_ARRAY_BUFFER_BINDING);
        auto [old_index_buffer] = lak::opengl::GetUint<1>(GL_ELEMENT_ARRAY_BUFFER_BINDING);
        auto old_blend_enabled = glIsEnabled(GL_BLEND);
        auto old_cull_face_enabled = glIsEnabled(GL_CULL_FACE);
        auto old_depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
        auto old_scissor_test_enabled = glIsEnabled(GL_SCISSOR_TEST);
        auto old_viewport = lak::opengl::GetInt<4>(GL_VIEWPORT);
        auto old_scissor = lak::opengl::GetInt<4>(GL_SCISSOR_BOX);
        auto [old_clip_origin] = lak::opengl::GetEnum<1>(GL_CLIP_ORIGIN);

        glGenVertexArrays(1, &context->vertexArray);

        DEFER
        ({
            glUseProgram(old_program);
            glActiveTexture(old_active_texture);
            glBindTexture(GL_TEXTURE_2D, old_texture);
            glBindVertexArray(old_vertex_array);
            glBindBuffer(GL_ARRAY_BUFFER, old_array_buffer);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, old_index_buffer);
            if (old_blend_enabled) glEnable(GL_BLEND);
            else glDisable(GL_BLEND);
            if (old_cull_face_enabled) glEnable(GL_CULL_FACE);
            else glDisable(GL_CULL_FACE);
            if (old_depth_test_enabled) glEnable(GL_DEPTH_TEST);
            else glDisable(GL_DEPTH_TEST);
            if (old_scissor_test_enabled) glEnable(GL_SCISSOR_TEST);
            else glDisable(GL_SCISSOR_TEST);
            glViewport(old_viewport[0], old_viewport[1], old_viewport[2], old_viewport[3]);
            glScissor(old_scissor[0], old_scissor[1], old_scissor[2], old_scissor[3]);

            glDeleteVertexArrays(1, &context->vertexArray);
            context->vertexArray = 0;
        });

        glUseProgram(context->shader.get());
        glBindTexture(GL_TEXTURE_2D, context->font.get());
        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(context->vertexArray);
        glBindBuffer(GL_ARRAY_BUFFER, context->arrayBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->elements);
        glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
        glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA,
                            GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glEnable(GL_BLEND);
        glDisable(GL_CULL_FACE);
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_SCISSOR_TEST);
        glViewport(viewport.x, viewport.y, viewport.z, viewport.w);

        {
            const float &W = drawData->DisplaySize.x;
            const float &H = drawData->DisplaySize.y;
            const float O1 = ((drawData->DisplayPos.x * 2) + W) / -W;
            const float O2 = ((drawData->DisplayPos.y * 2) + H) / H;
            const float orthoProj[] =
            {
                2.0f / W,   0.0f,       0.0f,   0.0f,
                0.0f,       2.0f / -H,  0.0f,   0.0f,
                0.0f,       0.0f,       -1.0f,  0.0f,
                O1,         O2,         0.0f,   1.0f
            };
            glUniformMatrix4fv(context->attribViewProj, 1, GL_FALSE, orthoProj);
        }
        glUniform1i(context->attribTex, 0);
        // #ifdef GL_SAMPLER_BINDING
        // glBindSampler(0, 0);
        // #endif

        glEnableVertexAttribArray(context->attribPos);
        glVertexAttribPointer(context->attribPos, 2, GL_FLOAT, GL_FALSE,
            sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));

        glEnableVertexAttribArray(context->attribUV);
        glVertexAttribPointer(context->attribUV, 2, GL_FLOAT, GL_FALSE,
            sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));

        glEnableVertexAttribArray(context->attribCol);
        glVertexAttribPointer(context->attribCol, 4, GL_UNSIGNED_BYTE, GL_TRUE,
            sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));

        for (int n = 0; n < drawData->CmdListsCount; ++n)
        {
            const ImDrawList *cmdList = drawData->CmdLists[n];
            const ImDrawIdx *idxBufferOffset = 0;

            glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)cmdList->VtxBuffer.Size * sizeof(ImDrawVert),
                (const GLvoid*)cmdList->VtxBuffer.Data, GL_STREAM_DRAW);

            glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)cmdList->IdxBuffer.Size * sizeof(ImDrawIdx),
                (const GLvoid*)cmdList->IdxBuffer.Data, GL_STREAM_DRAW);

            for (int cmdI = 0; cmdI < cmdList->CmdBuffer.Size; ++cmdI)
            {
                const ImDrawCmd &pcmd = cmdList->CmdBuffer[cmdI];
                if (pcmd.UserCallback)
                {
                    pcmd.UserCallback(cmdList, &pcmd);
                }
                else
                {
                    lak::vec4f_t clip;
                    clip.x = pcmd.ClipRect.x - viewport.x;
                    clip.y = pcmd.ClipRect.y - viewport.y;
                    clip.z = pcmd.ClipRect.z - viewport.x;
                    clip.w = pcmd.ClipRect.w - viewport.y;

                    if (clip.x < viewport.z &&
                        clip.y < viewport.w &&
                        clip.z >= 0.0f &&
                        clip.w >= 0.0f)
                    {
                        #ifdef GL_CLIP_ORIGIN
                        if (old_clip_origin == GL_UPPER_LEFT)
                            // Support for GL 4.5's glClipControl(GL_UPPER_LEFT)
                            glScissor(
                                (GLint)clip.x, (GLint)clip.y,
                                (GLint)clip.z, (GLint)clip.w
                            );
                        else
                        #endif
                        glScissor(
                            (GLint)clip.x,
                            (GLint)(viewport.w - clip.w),
                            (GLint)(clip.z - clip.x),
                            (GLint)(clip.w - clip.y)
                        );

                        glBindTexture(GL_TEXTURE_2D, (GLuint)(intptr_t)pcmd.TextureId);
                        glDrawElements(GL_TRIANGLES, (GLsizei)pcmd.ElemCount,
                            sizeof(ImDrawIdx) == 2 ? GL_UNSIGNED_SHORT : GL_UNSIGNED_INT,
                            idxBufferOffset);
                    }
                }
                idxBufferOffset += pcmd.ElemCount;
            }
        }
    }

    void ImplVkRender(ImplVkContext context, ImDrawData *drawData)
    {

    }

    void ImplRender(ImplContext context, ImDrawData *drawData)
    {
        switch (context->mode)
        {
            case GraphicsMode::SOFTWARE:
                ImplSRRender(context->srContext, drawData);
                break;
            case GraphicsMode::OPENGL:
                ImplGLRender(context->glContext, drawData);
                break;
            case GraphicsMode::VULKAN:
                ImplVkRender(context->vkContext, drawData);
                break;
            default:
                ASSERTF(false, "Invalid Context Mode");
                break;
        }

        ImGuiIO &io = ImGui::GetIO();
        for (size_t i = 0; i < 3; ++i)
            if (context->mouseRelease[i])
                io.MouseDown[i] = false;
    }

    void ImplSetClipboard(void *, const char *text)
    {
        SDL_SetClipboardText(text);
    }

    const char *ImplGetClipboard(char **clipboard)
    {
        if (*clipboard)
            SDL_free(*clipboard);
        *clipboard = SDL_GetClipboardText();
        return *clipboard;
    }
}

namespace lak
{
    bool HoriSplitter(float &top, float &bottom, float width, float topMin, float bottomMin, float length)
    {
        const float thickness = ImGui::GetStyle().FramePadding.x * 2;
        const float maxLeft = width - (bottomMin + thickness);
        if (top > maxLeft)
            top = maxLeft;
        bottom = width - (top + thickness);

        ImGuiContext &g = *GImGui;
        ImGuiWindow *window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;

        bb.Min.x = window->DC.CursorPos.x + top;
        bb.Min.y = window->DC.CursorPos.y;

        bb.Max = ImGui::CalcItemSize(ImVec2(thickness, length), 0.0f, 0.0f);
        bb.Max.x += bb.Min.x;
        bb.Max.y += bb.Min.y;

        return ImGui::SplitterBehavior(bb, id, ImGuiAxis_X, &top, &bottom, topMin, bottomMin);
    }

    bool VertSplitter(float &left, float &right, float width, float leftMin, float rightMin, float length)
    {
        const float thickness = ImGui::GetStyle().FramePadding.x * 2;
        const float maxTop = width - (rightMin + thickness);
        if (left > maxTop)
            left = maxTop;
        right = width - (left + thickness);

        ImGuiContext &g = *GImGui;
        ImGuiWindow *window = g.CurrentWindow;
        ImGuiID id = window->GetID("##Splitter");
        ImRect bb;

        bb.Min.x = window->DC.CursorPos.x;
        bb.Min.y = window->DC.CursorPos.y + left;

        bb.Max = ImGui::CalcItemSize(ImVec2(length, thickness), 0.0f, 0.0f);
        bb.Max.x += bb.Min.x;
        bb.Max.y += bb.Min.y;

        return ImGui::SplitterBehavior(bb, id, ImGuiAxis_Y, &left, &right, leftMin, rightMin);
    }

    bool TreeNode(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);

        ImGuiWindow* window = ImGui::GetCurrentWindow();
        if (window->SkipItems)
            return false;

        ImGuiContext &g = *GImGui;

        ImFormatStringV(g.TempBuffer, IM_ARRAYSIZE(g.TempBuffer), fmt, args);

        bool is_open = ImGui::TreeNodeBehavior(window->GetID(g.TempBuffer), 0, g.TempBuffer);

        va_end(args);
        return is_open;
    }
}

#include <imgui/misc/cpp/imgui_stdlib.cpp>
#include <imgui/imgui.cpp>
#include <imgui/imgui_widgets.cpp>
#include <imgui/imgui_draw.cpp>
#include <imgui/imgui_demo.cpp>