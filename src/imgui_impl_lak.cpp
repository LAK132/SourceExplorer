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

namespace ImGui
{
    ImplContext ImplCreateContext(GraphicsMode mode)
    {
        ImplContext result = (ImplContext)std::malloc(sizeof(_ImplContext));
        result->mode = mode;
        switch (mode)
        {
            case SOFTWARE:
                result->vdContext = std::malloc(sizeof(_ImplSRContext));
                break;
            case OPENGL:
                result->vdContext = std::malloc(sizeof(_ImplGLContext));
                break;
            case VULKAN:
                result->vdContext = std::malloc(sizeof(_ImplVkContext));
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
                std::free(context->vdContext);
            std::free(context);
        }
    }

    inline void UpdateDisplaySize(const lak::vec2f_t window, const lak::vec2f_t display)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.DisplaySize.x = window.x;
        io.DisplaySize.y = window.y;
        io.DisplayFramebufferScale.x = (window.x > 0) ? (display.x / window.x) : 0;
        io.DisplayFramebufferScale.y = (window.y > 0) ? (display.y / window.y) : 0;
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

    }

    void ImplInitGLContext(ImplGLContext context, const lak::window_t &window)
    {
        {
            int windW, windH, dispW, dispH;
            SDL_GetWindowSize(window.window, &windW, &windH);
            SDL_GL_GetDrawableSize(window.window, &dispW, &dispH);
            UpdateDisplaySize({(float)windW, (float)windH}, {(float)dispW, (float)dispH});
        }

        context->state.program                   = 0;
        context->state.texture                   = 0;
        context->state.activeTexture             = GL_TEXTURE0;
        context->state.sampler                   = 0;
        context->state.vertexArray               = 0;
        context->state.arrayBuffer               = 0;
        context->state.polygonMode[0]            = GL_FRONT_AND_BACK;
        context->state.polygonMode[1]            = GL_FILL;
        context->state.clipOrigin                = {};
        context->state.viewport                  = {};
        context->state.scissorBox                = {};

        context->state.blendRGB.source           = GL_SRC_ALPHA;
        context->state.blendRGB.destination      = GL_ONE_MINUS_SRC_ALPHA;
        context->state.blendRGB.equation         = GL_FUNC_ADD;

        context->state.blendAlpha.source         = GL_SRC_ALPHA;
        context->state.blendAlpha.destination    = GL_ONE_MINUS_SRC_ALPHA;
        context->state.blendAlpha.equation       = GL_FUNC_ADD;

        context->state.enableBlend               = GL_TRUE;
        context->state.enableCullFace            = GL_FALSE;
        context->state.enableDepthTest           = GL_FALSE;
        context->state.enableScissorTest         = GL_TRUE;

        lak::glState_t backupState; backupState.backup();

        const GLchar *vertShader =
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
            "}\n";

        context->vertShader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(context->vertShader, 1, &vertShader, nullptr);
        glCompileShader(context->vertShader);
        // TODO: Check Shader

        const GLchar *fragShader =
            "#version 130\n"
            "uniform sampler2D fTexture;\n"
            "in vec2 fUV;\n"
            "in vec4 fColor;\n"
            "out vec4 pColor;\n"
            "void main()\n"
            "{\n"
            "   pColor = fColor * texture(fTexture, fUV.st);\n"
            "}\n";

        context->fragShader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(context->fragShader, 1, &fragShader, nullptr);
        glCompileShader(context->fragShader);
        // TODO: Check Shader

        context->state.program = glCreateProgram();
        glAttachShader(context->state.program, context->vertShader);
        glAttachShader(context->state.program, context->fragShader);
        glLinkProgram(context->state.program);

        context->attribTex       = glGetUniformLocation(context->state.program, "fTexture");
        context->attribViewProj  = glGetUniformLocation(context->state.program, "viewProj");
        context->attribPos       = glGetAttribLocation(context->state.program, "vPosition");
        context->attribUV        = glGetAttribLocation(context->state.program, "vUV");
        context->attribCol       = glGetAttribLocation(context->state.program, "vColor");

        glGenBuffers(1, &context->state.arrayBuffer);
        glGenBuffers(1, &context->elements);

        ImGuiIO &io = ImGui::GetIO();

        // Create fonts texture
        uint8_t *pixels;
        int width, height;
        io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

        glGenTextures(1, &context->state.texture);
        glBindTexture(GL_TEXTURE_2D, context->state.texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

        io.Fonts->TexID = (ImTextureID)(intptr_t)context->state.texture;

        backupState.restore();
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
            case SOFTWARE:
                ImplInitSRContext(context->srContext, window);
                break;
            case OPENGL:
                ImplInitGLContext(context->glContext, window);
                break;
            case VULKAN:
                ImplInitVkContext(context->vkContext, window);
                break;
            default:
                assert(false);
                break;
        }

        #ifdef _WIN32
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(window.window, &wmInfo);
        ImGui::GetIO().ImeWindowHandle = wmInfo.info.win.window;
        #endif
    }

    void ImplShutdownSRContext(ImplSRContext context)
    {

    }

    void ImplShutdownGLContext(ImplGLContext context)
    {
        if (context->state.arrayBuffer) glDeleteBuffers(1, &context->state.arrayBuffer);
        context->state.arrayBuffer = 0;

        if (context->elements) glDeleteBuffers(1, &context->elements);
        context->elements = 0;

        if (context->state.program && context->vertShader) glDetachShader(context->state.program, context->vertShader);
        if (context->vertShader) glDeleteShader(context->vertShader);
        context->vertShader = 0;

        if (context->state.program && context->fragShader) glDetachShader(context->state.program, context->fragShader);
        if (context->fragShader) glDeleteShader(context->fragShader);
        context->fragShader = 0;

        if (context->state.program) glDeleteProgram(context->state.program);
        context->state.program = 0;

        // Destroy Fonts Texture

        if (context->state.texture)
        {
            ImGuiIO &io = ImGui::GetIO();
            glDeleteTextures(1, &context->state.texture);
            io.Fonts->TexID = 0;
            context->state.texture = 0;
        }
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
            case SOFTWARE:
                ImplShutdownSRContext(context->srContext);
                break;
            case OPENGL:
                ImplShutdownGLContext(context->glContext);
                break;
            case VULKAN:
                ImplShutdownVkContext(context->vkContext);
                break;
            default:
                assert(false);
                break;
        }

        DestroyContext(context->imContext);
    }

    void ImplSetCurrentContext(ImplContext context)
    {
        SetCurrentContext(context->imContext);
    }

    void ImplNewFrame(ImplContext context, SDL_Window *window, const float deltaTime, const bool callBaseNewFrame)
    {
        ImGuiIO &io = ImGui::GetIO();

        assert(io.Fonts->IsBuilt() && "Font atlas not build");

        assert(deltaTime > 0);
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
                    int dispW, dispH;
                    SDL_GL_GetDrawableSize(SDL_GetWindowFromID(event.window.windowID), &dispW, &dispH);
                    UpdateDisplaySize({(float)event.window.data1, (float)event.window.data2}, {(float)dispW, (float)dispH});
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
                assert(key >= 0 && key < IM_ARRAYSIZE(io.KeysDown));
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

    }

    void ImplGLRender(ImplGLContext context, ImDrawData *drawData)
    {
        assert(drawData != nullptr);
        ImGuiIO &io = ImGui::GetIO();

        lak::vec4f_t viewport;
        // int = float = float
        context->state.viewport.x = (GLint)(viewport.x = drawData->DisplayPos.x);
        context->state.viewport.y = (GLint)(viewport.y = drawData->DisplayPos.y);
        context->state.viewport.w = (GLint)(viewport.z = drawData->DisplaySize.x * io.DisplayFramebufferScale.x);
        context->state.viewport.h = (GLint)(viewport.w = drawData->DisplaySize.y * io.DisplayFramebufferScale.y);
        if (context->state.viewport.w <= 0 || context->state.viewport.h <= 0)
            return;

        drawData->ScaleClipRects(io.DisplayFramebufferScale);

        lak::glState_t backupState; backupState.backup();

        glGenVertexArrays(1, &context->state.vertexArray);

        context->state.restore(); // calls glBindVertexArray

        {
            const float &W = drawData->DisplaySize.x;
            const float &H = drawData->DisplaySize.y;
            const float O1 = ((drawData->DisplayPos.x * 2) + W) / -W;
            const float O2 = ((drawData->DisplayPos.y * 2) + H) / H;
            const float orthoProj[4][4] =
            {
                { 2.0f/W,   0.0f,       0.0f,   0.0f },
                { 0.0f,     2.0f/-H,    0.0f,   0.0f },
                { 0.0f,     0.0f,       -1.0f,  0.0f },
                { O1,       O2,         0.0f,   1.0f }
            };
            glUniformMatrix4fv(context->attribViewProj, 1, GL_FALSE, &orthoProj[0][0]);
        }
        glUniform1i(context->attribTex, 0);
        #ifdef GL_SAMPLER_BINDING
        glBindSampler(0, 0);
        #endif

        glEnableVertexAttribArray(context->attribPos);
        glVertexAttribPointer(context->attribPos, 2, GL_FLOAT, GL_FALSE,
            sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, pos));

        glEnableVertexAttribArray(context->attribUV);
        glVertexAttribPointer(context->attribUV, 2, GL_FLOAT, GL_FALSE,
            sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, uv));

        glEnableVertexAttribArray(context->attribCol);
        glVertexAttribPointer(context->attribCol, 4, GL_UNSIGNED_BYTE, GL_TRUE,
            sizeof(ImDrawVert), (GLvoid*)IM_OFFSETOF(ImDrawVert, col));

        glBindBuffer(GL_ARRAY_BUFFER, context->state.arrayBuffer);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, context->elements);

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
                        if (backupState.clipOrigin == GL_UPPER_LEFT)
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

        backupState.restore();

        glDeleteVertexArrays(1, &context->state.vertexArray);
        context->state.vertexArray = 0;
    }

    void ImplVkRender(ImplVkContext context, ImDrawData *drawData)
    {

    }

    void ImplRender(ImplContext context, ImDrawData *drawData)
    {
        switch (context->mode)
        {
            case SOFTWARE:
                ImplSRRender(context->srContext, drawData);
                break;
            case OPENGL:
                ImplGLRender(context->glContext, drawData);
                break;
            case VULKAN:
                ImplVkRender(context->vkContext, drawData);
                break;
            default:
                assert(false);
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
    bool OpenPath(fs::path &path, bool &good, bool file, bool save)
    {
        bool done = false;
        bool open = true;

        // static string filename = "";
        static fs::path filename = "";

        const char *name;
        if (file && save) name = "Save File";
        else if (file) name = "Open File";
        else if (save) name = "Save Folder";
        else name = "Open Folder";

        ImGui::SetNextWindowSizeConstraints({200, 300}, {1000, 1400});
        if (ImGui::BeginPopupModal(name, &open, ImGuiWindowFlags_NoSavedSettings))
        {
            while (path.has_parent_path() && !fs::is_directory(path.parent_path()))
                path = path.parent_path();

            if (!(fs::is_directory(path) || (path.has_parent_path() && fs::is_directory(path.parent_path()))))
                path = fs::current_path();

            bool hasParent = path.has_parent_path();
            bool isDir = fs::is_directory(path);
            fs::path p = (isDir ? path : (hasParent ? path.parent_path() : fs::current_path()));

            const float footer = ImGui::GetStyle().ItemSpacing.y + (ImGui::GetFrameHeightWithSpacing() * 2);

            if ((hasParent && isDir) || (hasParent && path.parent_path().has_parent_path()))
            {
                if (ImGui::Button("<- Back"))
                {
                    path = (path/"..").lexically_normal();
                }
            }

            if (lak::InputPath("directory", p))
                path = p/filename;

            // ImGui::Text("%s", p.u8string().c_str());
            ImGui::Separator();
            if (ImGui::BeginChild("Viewer", ImVec2(0, -footer)))
            {
                for(auto& d : fs::directory_iterator(p))
                {
                    fs::path dp = d.path();
                    bool selected = path == dp;
                    if (ImGui::Selectable(dp.filename().u8string().c_str(), &selected))
                    {
                        if (fs::is_directory(dp))
                        {
                            p = dp;
                            path = file ? p/filename : p;
                            break;
                        }
                        else if (file)
                        {
                            filename = dp.filename().u8string();
                            path = p/filename;
                        }
                    }
                }
            }
            ImGui::EndChild();

            ImGui::Separator();

            if (lak::InputPath("filename", filename))
            {
                path = p/filename;
            }

            // Enable open button if file selected (in file mode) or directory selected (if not in file mode)
            isDir = fs::is_directory(path);
            hasParent = path.has_parent_path();

            if (!save && (file ? !isDir : (isDir || hasParent)))
            {
                if (ImGui::Button("Open"))
                {
                    // Get file parent directory if not in file mode
                    if (!file && !isDir)
                        path = path.parent_path();
                    done = true;
                    good = true;
                    filename = "";
                }
                ImGui::SameLine();
            }
            else if (save && !isDir && hasParent)
            {
                if (ImGui::Button("Save"))
                {
                    // Get file parent directory if not in file mode
                    if (!file && !isDir)
                        path = path.parent_path();
                    done = true;
                    good = true;
                    filename = "";
                }
                ImGui::SameLine();
            }

            if (ImGui::Button("Cancel"))
            {
                done = true;
                good = false;
                filename = "";
            }
            ImGui::EndPopup();
        }

        if (!ImGui::IsPopupOpen(name))
            ImGui::OpenPopup(name);

        if (!open)
        {
            good = false;
            return true;
        }

        return done;
    }

    bool LongestDirectoryPath(fs::path &path)
    {
        if (fs::is_directory(path))
            return false;

        path = path.lexically_normal();

        while (!fs::is_directory(path) && path.has_parent_path())
            path = path.parent_path();

        if (!fs::is_directory(path))
            path = fs::current_path();

        return true;
    }

    fs::path LongestDirectoryPathCopy(fs::path &path)
    {
        fs::path result = path;
        LongestDirectoryPath(result);
        return result;
    }

    bool CorrectPath(fs::path &root, fs::path &filename)
    {
        if (!fs::is_directory(root))
        {
            filename = root.filename();
            lak::LongestDirectoryPath(root);
            return true;
        }
        if (fs::is_directory(root/filename))
        {
            root /= filename;
            filename = "";
            return true;
        }
        else if (filename.has_parent_path())
        {
            root /= filename;
            filename = root.filename();
            root = root.lexically_normal().parent_path();

            lak::LongestDirectoryPath(root);
            return true;
        }
        else return lak::LongestDirectoryPath(root);
        return false;
    }

    bool DirectoryList(const char *str_id, fs::path &path, ImVec2 size_arg = ImVec2(0, 0))
    {
        if (ImGui::BeginChild(str_id, size_arg))
        {
            for (auto &dir : fs::directory_iterator(LongestDirectoryPathCopy(path)))
            {
                if (ImGui::Selectable(dir.path().filename().string().c_str()))
                {
                    path = dir.path();
                    ImGui::EndChild();
                    return true;
                }
            }
        }
        ImGui::EndChild();
        return false;
    }

    bool DirectoryViewer(const char *str_id, fs::path &path, ImVec2 size_arg = ImVec2(0, 0))
    {
        bool result = false;
        ImGui::PushID(str_id);

        if (path.has_parent_path())
        {
            if (ImGui::Button("<- Back"))
            {
                path /= "..";
                if (!lak::LongestDirectoryPath(path))
                    path = path.lexically_normal();
                result = true;
            }
            ImGui::SameLine();
        }

        if (lak::InputPath("string", path))
        {
            if (!lak::LongestDirectoryPath(path))
                path = path.lexically_normal();
            result = true;
        }

        ImGui::Separator();

        result |= lak::DirectoryList("list", path, size_arg);

        ImGui::PopID();

        return result;
    }

    bool SaveFile(fs::path &path, bool &good)
    {
        fs::path file;
        lak::CorrectPath(path, file);
        bool result = SaveFile(path, file, good);
        path /= file;
        return result;
    }

    bool SaveFile(fs::path &path, fs::path &file, bool &good)
    {
        bool done = false;
        bool open = true;
        const char name[] = "Save File";

        ImGui::SetNextWindowSizeConstraints({200, 300}, {1000, 1400});
        if (ImGui::BeginPopupModal(name, &open, ImGuiWindowFlags_NoSavedSettings))
        {
            const ImVec2 viewSize = ImVec2(0, -(ImGui::GetStyle().ItemSpacing.y + (ImGui::GetFrameHeightWithSpacing() * 2)));

            if (DirectoryViewer("viewer", path, viewSize))
            {
                if (!fs::is_directory(path))
                    lak::CorrectPath(path, file);
            }

            ImGui::Separator();

            if (lak::InputPath("filename", file))
            {
                lak::CorrectPath(path, file);
            }

            if (fs::is_directory(path) && !fs::is_empty(file))
            {
                if (ImGui::Button("Save"))
                {
                    done = true;
                    good = true;
                }
                ImGui::SameLine();
            }

            if (ImGui::Button("Cancel"))
            {
                done = true;
                good = false;
            }

            ImGui::EndPopup();
        }

        if (open && !ImGui::IsPopupOpen(name))
        {
            // we should only reach this once per opening,
            // so we can do a one-off CorrectPath here
            CorrectPath(path, file);
            ImGui::OpenPopup(name);
        }

        if (!open)
        {
            good = false;
            return true;
        }

        return done;
    }

    bool OpenFile(fs::path &path, bool &good)
    {
        fs::path file;
        lak::CorrectPath(path, file);
        bool result = OpenFile(path, file, good);
        path /= file;
        return result;
    }

    bool OpenFile(fs::path &path, fs::path &file, bool &good)
    {
        bool done = false;
        bool open = true;
        const char name[] = "Open File";

        ImGui::SetNextWindowSizeConstraints({200, 300}, {1000, 1400});
        if (ImGui::BeginPopupModal(name, &open, ImGuiWindowFlags_NoSavedSettings))
        {
            const ImVec2 viewSize = ImVec2(0, -(ImGui::GetStyle().ItemSpacing.y + (ImGui::GetFrameHeightWithSpacing() * 2)));

            if (DirectoryViewer("viewer", path, viewSize))
            {
                if (!fs::is_directory(path))
                    lak::CorrectPath(path, file);
            }

            ImGui::Separator();

            if (lak::InputPath("filename", file))
            {
                lak::CorrectPath(path, file);
            }

            if (fs::is_directory(path) && fs::exists(path/file) && !fs::is_directory(path/file))
            {
                if (ImGui::Button("Open"))
                {
                    done = true;
                    good = true;
                }
                ImGui::SameLine();
            }

            if (ImGui::Button("Cancel"))
            {
                done = true;
                good = false;
            }

            ImGui::EndPopup();
        }

        if (open && !ImGui::IsPopupOpen(name))
        {
            // we should only reach this once per opening,
            // so we can do a one-off CorrectPath here
            CorrectPath(path, file);
            ImGui::OpenPopup(name);
        }

        if (!open)
        {
            good = false;
            return true;
        }

        return done;
    }

    bool SaveFolder(fs::path &path, bool &good)
    {
        fs::path file;
        lak::CorrectPath(path, file);
        bool result = SaveFolder(path, file, good);
        path /= file;
        return result;
    }

    bool SaveFolder(fs::path &path, fs::path &file, bool &good)
    {
        bool done = false;
        bool open = true;
        const char name[] = "Save Folder";

        ImGui::SetNextWindowSizeConstraints({200, 300}, {1000, 1400});
        if (ImGui::BeginPopupModal(name, &open, ImGuiWindowFlags_NoSavedSettings))
        {
            const ImVec2 viewSize = ImVec2(0, -(ImGui::GetStyle().ItemSpacing.y + (ImGui::GetFrameHeightWithSpacing() * 2)));

            if (DirectoryViewer("viewer", path, viewSize))
            {
                if (!fs::is_directory(path))
                    lak::CorrectPath(path, file);
            }

            ImGui::Separator();

            if (lak::InputPath("filename", file))
            {
                lak::CorrectPath(path, file);
            }

            if (fs::is_directory(path) && !fs::is_empty(file))
            {
                if (ImGui::Button("Save"))
                {
                    done = true;
                    good = true;
                }
                ImGui::SameLine();
            }

            if (ImGui::Button("Cancel"))
            {
                done = true;
                good = false;
            }

            ImGui::EndPopup();
        }

        if (open && !ImGui::IsPopupOpen(name))
        {
            // we should only reach this once per opening,
            // so we can do a one-off CorrectPath here
            CorrectPath(path, file);
            ImGui::OpenPopup(name);
        }

        if (!open)
        {
            good = false;
            return true;
        }

        return done;
    }

    bool OpenFolder(fs::path &path, bool &good)
    {
        bool done = false;
        bool open = true;
        const char name[] = "Open Folder";

        ImGui::SetNextWindowSizeConstraints({200, 300}, {1000, 1400});
        if (ImGui::BeginPopupModal(name, &open, ImGuiWindowFlags_NoSavedSettings))
        {
            const ImVec2 viewSize = ImVec2(0, -(ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing()));

            if (DirectoryViewer("viewer", path, viewSize))
            {
                if (!fs::is_directory(path))
                    lak::LongestDirectoryPath(path);
            }

            ImGui::Separator();

            if (fs::is_directory(path))
            {
                if (ImGui::Button("Open"))
                {
                    done = true;
                    good = true;
                }
                ImGui::SameLine();
            }

            if (ImGui::Button("Cancel"))
            {
                done = true;
                good = false;
            }

            ImGui::EndPopup();
        }

        if (open && !ImGui::IsPopupOpen(name))
        {
            // we should only reach this once per opening,
            // so we can do a one-off CorrectPath here
            lak::LongestDirectoryPath(path);
            ImGui::OpenPopup(name);
        }

        if (!open)
        {
            good = false;
            return true;
        }

        return done;
    }

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

    bool InputPath(const char *str_id, fs::path &path)
    {
        std::string str = path.string();
        lak::InputText(str_id, &str);
        if (ImGui::IsItemEdited())
            path = str;
        return ImGui::IsItemDeactivatedAfterEdit();
    }
}

#include <imgui/misc/cpp/imgui_stdlib.cpp>

namespace lak
{
    bool InputText(const char *str_id, char* buf, size_t buf_size, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
    {
        IM_ASSERT(!(flags & ImGuiInputTextFlags_Multiline)); // call InputTextMultiline()
        bool result;
        ImGui::PushID(str_id);
        result = ImGui::InputTextEx("", buf, (int)buf_size, ImVec2(-1,0), flags, callback, user_data);
        // result = ImGui::InputTextEx((std::string("##")+str_id).c_str(), buf, (int)buf_size, ImVec2(-1,0), flags, callback, user_data);
        ImGui::PopID();
        return result;
    }

    bool InputText(const char *str_id, std::string* str, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* user_data)
    {
        IM_ASSERT((flags & ImGuiInputTextFlags_CallbackResize) == 0);
        flags |= ImGuiInputTextFlags_CallbackResize;

        InputTextCallback_UserData cb_user_data;
        cb_user_data.Str = str;
        cb_user_data.ChainCallback = callback;
        cb_user_data.ChainCallbackUserData = user_data;

        return lak::InputText(str_id, (char*)str->c_str(), str->capacity() + 1, flags, InputTextCallback, &cb_user_data);
    }
}

#include <imgui/imgui.cpp>
#include <imgui/imgui_widgets.cpp>
#include <imgui/imgui_draw.cpp>
#include <imgui/imgui_demo.cpp>