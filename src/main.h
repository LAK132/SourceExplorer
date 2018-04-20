#include <cmath>
#include <iostream>
#include <string.h>
using std::string;
#include <iostream>
using std::cout;
using std::endl;
#include <exception>
using std::exception;
#include <memory>
using std::shared_ptr;
using std::make_shared;
#include <vector>
using std::vector;
#include <stddef.h>
#include <imgui.h>
#include <GL/gl3w.h>
#include <SDL.h>
#include <imgui_impl_sdl_gl3.h>
#include <imgui_memory_editor.h>
#include "explorer.h"

#define APP_NAME "OpenGL Application"
#define SDL_MAIN_HANDLED

#define STB_IMAGE_WRITE_IMPLEMENTATION
#ifdef _WIN32
#define STBI_MSC_SECURE_CRT
#endif
#include <stb_image_write.h>

#ifndef MAIN_H
#define MAIN_H

extern SDL_Window* window;
extern SDL_GLContext glContext;
extern float clearCol[4];
extern ImGuiIO* io;

void init();
void draw();
void loop();
void destroy();

void glakCredits()
{
    ImGui::PushID("Credits");
    if(ImGui::TreeNode("ImGui"))
    {
        ImGui::Text(R"(
https://github.com/ocornut/imgui

The MIT License (MIT)

Copyright (c) 2014-2018 Omar Cornut

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
        )");
        ImGui::TreePop();
    }
    if(ImGui::TreeNode("glm"))
    {
        ImGui::Text(R"(
https://github.com/g-truc/glm

The Happy Bunny License (Modified MIT License)

Copyright (c) 2005 - 2016 G-Truc Creation

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
associated documentation files (the "Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do so, subject to the
following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions
of the Software.

Restrictions: By making use of the Software for military purposes, you choose to make a Bunny unhappy.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.
        )");
        ImGui::TreePop();
    }
    if(ImGui::TreeNode("gl3w"))
    {
        ImGui::Text("https://github.com/skaslev/gl3w");
        ImGui::TreePop();
    }
    if(ImGui::TreeNode("SDL2"))
    {
        ImGui::Text("https://www.libsdl.org/");
        ImGui::TreePop();
    }
    if(ImGui::TreeNode("miniz"))
    {
        ImGui::Text("https://github.com/richgel999/miniz");
        ImGui::TreePop();
    }
    ImGui::PopID();
}

#endif