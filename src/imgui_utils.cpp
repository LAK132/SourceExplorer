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

#include "imgui_utils.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include <misc/cpp/imgui_stdlib.h>

#include <misc/cpp/imgui_stdlib.cpp>

bool lak::input_text(const char *str_id,
                     char *buf,
                     size_t buf_size,
                     ImGuiInputTextFlags flags,
                     ImGuiInputTextCallback callback,
                     void *user_data)
{
	bool result;
	ImGui::PushID(str_id);
	result = ImGui::InputTextEx(
	  "", "", buf, (int)buf_size, ImVec2(-1, 0), flags, callback, user_data);
	ImGui::PopID();
	return result;
}

bool lak::input_text(const char *str_id,
                     lak::astring *str,
                     ImGuiInputTextFlags flags,
                     ImGuiInputTextCallback callback,
                     void *user_data)
{
	IM_ASSERT(!(flags & ImGuiInputTextFlags_CallbackResize));
	flags |= ImGuiInputTextFlags_CallbackResize;

	InputTextCallback_UserData cb_user_data;
	cb_user_data.Str                   = str;
	cb_user_data.ChainCallback         = callback;
	cb_user_data.ChainCallbackUserData = user_data;

	return lak::input_text(str_id,
	                       (char *)str->c_str(),
	                       str->capacity() + 1,
	                       flags,
	                       InputTextCallback,
	                       &cb_user_data);
}
