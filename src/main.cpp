// Copyright (c) Mathias Kaerlev 2012, LAK132 2019

// This file is part of Anaconda.

// Anaconda is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Anaconda is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Anaconda.  If not, see <http://www.gnu.org/licenses/>.

// This is here to stop the #define ERROR clash caused by wingdi
#include <GL/gl3w.h>
#include <SDL2/SDL.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui_utils.hpp"

#include "dump.h"
#include "lisk_impl.hpp"
#include "main.h"

#include <lak/opengl/shader.hpp>
#include <lak/opengl/state.hpp>
#include <lak/opengl/texture.hpp>

#include <lak/bank_ptr.hpp>
#include <lak/defer.hpp>
#include <lak/file.hpp>
#include <lak/string_utils.hpp>
#include <lak/test.hpp>
#include <lak/window.hpp>

se::source_explorer_t SrcExp;
int opengl_major, opengl_minor;

#ifndef MAXDIRLEN
#	define MAXDIRLEN 512
#endif

bool byte_pairs_mode = false;

void ViewImage(const lak::opengl::texture &texture, const float scale)
{
	ImGui::Image(
	  (ImTextureID)(uintptr_t)texture.get(),
	  ImVec2(scale * (float)texture.size().x, scale * (float)texture.size().y));
}

void HelpText()
{
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(10.0, 10.0));
	ImGui::Text(
	  "To open a game either drag and drop it into this window or "
	  "go to [File]->[Open]\n");
	ImGui::PushStyleColor(ImGuiCol_Text, 0xFF8080FF);
	ImGui::Text(
	  "If Source Explorer cannot open/throws errors while opening a file,\n"
	  "please SAVE THE ERROR LOG ([File]->[Save Error Log]) and share\n"
	  "it at https://github.com/LAK132/SourceExplorer/issues\n");
	ImGui::PopStyleColor();
	ImGui::Text(
	  "If Source Explorer crashes before you can save the log file,\n"
	  "it will attempt to save it to:\n'%s'\n",
	  lak::debugger.crash_path.string().c_str());
	ImGui::PopStyleVar();
}

void MenuBar(float frame_time)
{
	if (ImGui::BeginMenu("File"))
	{
		ImGui::Checkbox("Auto-dump Mode", &SrcExp.baby_mode);
		SrcExp.exe.attempt |= ImGui::MenuItem(
		  SrcExp.baby_mode ? "Open And Dump..." : "Open...", nullptr);
		SrcExp.sorted_images.attempt |= ImGui::MenuItem(
		  "Dump Sorted Images...", nullptr, false, !SrcExp.baby_mode);
		SrcExp.images.attempt |=
		  ImGui::MenuItem("Dump Images...", nullptr, false, !SrcExp.baby_mode);
		SrcExp.sounds.attempt |=
		  ImGui::MenuItem("Dump Sounds...", nullptr, false, !SrcExp.baby_mode);
		SrcExp.music.attempt |=
		  ImGui::MenuItem("Dump Music...", nullptr, false, !SrcExp.baby_mode);
		SrcExp.shaders.attempt |=
		  ImGui::MenuItem("Dump Shaders...", nullptr, false, !SrcExp.baby_mode);
		SrcExp.binary_files.attempt |= ImGui::MenuItem(
		  "Dump Binary Files...", nullptr, false, !SrcExp.baby_mode);
		SrcExp.appicon.attempt |=
		  ImGui::MenuItem("Dump App Icon...", nullptr, false, !SrcExp.baby_mode);
		ImGui::Separator();
		SrcExp.error_log.attempt |= ImGui::MenuItem("Save Error Log...");
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("About"))
	{
		ImGui::Text(APP_NAME " by LAK132");
		switch (SrcExp.graphics_mode)
		{
			case lak::graphics_mode::OpenGL:
				ImGui::Text("Using OpenGL %d.%d", opengl_major, opengl_minor);
				break;

			case lak::graphics_mode::Software:
				ImGui::Text("Using Softraster");
				break;

			default: break;
		}
		ImGui::Text("Frame rate %f", std::round(1.0f / frame_time));
		credits();
		ImGui::Checkbox("Byte Pairs", &byte_pairs_mode);
		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Help"))
	{
		HelpText();
		ImGui::EndMenu();
	}

	ImGui::Checkbox("Color transparency?", &SrcExp.dump_color_transparent);
	ImGui::Checkbox("Force compat mode?", &se::force_compat);
	ImGui::Checkbox("Debug console? (May make SE slow)",
	                &lak::debugger.live_output_enabled);

	if (lak::debugger.live_output_enabled)
	{
		ImGui::Checkbox("Only errors?", &lak::debugger.live_errors_only);
		ImGui::Checkbox("Developer mode?", &lak::debugger.line_info_enabled);
	}
}

void Navigator()
{
	if (SrcExp.loaded)
	{
		if (SrcExp.state.game.title)
			ImGui::Text("Title: %s",
			            lak::strconv<char>(SrcExp.state.game.title->value).c_str());
		if (SrcExp.state.game.author)
			ImGui::Text("Author: %s",
			            lak::strconv<char>(SrcExp.state.game.author->value).c_str());
		if (SrcExp.state.game.copyright)
			ImGui::Text(
			  "Copyright: %s",
			  lak::strconv<char>(SrcExp.state.game.copyright->value).c_str());
		if (SrcExp.state.game.output_path)
			ImGui::Text(
			  "Output: %s",
			  lak::strconv<char>(SrcExp.state.game.output_path->value).c_str());
		if (SrcExp.state.game.project_path)
			ImGui::Text(
			  "Project: %s",
			  lak::strconv<char>(SrcExp.state.game.project_path->value).c_str());

		ImGui::Separator();

		if (SrcExp.state.recompiled)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, 0xFF8080FF);
			ImGui::Text(
			  "WARNING: THIS GAME APPEARS TO HAVE BEEN RECOMPILED WITH\n"
			  "AN EXTERNAL TOOL, DUMPING WILL FAIL!\nTHIS IS NOT A BUG.");
			ImGui::PopStyleColor();
		}

		ImGui::Text("New Game: %s", SrcExp.state.old_game ? "No" : "Yes");
		ImGui::Text("Unicode Game: %s", SrcExp.state.unicode ? "Yes" : "No");
		ImGui::Text("Compat Game: %s", SrcExp.state.compat ? "Yes" : "No");
		ImGui::Text("2.5+ Game: %s",
		            SrcExp.state.two_five_plus_game ? "Yes" : "No");
		ImGui::Text("Product Build: %zu", (size_t)SrcExp.state.product_build);
		ImGui::Text("Product Version: %zu", (size_t)SrcExp.state.product_version);
		ImGui::Text("Runtime Version: %zu", (size_t)SrcExp.state.runtime_version);
		ImGui::Text("Runtime Sub-Version: %zu",
		            (size_t)SrcExp.state.runtime_sub_version);

		ImGui::Separator();

		SrcExp.state.game.view(SrcExp).UNWRAP();
	}
}

bool Crypto()
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

void BytePairsMemoryExplorer(const byte_t *data, size_t size, bool update)
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
		ViewImage(texture, scale);
	}
}

void RawImageMemoryExplorer(const byte_t *data, size_t size, bool update)
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

		auto img           = (byte_t *)image.data();
		const auto img_end = img + (image.contig_size() * sizeof(image[0]));

		for (uint64_t i = 0; img < img_end && it < end;)
		{
			*img = *it;
			++it;
			++i;
			if (block_skip.x > 0 && (i % block_skip.x) == 0) it += block_skip.y;
			img += (5 - colour_size);
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
		ViewImage(texture, scale);
	}
}

void MemoryExplorer(bool &update)
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
			BytePairsMemoryExplorer(
			  SrcExp.state.file->data(), SrcExp.state.file->size(), update);
		}
		else if (content_mode == 2)
		{
			RawImageMemoryExplorer(
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
			  raw ? SrcExp.view->head.data : SrcExp.view->decode_head().UNWRAP();

		if (content_mode == 1)
		{
			BytePairsMemoryExplorer(
			  SrcExp.buffer.data(), SrcExp.buffer.size(), update);
		}
		else if (content_mode == 2)
		{
			RawImageMemoryExplorer(
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
			  raw ? SrcExp.view->body.data : SrcExp.view->decode_body().UNWRAP();

		if (content_mode == 1)
		{
			BytePairsMemoryExplorer(
			  SrcExp.buffer.data(), SrcExp.buffer.size(), update);
		}
		else if (content_mode == 2)
		{
			RawImageMemoryExplorer(
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

void ImageExplorer(bool &update)
{
	static float scale = 1.0f;
	ImGui::DragFloat("Scale", &scale, 0.1f, 0.1f, 10.0f);
	ImGui::Separator();
	se::ViewImage(SrcExp, scale);
	update = false;
}

void AudioExplorer(bool &update)
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
		ImGui::ProgressBar(
		  1.0f - float(SDL_GetQueuedAudioSize(audio_device) / (double)audio_size));
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

lisk::string lisk_init_script;
lisk::string lisk_loop_script;
lisk::expression lisk_loop_expr;
lisk::string lisk_exception_message = "";
lisk::environment lisk_script_environment;
bool run_lisk_script = false;

void LiskEditor()
{
	if (!run_lisk_script)
	{
		if (ImGui::Button("Run"))
		{
			lisk_script_environment = DefaultEnvironment();

			if (const auto result =
			      lisk::root_eval_string(lisk_init_script, lisk_script_environment);
			    result.is_exception())
			{
				lisk_exception_message = result.as_exception().message;
			}

			if (const auto tokens = lisk::root_tokenise(lisk_loop_script);
			    !tokens.empty())
			{
				auto loop = lisk::parse(tokens);
				if (loop.is_exception())
				{
					lisk_exception_message = loop.as_exception().message;
				}
				else if (loop.is_list())
				{
					run_lisk_script = true;
					lisk_loop_expr  = loop;
				}
			}
		}
	}
	else
	{
		if (ImGui::Button("Stop"))
		{
			run_lisk_script = false;
		}
	}

	if (!lisk_exception_message.empty())
		ImGui::Text("Exception thrown: %s", lisk_exception_message.c_str());

	ImGui::Text("Init:");

	lak::input_text(
	  "lisk-init-editor", &lisk_init_script, ImGuiInputTextFlags_Multiline);

	ImGui::Text("Loop:");

	lak::input_text(
	  "lisk-loop-editor", &lisk_loop_script, ImGuiInputTextFlags_Multiline);

	if (run_lisk_script)
	{
		auto result = lisk::eval(lisk_loop_expr, lisk_script_environment, true);
		if (result.is_exception())
		{
			run_lisk_script        = false;
			lisk_exception_message = result.as_exception().message;
		}
	}
}

void Explorer()
{
	if (SrcExp.loaded)
	{
		enum mode
		{
			MEMORY,
			IMAGE,
			AUDIO,
			LISK
		};
		static int selected = 0;
		ImGui::RadioButton("Memory", &selected, MEMORY);
		ImGui::SameLine();
		ImGui::RadioButton("Image", &selected, IMAGE);
		ImGui::SameLine();
		ImGui::RadioButton("Audio", &selected, AUDIO);
		ImGui::SameLine();
		ImGui::RadioButton("Lisk", &selected, LISK);

		static bool crypto = false;
		ImGui::Checkbox("Crypto", &crypto);
		ImGui::Separator();

		static bool mem_update   = false;
		static bool image_update = false;
		static bool audio_update = false;
		if (crypto)
		{
			if (Crypto()) mem_update = image_update = audio_update = true;
			ImGui::Separator();
		}

		switch (selected)
		{
			case MEMORY: MemoryExplorer(mem_update); break;
			case IMAGE: ImageExplorer(image_update); break;
			case AUDIO: AudioExplorer(audio_update); break;
			case LISK: LiskEditor(); break;
			default: selected = 0; break;
		}
	}
}

void SourceExplorerMain(float frame_time)
{
	if (ImGui::BeginMenuBar())
	{
		MenuBar(frame_time);
		ImGui::EndMenuBar();
	}

	if (!SrcExp.baby_mode && SrcExp.loaded)
	{
		ImVec2 content_size = ImGui::GetWindowContentRegionMax();
		content_size.x      = ImGui::GetWindowContentRegionWidth();

		static float left_size  = content_size.x / 2;
		static float right_size = content_size.x / 2;

		lak::HoriSplitter(left_size, right_size, content_size.x);

		ImGui::BeginChild(
		  "Left", {left_size, -1}, true, ImGuiWindowFlags_NoSavedSettings);
		Navigator();
		ImGui::EndChild();

		ImGui::SameLine();

		ImGui::BeginChild(
		  "Right", {right_size, -1}, true, ImGuiWindowFlags_NoSavedSettings);
		Explorer();
		ImGui::EndChild();
	}
	else
	{
		HelpText();
	}

	if (SrcExp.exe.attempt)
		se::AttemptExe(SrcExp);
	else if (SrcExp.images.attempt)
		se::AttemptImages(SrcExp);
	else if (SrcExp.sorted_images.attempt)
		se::AttemptSortedImages(SrcExp);
	else if (SrcExp.appicon.attempt)
		se::AttemptAppIcon(SrcExp);
	else if (SrcExp.sounds.attempt)
		se::AttemptSounds(SrcExp);
	else if (SrcExp.music.attempt)
		se::AttemptMusic(SrcExp);
	else if (SrcExp.shaders.attempt)
		se::AttemptShaders(SrcExp);
	else if (SrcExp.binary_files.attempt)
		se::AttemptBinaryFiles(SrcExp);
	else if (SrcExp.error_log.attempt)
		se::AttemptErrorLog(SrcExp);
	else if (SrcExp.binary_block.attempt)
		se::AttemptBinaryBlock(SrcExp);
}

void SourceBytePairsMain(float)
{
	if (ImGui::BeginMenuBar())
	{
		ImGui::Checkbox("Byte Pairs", &byte_pairs_mode);
		ImGui::EndMenuBar();
	}

	auto load = [](se::file_state_t &file_state)
	{
		lak::debugger.clear();
		auto code = lak::open_file_modal(file_state.path, false);
		if (code.is_ok() && code.unwrap() == lak::file_open_error::VALID)
		{
			SrcExp.state      = se::game_t{};
			SrcExp.state.file = se::make_data_ref_ptr(
			  se::data_ref_ptr_t{},
			  lak::read_file(file_state.path).EXPECT("failed to load file"));
			ASSERT(SrcExp.state.file != nullptr);
			return true;
		}
		return code.is_err() || code.unwrap() != lak::file_open_error::INCOMPLETE;
	};

	auto manip = []
	{
		DEBUG("File size: ", SrcExp.state.file->size());
		SrcExp.loaded = true;
		return true;
	};

	if (!SrcExp.loaded || SrcExp.exe.attempt)
	{
		SrcExp.exe.attempt = true;
		se::Attempt(SrcExp.exe, load, manip);
		if (!SrcExp.exe.attempt) byte_pairs_mode = false; // User cancelled
	}

	if (SrcExp.loaded)
		BytePairsMemoryExplorer(
		  SrcExp.state.file->data(), SrcExp.state.file->size(), false);
}

#if 1
void MainScreen(float frame_time)
{
	if (byte_pairs_mode)
		SourceBytePairsMain(frame_time);
	else
		SourceExplorerMain(frame_time);
}

#else
void FloatThing(lak::memory &block)
{
	if (auto *ptr = block.read_type<float>(); ptr)
		ImGui::DragFloat("FloatThing", ptr);
}

std::vector<void (*)(lak::memory &block)> funcs = {&FloatThing};

void MainScreen(float frame_time)
{
	if (ImGui::BeginMenuBar())
	{
		ImGui::EndMenuBar();
	}

	float f = 0.0;

	lak::memory block;
	block.write_type(&f);
	block.position = 0;

	for (auto *func : funcs) func(block);
}
#endif

#include <lak/basic_program.inl>

ImGui::ImplContext imgui_context = nullptr;

bool force_only_error = false;

lak::optional<int> basic_window_preinit(int argc, char **argv)
{
	lak::debugger.std_out(u8"", u8"" APP_NAME "\n");

	for (int arg = 1; arg < argc; ++arg)
	{
		if (argv[arg] == lak::astring("-help"))
		{
			std::cout
			  << "srcexp.exe [-help] [-nogl] [-onlyerr] "
			     "[-listtests | -testall | -tests \"test1;test2\"] [<filepath>]\n";
			return lak::optional<int>(0);
		}
		else if (argv[arg] == lak::astring("-nogl"))
		{
			basic_window_force_software = true;
		}
		else if (argv[arg] == lak::astring("-onlyerr"))
		{
			force_only_error = true;
		}
		else if (argv[arg] == lak::astring("-listtests"))
		{
			lak::debugger.std_out(lak::u8string(),
			                      lak::u8string(u8"Available tests:\n"));
			for (const auto &[name, func] : lak::registered_tests())
			{
				lak::debugger.std_out(lak::u8string(),
				                      lak::to_u8string(name) + u8"\n");
			}
		}
		else if (argv[arg] == lak::astring("-testall"))
		{
			return lak::optional<int>(lak::run_tests());
		}
		else if (argv[arg] == lak::astring("-tests") ||
		         argv[arg] == lak::astring("-test"))
		{
			++arg;
			if (arg >= argc) FATAL("Missing tests");
			return lak::optional<int>(lak::run_tests(
			  lak::as_u8string(lak::astring_view::from_c_str(argv[arg]))));
		}
		else
		{
			SrcExp.baby_mode   = false;
			SrcExp.exe.path    = argv[arg];
			SrcExp.exe.valid   = true;
			SrcExp.exe.attempt = true;
			if (!lak::path_exists(SrcExp.exe.path).UNWRAP())
				FATAL(SrcExp.exe.path, " does not exist");
		}
	}

	basic_window_target_framerate      = 30;
	basic_window_opengl_settings.major = 3;
	basic_window_opengl_settings.minor = 2;
	basic_window_clear_colour          = {0.0f, 0.0f, 0.0f, 1.0f};

	return lak::nullopt;
}

void basic_window_init(lak::window &window)
{
	lak::debugger.crash_path = SrcExp.error_log.path =
	  fs::current_path() / "SEND-THIS-CRASH-LOG-TO-LAK132.txt";

	SrcExp.images.path = SrcExp.sorted_images.path = SrcExp.sounds.path =
	  SrcExp.music.path = SrcExp.shaders.path = SrcExp.binary_files.path =
	    SrcExp.appicon.path = SrcExp.binary_block.path = fs::current_path();

	lak::debugger.live_output_enabled = true;

	if (!SrcExp.exe.attempt)
	{
		lak::debugger.live_errors_only = true;
		SrcExp.exe.path                = fs::current_path();
	}
	else
	{
		lak::debugger.live_errors_only = force_only_error;
	}

	imgui_context        = ImGui::ImplCreateContext(window.graphics());
	SrcExp.graphics_mode = window.graphics();
	ImGui::ImplInit();
	ImGui::ImplInitContext(imgui_context, window);

	switch (window.graphics())
	{
		case lak::graphics_mode::OpenGL:
		{
			opengl_major = lak::opengl::get_uint(GL_MAJOR_VERSION);
			opengl_minor = lak::opengl::get_uint(GL_MINOR_VERSION);
		}
		break;

		case lak::graphics_mode::Software:
		{
			ImGuiStyle &style      = ImGui::GetStyle();
			style.AntiAliasedLines = false;
			style.AntiAliasedFill  = false;
			style.WindowRounding   = 0.0f;
		}
		break;

		default: break;
	}

#ifdef LAK_USE_SDL
	if (SDL_Init(SDL_INIT_AUDIO)) ERROR("Failed to initialise SDL audio");
#endif

	ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowRounding = 0;
}

void basic_window_handle_event(lak::window &, lak::event &event)
{
	ImGui::ImplProcessEvent(imgui_context, event);

	switch (event.type)
	{
		case lak::event_type::dropfile:
			SrcExp.exe.path    = event.dropfile().path;
			SrcExp.exe.valid   = true;
			SrcExp.exe.attempt = true;
			break;
		default: break;
	}
}

void basic_window_loop(lak::window &window, uint64_t counter_delta)
{
	const float frame_time = (float)counter_delta / lak::performance_frequency();
	ImGui::ImplNewFrame(imgui_context, window, frame_time);

	bool mainOpen = true;

	ImGuiStyle &style = ImGui::GetStyle();
	ImGuiIO &io       = ImGui::GetIO();

	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(io.DisplaySize);
	ImVec2 old_window_padding = style.WindowPadding;
	style.WindowPadding       = ImVec2(0.0f, 0.0f);
	if (ImGui::Begin(APP_NAME,
	                 &mainOpen,
	                 ImGuiWindowFlags_AlwaysAutoResize |
	                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_MenuBar |
	                   ImGuiWindowFlags_NoSavedSettings |
	                   ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove))
	{
		style.WindowPadding = old_window_padding;
		MainScreen(frame_time);
		ImGui::End();
	}

	ImGui::ImplRender(imgui_context);
}

int basic_window_quit(lak::window &)
{
	ImGui::ImplShutdownContext(imgui_context);
	return 0;
}
