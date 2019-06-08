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

#include <stdint.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <exception>
#include <atomic>
#include <thread>
#include <future>
#include <filesystem>
namespace fs = std::filesystem;

// UI
#include "lak.h"
#include <strconv/strconv.hpp>
#include "imgui_impl_lak.h"
#include <imgui_memory_editor.h>
#include <imgui_stdlib.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#ifdef _WIN32
#define STBI_MSC_SECURE_CRT
#endif
#include <stb_image_write.h>

#include "tinflate.hpp"
#include "tinf.h"
#include "explorer.h"
#include "image.h"
namespace se = SourceExplorer;

#define APP_NAME "OpenGL Demo Application"

#ifndef MAIN_H
#define MAIN_H

void credits()
{
    ImGui::PushID("Credits");
    if (ImGui::TreeNode("ImGui"))
    {
        ImGui::Text(R"(https://github.com/ocornut/imgui

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
SOFTWARE.)");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("gl3w"))
    {
        ImGui::Text("https://github.com/skaslev/gl3w");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("SDL2"))
    {
        ImGui::Text("https://www.libsdl.org/");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("tinflate"))
    {
        ImGui::Text("http://achurch.org/tinflate.c");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("tinf"))
    {
        ImGui::Text("https://github.com/jibsen/tinf");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("stb_image_write"))
    {
        ImGui::Text("https://github.com/nothings/stb/blob/master/stb_image_write.h");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Tiny C Compiler"))
    {
        ImGui::Text(R"(https://www.bellard.org/tcc/

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA)");
        ImGui::TreePop();
    }
    if (ImGui::TreeNode("Anaconda/Chowdren"))
    {
        ImGui::Text(R"(https://github.com/Matt-Esch/anaconda

http://mp2.dk/anaconda/

http://mp2.dk/chowdren/

Anaconda is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Anaconda is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Anaconda.  If not, see <http://www.gnu.org/licenses/>.)");
        ImGui::TreePop();
    }
    ImGui::PopID();
}

#endif