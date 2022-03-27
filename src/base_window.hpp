#ifndef BASE_WINDOW_HPP
#define BASE_WINDOW_HPP

#include "imgui_impl_lak.h"

#include <lak/opengl/texture.hpp>
#include <lak/string_literals.hpp>

#include "main.h"

#include "explorer.h"

template<typename DERIVED>
struct base_window
{
	static void credits()
	{
		ImGui::PushID("Credits");
		LAK_TREE_NODE("ImGui")
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
		}
		LAK_TREE_NODE("gl3w") { ImGui::Text("https://github.com/skaslev/gl3w"); }
#ifdef LAK_USE_SDL
		LAK_TREE_NODE("SDL2") { ImGui::Text("https://www.libsdl.org/"); }
#endif
		LAK_TREE_NODE("tinflate")
		{
			ImGui::Text("http://achurch.org/tinflate.c");
			ImGui::Text("Fork: https://github.com/LAK132/tinflate");
		}
		LAK_TREE_NODE("stb_image_write")
		{
			ImGui::Text(
			  "https://github.com/nothings/stb/blob/master/stb_image_write.h");
		}
		LAK_TREE_NODE("Anaconda/Chowdren")
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
		}
		ImGui::PopID();
	}

#ifdef LAK_COMPILER_MSVC
#	pragma warning(push)
// warning C4706: assignment within conditional expression
#	pragma warning(disable : 4706)
#endif
	static bool mode_select()
	{
		auto mode_check = [](se_main_mode_t mode, const char *mode_name) -> bool
		{
			bool result = false;
			if (bool set = se_main_mode == mode;
			    (result |= ImGui::Checkbox(mode_name, &set)) && set)
				se_main_mode = mode;
			return result;
		};

		return mode_check(se_main_mode_t::normal, "Normal Mode") |
		       mode_check(se_main_mode_t::byte_pairs, "Byte Pairs") |
		       mode_check(se_main_mode_t::testing, "Testing");
	}
#ifdef LAK_COMPILER_MSVC
#	pragma warning(pop)
#endif

	static void mode_select_menu()
	{
		if (ImGui::BeginMenu("Mode"))
		{
			mode_select();
			ImGui::EndMenu();
		}
	}

	static void debug_menu()
	{
		if (ImGui::BeginMenu("Debug"))
		{
			ImGui::Checkbox("Debug console (May make SE slow)",
			                &lak::debugger.live_output_enabled);
			if (lak::debugger.live_output_enabled)
			{
				ImGui::Checkbox("Only errors", &lak::debugger.live_errors_only);
				ImGui::Checkbox("Developer mode", &lak::debugger.line_info_enabled);
			}
			ImGui::EndMenu();
		}
	}

	static void menu_bar(float)
	{
		mode_select_menu();
		debug_menu();
	}

	static void left_region() {}

	static void right_region() {}

	static void main_region()
	{
		ImVec2 content_size = ImGui::GetWindowContentRegionMax();
		content_size.x      = ImGui::GetWindowContentRegionWidth();

		static float left_size  = content_size.x / 2;
		static float right_size = content_size.x / 2;

		lak::HoriSplitter(left_size, right_size, content_size.x);

		ImGui::BeginChild(
		  "Left", {left_size, -1}, true, ImGuiWindowFlags_NoSavedSettings);
		DERIVED::left_region();
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild(
		  "Right", {right_size, -1}, true, ImGuiWindowFlags_NoSavedSettings);
		DERIVED::right_region();
		ImGui::EndChild();
	}

	static void view_image(const lak::opengl::texture &texture,
	                       const float scale)
	{
		ImGui::Image((ImTextureID)(uintptr_t)texture.get(),
		             ImVec2(scale * (float)texture.size().x,
		                    scale * (float)texture.size().y));
	}

	static void image_memory_explorer(const byte_t *data,
	                                  size_t size,
	                                  bool update)
	{
		static bool reset_on_update      = true;
		static lak::vec2u64_t image_size = {256, 256};
		static lak::vec2u64_t block_skip = {0, 0};
		static lak::image4_t image{lak::vec2s_t(image_size)};
		static lak::opengl::texture texture(GL_TEXTURE_2D);
		static float scale            = 1.0f;
		static uint64_t from          = 0;
		static uint64_t to            = SIZE_MAX;
		static int colour_size        = 3;
		static const byte_t *old_data = data;

		if (data == nullptr && old_data == nullptr) return;

		if (data != nullptr && data != old_data)
		{
			old_data = data;
			update   = true;
		}

		if (update)
		{
			if (reset_on_update)
			{
				image_size = {256, 256};
				block_skip = {0, 0};
				image.resize(lak::vec2s_t(image_size));
			}

			if (SrcExp.view != nullptr && SrcExp.state.file != nullptr &&
			    data == SrcExp.state.file->data())
			{
				auto ref_span = SrcExp.view->ref_span;
				while (ref_span._source && ref_span._source != SrcExp.state.file)
					ref_span = ref_span.parent_span();
				if (!ref_span.empty())
				{
					from = ref_span.position().UNWRAP();
					to   = from + ref_span.size();
				}
				else
				{
					from = 0;
					to   = SIZE_MAX;
				}
			}
			else
			{
				from = 0;
				to   = SIZE_MAX;
			}
		}

		update |= !texture.get();

		{
			ImGui::Checkbox("Reset Configuration On Update", &reset_on_update);

			static bool count_mode = true;
			ImGui::Checkbox("Fixed Size", &count_mode);
			uint64_t count = to - from;

			if (ImGui::DragScalar("From", ImGuiDataType_U64, &from, 1.0f))
			{
				if (count_mode)
					to = from + count;
				else if (from > to)
					to = from;
				update = true;
			}
			if (ImGui::DragScalar("To", ImGuiDataType_U64, &to, 1.0f))
			{
				if (from > to) from = to;
				update = true;
			}
			const static uint64_t sizeMin = 0;
			const static uint64_t sizeMax = 10000;
			if (ImGui::DragScalarN("Image Size (Width/Height)",
			                       ImGuiDataType_U64,
			                       &image_size,
			                       2,
			                       1.0f,
			                       &sizeMin,
			                       &sizeMax))
			{
				image.resize(lak::vec2s_t(image_size));
				update = true;
			}
			if (ImGui::DragScalarN("For Every/Skip",
			                       ImGuiDataType_U64,
			                       &block_skip,
			                       2,
			                       0.1f,
			                       &sizeMin,
			                       &sizeMax))
			{
				update = true;
			}
			if (ImGui::SliderInt("Colour Size", &colour_size, 1, 4))
			{
				update = true;
			}
		}

		ImGui::Separator();

		if (from > to) from = to;
		if (from > size) from = size;
		if (to > size) to = size;

		if (update)
		{
			image.fill({0, 0, 0, 255});

			const auto begin = old_data;
			const auto end   = begin + to;
			auto it          = begin + from;

			auto out_img       = (byte_t *)image.data();
			const auto img_end = out_img + (image.contig_size() * sizeof(image[0]));

			for (uint64_t i = 1; out_img < img_end && it < end; ++i, ++it, ++out_img)
			{
				*out_img = *it;

				if (block_skip.x > 0 && (i % block_skip.x) == 0) it += block_skip.y;

				if (colour_size < 4 && (i % colour_size) == 0)
					out_img += 4 - colour_size;
			}

			if (colour_size < 4)
			{
				for (size_t sz = image.contig_size(); sz-- > 0;)
				{
					image[sz].a = 0xFF;
				}
			}

			texture.bind()
			  .apply(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
			  .apply(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER)
			  .apply(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
			  .apply(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
			  .build(0,
			         GL_RGBA,
			         (lak::vec2<GLsizei>)image.size(),
			         0,
			         GL_RGBA,
			         GL_UNSIGNED_BYTE,
			         image.data());
		}

		if (texture.get())
		{
			ImGui::DragFloat("Scale", &scale, 0.1f, 0.1f, 10.0f);
			ImGui::Separator();
			DERIVED::view_image(texture, scale);
		}
	}

	static void byte_pairs_memory_explorer(const byte_t *data,
	                                       size_t size,
	                                       bool update)
	{
		static lak::image<GLfloat> image(lak::vec2s_t(256, 256));
		static lak::opengl::texture texture(GL_TEXTURE_2D);
		static float scale            = 1.0f;
		static uint64_t from          = 0;
		static uint64_t to            = SIZE_MAX;
		static const byte_t *old_data = data;

		if (data == nullptr && old_data == nullptr) return;

		if (data != nullptr && data != old_data)
		{
			old_data = data;
			update   = true;
		}

		if (update)
		{
			if (SrcExp.view != nullptr && SrcExp.state.file != nullptr &&
			    data == SrcExp.state.file->data())
			{
				auto ref_span = SrcExp.view->ref_span;
				while (ref_span._source && ref_span._source != SrcExp.state.file)
					ref_span = ref_span.parent_span();
				if (!ref_span.empty())
				{
					from = ref_span.position().UNWRAP();
					to   = from + ref_span.size();
				}
				else
				{
					from = 0;
					to   = SIZE_MAX;
				}
			}
			else
			{
				from = 0;
				to   = SIZE_MAX;
			}
		}

		update |= !texture.get();

		{
			static bool count_mode = true;
			ImGui::Checkbox("Fixed Size", &count_mode);
			uint64_t count = to - from;

			if (ImGui::DragScalar("From", ImGuiDataType_U64, &from, 1.0f))
			{
				if (count_mode)
					to = from + count;
				else if (from > to)
					to = from;
				update = true;
			}
			if (ImGui::DragScalar("To", ImGuiDataType_U64, &to, 1.0f))
			{
				if (from > to) from = to;
				update = true;
			}
		}

		ImGui::Separator();

		if (from > to) from = to;
		if (from > size) from = size;
		if (to > size) to = size;

		if (update)
		{
			image.fill(0.0f);

			const auto begin = old_data;
			const auto end   = begin + to;
			auto it          = begin + from;

			const GLfloat step = 1.0f / ((to - from) / image.contig_size());
			for (uint8_t prev = (it != end ? uint8_t(*it) : 0); it != end;
			     prev         = uint8_t(*(it++)))
        image[{prev, uint8_t(*it)}] += step;

			texture.bind()
			  .apply(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER)
			  .apply(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER)
			  .apply(GL_TEXTURE_MIN_FILTER, GL_NEAREST)
			  .apply(GL_TEXTURE_MAG_FILTER, GL_NEAREST)
			  .build(0,
			         GL_RED,
			         (lak::vec2<GLsizei>)image.size(),
			         0,
			         GL_RED,
			         GL_FLOAT,
			         image.data());
		}

		if (texture.get())
		{
			ImGui::DragFloat("Scale", &scale, 0.1f, 0.1f, 10.0f);
			ImGui::Separator();
			base_window::view_image(texture, scale);
		}
	}

	static bool crypto()
	{
		bool updated   = false;
		int magic_char = se::_magic_char;
		if (ImGui::InputInt("Magic Char (u8)", &magic_char))
		{
			se::_magic_char = static_cast<uint8_t>(magic_char);
			se::GetEncryptionKey(SrcExp.state);
			updated = true;
		}
		if (ImGui::Button("Generate Crypto Key"))
		{
			se::GetEncryptionKey(SrcExp.state);
			updated = true;
		}
		return updated;
	}

	static void memory_explorer(bool &update)
	{
		if (!SrcExp.state.file) return;

		static const se::basic_entry_t *last = nullptr;
		static int data_mode                 = 0;
		static int content_mode              = 0;
		static bool raw                      = true;
		update |= last != SrcExp.view;

		update |= ImGui::RadioButton("EXE", &data_mode, 0);
		ImGui::SameLine();
		update |= ImGui::RadioButton("Header", &data_mode, 1);
		ImGui::SameLine();
		update |= ImGui::RadioButton("Data", &data_mode, 2);
		ImGui::SameLine();
		update |= ImGui::RadioButton("Magic Key", &data_mode, 3);
		ImGui::Separator();

		if (data_mode != 3)
		{
			if (data_mode != 0)
			{
				update |= ImGui::Checkbox("Raw", &raw);
				ImGui::SameLine();
			}
			update |= ImGui::RadioButton("Binary", &content_mode, 0);
			ImGui::SameLine();
			update |= ImGui::RadioButton("Byte Pairs", &content_mode, 1);
			ImGui::SameLine();
			update |= ImGui::RadioButton("Data Image", &content_mode, 2);
			ImGui::SameLine();
			SrcExp.binary_block.attempt |= ImGui::Button("Save Binary");
			ImGui::Separator();
		}

		if (data_mode == 0) // EXE
		{
			if (content_mode == 1)
			{
				byte_pairs_memory_explorer(
				  SrcExp.state.file->data(), SrcExp.state.file->size(), update);
			}
			else if (content_mode == 2)
			{
				image_memory_explorer(
				  SrcExp.state.file->data(), SrcExp.state.file->size(), update);
			}
			else
			{
				if (content_mode != 0) content_mode = 0;
				if (update) SrcExp.buffer = se::data_ref_span_t(SrcExp.state.file);
				SrcExp.editor.DrawContents(
				  reinterpret_cast<uint8_t *>(SrcExp.buffer.data()),
				  SrcExp.buffer.size());
				if (update && SrcExp.view != nullptr)
				{
					SCOPED_CHECKPOINT(__func__, "::EXE");
					auto ref_span = SrcExp.view->ref_span;
					while (ref_span._source && ref_span._source != SrcExp.state.file)
					{
						CHECKPOINT();
						ref_span = ref_span.parent_span();
					}
					if (ref_span._source && !ref_span.empty())
					{
						const auto from = ref_span.position().UNWRAP();
						SrcExp.editor.GotoAddrAndHighlight(from, from + ref_span.size());
						DEBUG("From: ", from);
					}
					else
					{
						ERROR("Memory does not appear in EXE");
					}
				}
			}
		}
		else if (data_mode == 1) // Head
		{
			if (update && SrcExp.view != nullptr)
				SrcExp.buffer =
				  raw ? SrcExp.view->head.data
				      : SrcExp.view->decode_head()
				          .or_else(
				            [&](const auto &err) -> se::result_t<se::data_ref_span_t>
				            {
					            ERROR(err);
					            return lak::ok_t{SrcExp.view->head.data};
				            })
				          .UNWRAP();

			if (content_mode == 1)
			{
				byte_pairs_memory_explorer(
				  SrcExp.buffer.data(), SrcExp.buffer.size(), update);
			}
			else if (content_mode == 2)
			{
				image_memory_explorer(
				  SrcExp.buffer.data(), SrcExp.buffer.size(), update);
			}
			else
			{
				if (content_mode != 0) content_mode = 0;
				SrcExp.editor.DrawContents(
				  reinterpret_cast<uint8_t *>(SrcExp.buffer.data()),
				  SrcExp.buffer.size());
				if (update) SrcExp.editor.GotoAddrAndHighlight(0, 0);
			}
		}
		else if (data_mode == 2) // Body
		{
			if (update && SrcExp.view != nullptr)
				SrcExp.buffer =
				  raw ? SrcExp.view->body.data
				      : SrcExp.view->decode_body()
				          .or_else(
				            [&](const auto &err) -> se::result_t<se::data_ref_span_t>
				            {
					            ERROR(err);
					            return lak::ok_t{SrcExp.view->body.data};
				            })
				          .UNWRAP();

			if (content_mode == 1)
			{
				byte_pairs_memory_explorer(
				  SrcExp.buffer.data(), SrcExp.buffer.size(), update);
			}
			else if (content_mode == 2)
			{
				image_memory_explorer(
				  SrcExp.buffer.data(), SrcExp.buffer.size(), update);
			}
			else
			{
				if (content_mode != 0) content_mode = 0;
				SrcExp.editor.DrawContents(
				  reinterpret_cast<uint8_t *>(SrcExp.buffer.data()),
				  SrcExp.buffer.size());
				if (update) SrcExp.editor.GotoAddrAndHighlight(0, 0);
			}
		}
		else if (data_mode == 3) // _magic_key
		{
			SrcExp.editor.DrawContents(&(se::_magic_key[0]), se::_magic_key.size());
			if (update) SrcExp.editor.GotoAddrAndHighlight(0, 0);
		}
		else
			data_mode = 0;

		last   = SrcExp.view;
		update = false;
	}

	static void image_explorer(bool &update)
	{
		static float scale = 1.0f;
		ImGui::DragFloat("Scale", &scale, 0.1f, 0.1f, 10.0f);
		ImGui::Separator();
		se::ViewImage(SrcExp, scale);
		update = false;
	}

	static void audio_explorer(bool &update)
	{
		struct audio_data_t
		{
			lak::u8string name;
			lak::u16string u16name;
			se::sound_mode_t type    = (se::sound_mode_t)0;
			uint32_t checksum        = 0;
			uint32_t references      = 0;
			uint32_t decomp_len      = 0;
			uint32_t reserved        = 0;
			uint32_t name_len        = 0;
			uint16_t format          = 0;
			uint16_t channel_count   = 0;
			uint32_t sample_rate     = 0;
			uint32_t byte_rate       = 0;
			uint16_t block_align     = 0;
			uint16_t bits_per_sample = 0;
			uint16_t unknown         = 0;
			uint32_t chunk_size      = 0;
			lak::array<byte_t> data;
		};

		static const se::basic_entry_t *last = nullptr;
		update |= last != SrcExp.view;

		static audio_data_t audio_data;
		if (update && SrcExp.view != nullptr)
		{
			CHECKPOINT();

			se::data_reader_t audio(SrcExp.view->decode_body().UNWRAP());
			audio_data = audio_data_t{};
			if (SrcExp.state.old_game)
			{
				CHECKPOINT();
				audio_data.checksum   = audio.read_u16().UNWRAP();
				audio_data.references = audio.read_u32().UNWRAP();
				audio_data.decomp_len = audio.read_u32().UNWRAP();
				audio_data.type       = (se::sound_mode_t)audio.read_u32().UNWRAP();
				audio_data.reserved   = audio.read_u32().UNWRAP();
				audio_data.name_len   = audio.read_u32().UNWRAP();

				audio_data.name =
				  audio.read_exact_c_str<char8_t>(audio_data.name_len).UNWRAP();

				if (audio_data.type == se::sound_mode_t::wave)
				{
					audio_data.format          = audio.read_u16().UNWRAP();
					audio_data.channel_count   = audio.read_u16().UNWRAP();
					audio_data.sample_rate     = audio.read_u32().UNWRAP();
					audio_data.byte_rate       = audio.read_u32().UNWRAP();
					audio_data.block_align     = audio.read_u16().UNWRAP();
					audio_data.bits_per_sample = audio.read_u16().UNWRAP();
					audio_data.unknown         = audio.read_u16().UNWRAP();
					audio_data.chunk_size      = audio.read_u32().UNWRAP();
					audio_data.data = audio.read<byte_t>(audio_data.chunk_size).UNWRAP();
				}
			}
			else
			{
				CHECKPOINT();
				se::data_reader_t header(SrcExp.view->decode_head().UNWRAP());

				audio_data.checksum   = header.read_u32().UNWRAP();
				audio_data.references = header.read_u32().UNWRAP();
				audio_data.decomp_len = header.read_u32().UNWRAP();
				audio_data.type       = (se::sound_mode_t)header.read_u32().UNWRAP();
				audio_data.reserved   = header.read_u32().UNWRAP();
				audio_data.name_len   = header.read_u32().UNWRAP();

				if (SrcExp.state.unicode)
				{
					audio_data.name = lak::to_u8string(
					  audio.read_exact_c_str<char16_t>(audio_data.name_len).UNWRAP());
				}
				else
				{
					audio_data.name = lak::to_u8string(
					  audio.read_exact_c_str<char8_t>(audio_data.name_len).UNWRAP());
				}

				DEBUG("Name: ", audio_data.name);

				if (const auto peek = audio.peek<char>(4).UNWRAP();
				    lak::string_view(lak::span(peek)) == "OggS"_view)
					audio_data.type = se::sound_mode_t::oggs;
				else if (lak::string_view(lak::span(peek)) != "RIFF"_view)
					audio_data.type = se::sound_mode_t(-1);

				if (audio_data.type == se::sound_mode_t::wave)
				{
					audio.skip(4).UNWRAP(); // "RIFF"
					uint32_t size = audio.read_s32().UNWRAP() + 4;
					audio.skip(8).UNWRAP(); // "WAVEfmt "
					// audio.position += 4; // 0x00000010
					// 16, 18 or 40
					uint32_t chunk_size = audio.read_u32().UNWRAP();
					DEBUG("Chunk Size ", chunk_size);
					const size_t pos           = audio.position() + chunk_size;
					audio_data.format          = audio.read_u16().UNWRAP(); // 2
					audio_data.channel_count   = audio.read_u16().UNWRAP(); // 4
					audio_data.sample_rate     = audio.read_u32().UNWRAP(); // 8
					audio_data.byte_rate       = audio.read_u32().UNWRAP(); // 12
					audio_data.block_align     = audio.read_u16().UNWRAP(); // 14
					audio_data.bits_per_sample = audio.read_u16().UNWRAP(); // 16
					if (chunk_size >= 18)
					{
						[[maybe_unused]] uint16_t extensionSize =
						  audio.read_u16().UNWRAP(); // 18
						DEBUG("Extension Size ", extensionSize);
					}
					if (chunk_size >= 40)
					{
						[[maybe_unused]] uint16_t validPerSample =
						  audio.read_u16().UNWRAP(); // 20
						DEBUG("Valid Bits Per Sample ", validPerSample);
						[[maybe_unused]] uint32_t channelMask =
						  audio.read_u32().UNWRAP(); // 24
						DEBUG("Channel Mask ", channelMask);
						// SubFormat // 40
					}
					audio.seek(pos + 4).UNWRAP(); // "data"
					audio_data.chunk_size = audio.read_u32().UNWRAP();
					DEBUG("Pos: ", audio.position());
					DEBUG("Remaining: ", audio.remaining().size());
					DEBUG("Size: ", size);
					DEBUG("Chunk Size: ", audio_data.chunk_size);
					audio_data.data = audio.read<byte_t>(audio_data.chunk_size).UNWRAP();
				}
			}
		}

#if defined(LAK_USE_SDL)
		static size_t audio_size = 0;
		static bool playing      = false;
		static SDL_AudioSpec audio_spec;
		static SDL_AudioDeviceID audio_device = 0;
		static SDL_AudioSpec audio_specGot;

		if (!playing && ImGui::Button("Play"))
		{
			SDL_AudioSpec spec;
			spec.freq = audio_data.sample_rate;
			// spec.freq = audio_data.byte_rate;
			switch (audio_data.format)
			{
				case 0x0001: spec.format = AUDIO_S16; break;
				case 0x0003: spec.format = AUDIO_F32; break;
				case 0x0006:
					spec.format = AUDIO_S8; /*8bit A-law*/
					break;
				case 0x0007:
					spec.format = AUDIO_S8; /*abit mu-law*/
					break;
				case 0xFFFE: /*subformat*/ break;
				default: break;
			}
			spec.channels = static_cast<Uint8>(audio_data.channel_count);
			spec.samples  = 2048;
			spec.callback = nullptr;

			if (lak::as_bytes(&audio_spec) != lak::as_bytes(&spec))
			{
				lak::memcpy(&audio_spec, &spec);
				if (audio_device != 0)
				{
					SDL_CloseAudioDevice(audio_device);
					audio_device = 0;
				}
			}

			if (audio_device == 0)
				audio_device =
				  SDL_OpenAudioDevice(nullptr, false, &audio_spec, &audio_specGot, 0);

			audio_size = audio_data.data.size();
			SDL_QueueAudio(
			  audio_device, audio_data.data.data(), static_cast<Uint32>(audio_size));
			SDL_PauseAudioDevice(audio_device, 0);
			playing = true;
		}

		if (playing &&
		    (ImGui::Button("Stop") || (SDL_GetQueuedAudioSize(audio_device) == 0)))
		{
			SDL_PauseAudioDevice(audio_device, 1);
			SDL_ClearQueuedAudio(audio_device);
			audio_size = 0;
			playing    = false;
		}

		if (audio_size > 0)
			ImGui::ProgressBar(1.0f - float(SDL_GetQueuedAudioSize(audio_device) /
			                                (double)audio_size));
		else
			ImGui::ProgressBar(0);
#endif

		ImGui::Text("Name: %s",
		            reinterpret_cast<const char *>(audio_data.name.c_str()));
		ImGui::Text("Type: ");
		ImGui::SameLine();
		switch (audio_data.type)
		{
			case se::sound_mode_t::wave: ImGui::Text("WAV"); break;
			case se::sound_mode_t::midi: ImGui::Text("MIDI"); break;
			case se::sound_mode_t::oggs: ImGui::Text("OGG"); break;
			default: ImGui::Text("Unknown"); break;
		}
		ImGui::Text("Data Size: 0x%zX", (size_t)audio_data.data.size());
		ImGui::Text("Format: 0x%zX", (size_t)audio_data.format);
		ImGui::Text("Channel Count: %zu", (size_t)audio_data.channel_count);
		ImGui::Text("Sample Rate: %zu", (size_t)audio_data.sample_rate);
		ImGui::Text("Byte Rate: %zu", (size_t)audio_data.byte_rate);
		ImGui::Text("Block Align: 0x%zX", (size_t)audio_data.block_align);
		ImGui::Text("Bits Per Sample: %zu", (size_t)audio_data.bits_per_sample);
		ImGui::Text("Chunk Size: 0x%zX", (size_t)audio_data.chunk_size);

		last   = SrcExp.view;
		update = false;
	}

	static void draw(float frame_time)
	{
		if (ImGui::BeginMenuBar())
		{
			DERIVED::menu_bar(frame_time);
			ImGui::EndMenuBar();
		}

		DERIVED::main_region();
	}
};

#endif
