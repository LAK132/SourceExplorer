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

#include "imgui_impl_lak.h"
#include <imgui_memory_editor.h>
#include <imgui_stdlib.h>

#include "explorer.h"
#include "tostring.hpp"

#include <lak/strconv.hpp>
#include <lak/tinflate.hpp>

#include <atomic>
#include <exception>
#include <filesystem>
#include <fstream>
#include <future>
#include <iostream>
#include <queue>
#include <stdint.h>
#include <thread>
#include <unordered_set>
#include <vector>

#include "git.hpp"

#define APP_VERSION GIT_TAG "-" GIT_HASH

#define APP_NAME "Source Explorer " STRINGIFY(LAK_ARCH) " " APP_VERSION

#ifndef SOURCE_EXPLORER_MAIN_H
#	define SOURCE_EXPLORER_MAIN_H

namespace se = SourceExplorer;
namespace fs = std::filesystem;

extern se::source_explorer_t SrcExp;
extern int opengl_major, opengl_minor;

enum struct se_main_mode_t : unsigned int
{
	normal,
	byte_pairs,
};

extern se_main_mode_t se_main_mode;

#endif
