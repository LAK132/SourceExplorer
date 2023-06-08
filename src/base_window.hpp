#ifndef BASE_WINDOW_HPP
#define BASE_WINDOW_HPP

#include <lak/imgui/backend.hpp>
#include <lak/imgui/widgets.hpp>
#include <lak/opengl/texture.hpp>
#include <lak/span_manip.hpp>
#include <lak/string_literals.hpp>

#include "main.h"

#include "explorer.h"

template<typename DERIVED>
struct base_window
{
	struct memory_view
	{
		uint64_t view_begin = 0;
		uint64_t view_size  = lak::dynamic_extent;
		bool fixed_size     = true;

		bool draw(se::data_ref_span_t &view_data, bool force_update)
		{
			return draw(view_data.source_span(), view_data, force_update);
		}

		bool draw(const se::data_ref_span_t &parent_data,
		          se::data_ref_span_t &view_data,
		          bool force_update)
		{
			return draw(static_cast<lak::span<byte_t>>(parent_data),
			            static_cast<lak::span<byte_t> &>(view_data),
			            force_update);
		}

		bool draw(lak::span<byte_t> parent_data,
		          lak::span<byte_t> &view_data,
		          bool force_update)
		{
			ImGui::PushID((const void *)this);
			DEFER(ImGui::PopID());

			uint64_t view_end = view_begin + view_size;

			const uint64_t range_min = 0;
			uint64_t range_max       = parent_data.size();

			bool updated = force_update;

			ImGui::Checkbox("Fixed Size", &fixed_size);
			ImGui::SameLine();
			if (ImGui::Button("Reset View"))
			{
				view_begin = 0;
				view_end   = range_max;
				view_size  = range_max;
				updated    = true;
			}

			uint64_t old_size = view_end - view_begin;
			if (ImGui::DragScalar("View Begin",
			                      ImGuiDataType_U64,
			                      &view_begin,
			                      1.0f,
			                      &range_min,
			                      &range_max))
			{
				view_begin = std::min(view_begin, range_max);
				if (fixed_size)
					view_end = std::min(view_begin + old_size, range_max);
				else
					view_end = std::min(std::max(view_begin, view_end), range_max);
				view_size = view_end - view_begin;

				updated = true;
			}

			if (fixed_size)
			{
				if (uint64_t max_size = range_max - view_begin;
				    ImGui::DragScalar("View Size",
				                      ImGuiDataType_U64,
				                      &view_size,
				                      1.0f,
				                      &range_min,
				                      &max_size))
				{
					view_end = view_begin + view_size;

					updated = true;
				}
			}
			else
			{
				if (ImGui::DragScalar("View End",
				                      ImGuiDataType_U64,
				                      &view_end,
				                      1.0f,
				                      &range_min,
				                      &range_max))
				{
					view_end = std::min(view_end, range_max);
					if (fixed_size)
						view_begin =
						  std::min(view_end - std::min(old_size, view_end), range_max);
					else
						view_begin = std::min(view_begin, range_max);
					view_size = view_end - view_begin;

					updated = true;
				}
			}

			updated |= view_data.empty() != parent_data.empty();

			if (updated)
			{
				view_begin = std::min(view_begin, range_max);
				view_end   = std::min(std::max(view_begin, view_end), range_max);
				view_size  = view_end - view_begin;
				view_data  = parent_data.subspan(static_cast<size_t>(view_begin),
                                        static_cast<size_t>(view_size));
			}

			return updated;
		}
	};

	static void credits()
	{
		ImGui::PushID("Credits");
		LAK_TREE_NODE("ImGui") { ImGui::Text("https://github.com/ocornut/imgui"); }
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
		LAK_TREE_NODE("glm") { ImGui::Text("https://github.com/g-truc/glm"); }
		LAK_TREE_NODE("lak") { ImGui::Text("https://github.com/LAK132/lak"); }
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
		       mode_check(se_main_mode_t::binary_analysis, "Binary Analysis") |
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
		const auto content_size{ImGui::GetContentRegionAvail()};

		static float left_size  = content_size.x / 2;
		static float right_size = content_size.x / 2;

		lak::VertSplitter(left_size, right_size, content_size.x);

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

	static void view_image(const se::texture_t &texture, const float scale)
	{
		if (std::holds_alternative<lak::opengl::texture>(texture))
		{
			const auto &img = std::get<lak::opengl::texture>(texture);
			if (!img.get() || SrcExp.graphics_mode != lak::graphics_mode::OpenGL)
			{
				ImGui::Text("No image selected.");
			}
			else
			{
				ImGui::Image(
				  (ImTextureID)(uintptr_t)img.get(),
				  ImVec2(scale * (float)img.size().x, scale * (float)img.size().y));
			}
		}
		else if (std::holds_alternative<texture_color32_t>(texture))
		{
			const auto &img = std::get<texture_color32_t>(texture);
			if (!img.pixels || SrcExp.graphics_mode != lak::graphics_mode::Software)
			{
				ImGui::Text("No image selected.");
			}
			else
			{
				ImGui::Image((ImTextureID)(uintptr_t)&img,
				             ImVec2(scale * (float)img.w, scale * (float)img.h));
			}
		}
		else if (std::holds_alternative<std::monostate>(texture))
		{
			ImGui::Text("No image selected.");
		}
		else
		{
			ERROR("Invalid texture type");
		}
	}

	static void image_memory_explorer_impl(lak::span<byte_t> data,
	                                       lak::vec2u64_t &image_size,
	                                       lak::vec3u64_t &block_skip,
	                                       int &colour_size,
	                                       se::texture_t &texture,
	                                       float &scale,
	                                       bool &update)
	{
		{
			if (ImGui::Button("Reset View"))
			{
				image_size = {256, 256};
				block_skip = {0, 1, 0};
				update     = true;
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
				update = true;
			}
			if (ImGui::DragScalarN("For Every/Times/Skip",
			                       ImGuiDataType_U64,
			                       &block_skip,
			                       3,
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

		update |= std::holds_alternative<std::monostate>(texture);

		if (update)
		{
			static lak::image4_t image{}; // static so we can reuse the memory
			image.resize({static_cast<size_t>(image_size.x),
			              static_cast<size_t>(image_size.y)});

			image.fill({0, 0, 0, 255});

			const auto begin = data.begin();
			const auto end   = data.end();
			auto it          = begin;

			auto get_next = [&,
			                 i      = size_t(0),
			                 stride = uint64_t(block_skip.x) *
			                          uint64_t(block_skip.y)]() mutable -> uint8_t
			{
				const uint8_t value = static_cast<uint8_t>(*(it++));
				if (stride > 0 && ((i + 1) % stride) == 0U) it += block_skip.z;
				++i;
				return value;
			};

			if (colour_size < 1) colour_size = 1;
			switch (colour_size)
			{
				case 1:
					for (size_t i = 0; i < image.contig_size() && it < end; ++i)
					{
						const auto rgb{get_next()};
						image[i] =
						  lak::color4_t(rgb, rgb, rgb, static_cast<uint8_t>(0xFFU));
					}
					break;
				case 2:
					for (size_t i = 0; i < image.contig_size() && it < end; ++i)
					{
						uint16_t rgb = static_cast<uint16_t>(get_next()) << 8U;
						rgb |= static_cast<uint16_t>(get_next());
						image[i] = lak::color4_t(
						  static_cast<uint8_t>((rgb & 0xF800) >> 8), // 1111 1000 0000 0000
						  static_cast<uint8_t>((rgb & 0x07E0) >> 3), // 0000 0111 1110 0000
						  static_cast<uint8_t>((rgb & 0x001F) << 3), // 0000 0000 0001 1111
						  static_cast<uint8_t>(0xFFU));
					}
					break;
				default:
					for (size_t i = 0; i < image.contig_size() && it < end; ++i)
					{
						const uint8_t r = get_next();
						const uint8_t g = get_next();
						const uint8_t b = get_next();
						image[i] = lak::color4_t(r, g, b, static_cast<uint8_t>(0xFFU));
						it += colour_size - 3;
					}
					break;
			}

			texture = se::CreateTexture(image, SrcExp.graphics_mode);
		}

		if (!std::holds_alternative<std::monostate>(texture))
		{
			ImGui::Separator();
			ImGui::DragFloat("Scale", &scale, 0.1f, 0.1f, 10.0f);
			ImGui::Separator();
			DERIVED::view_image(texture, scale);
		}
	}

	static void image_memory_explorer(lak::span<byte_t> data, bool update)
	{
		static lak::vec2u64_t image_size = {256, 256};
		static lak::vec3u64_t block_skip = {0, 1, 0};
		static se::texture_t texture;
		static float scale                  = 1.0f;
		static int colour_size              = 3;
		static lak::span<byte_t> old_data   = data;
		static lak::span<byte_t> image_data = data;

		if (data.empty() && old_data.empty()) return;

		if (!data.empty() && !lak::same_span<byte_t>(data, old_data))
		{
			old_data   = data;
			image_data = data;
			update     = true;
		}

		// if (update)
		// {
		// 	if (SrcExp.view != nullptr && SrcExp.state.file != nullptr &&
		// 	    data == SrcExp.state.file->data())
		// 	{
		// 		auto ref_span = SrcExp.view->ref_span;
		// 		while (ref_span._source && ref_span._source != SrcExp.state.file)
		// 			ref_span = ref_span.parent_span();
		// 		if (!ref_span.empty())
		// 		{
		// 			from = ref_span.position().UNWRAP();
		// 			to   = from + ref_span.size();
		// 		}
		// 		else
		// 		{
		// 			from = 0;
		// 			to   = SIZE_MAX;
		// 		}
		// 	}
		// 	else
		// 	{
		// 		from = 0;
		// 		to   = SIZE_MAX;
		// 	}
		// }

		static memory_view view;
		update |= view.draw(data, image_data, update);

		ImGui::Separator();

		image_memory_explorer_impl(
		  image_data, image_size, block_skip, colour_size, texture, scale, update);
	}

	static void byte_pairs_memory_explorer_impl(lak::span<byte_t> data,
	                                            se::texture_t &texture,
	                                            float &scale,
	                                            bool &update)
	{
		update |= std::holds_alternative<std::monostate>(texture);

		if (update)
		{
			static lak::image<GLfloat> image{lak::vec2s_t{256, 256}};

			image.fill(0.0f);

			const auto begin = data.begin();
			const auto end   = data.end();
			auto it          = begin;

			const GLfloat step = 1.0f / (data.size() / float(image.contig_size()));
			for (uint8_t prev = (it != end ? uint8_t(*it) : 0); it != end;
			     prev         = uint8_t(*(it++)))
        image[{prev, uint8_t(*it)}] += step;

			texture = se::CreateTexture(image, SrcExp.graphics_mode);
		}

		if (!std::holds_alternative<std::monostate>(texture))
		{
			ImGui::DragFloat("Scale", &scale, 0.1f, 0.1f, 10.0f);
			ImGui::Separator();
			DERIVED::view_image(texture, scale);
		}
	}

	static void byte_pairs_memory_explorer(lak::span<byte_t> data, bool update)
	{
		static se::texture_t texture;
		static float scale                  = 1.0f;
		static lak::span<byte_t> old_data   = data;
		static lak::span<byte_t> image_data = data;

		if (data.empty() && old_data.empty()) return;

		if (!data.empty() && !lak::same_span<byte_t>(data, old_data))
		{
			old_data = data;
			update   = true;
		}

		// if (update)
		// {
		// 	if (SrcExp.view != nullptr && SrcExp.state.file != nullptr &&
		// 	    data == SrcExp.state.file->data())
		// 	{
		// 		auto ref_span = SrcExp.view->ref_span;
		// 		while (ref_span._source && ref_span._source != SrcExp.state.file)
		// 			ref_span = ref_span.parent_span();
		// 		if (!ref_span.empty())
		// 		{
		// 			from = ref_span.position().UNWRAP();
		// 			to   = from + ref_span.size();
		// 		}
		// 		else
		// 		{
		// 			from = 0;
		// 			to   = SIZE_MAX;
		// 		}
		// 	}
		// 	else
		// 	{
		// 		from = 0;
		// 		to   = SIZE_MAX;
		// 	}
		// }

		static memory_view view;
		update |= view.draw(data, image_data, update);

		ImGui::Separator();

		byte_pairs_memory_explorer_impl(image_data, texture, scale, update);
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

	enum memory_explorer_content_mode : int
	{
		VIEW_DATA_BINARY,
		VIEW_DATA_BYTE_PAIRS,
		VIEW_DATA_IMAGE,
	};

	static void memory_explorer_impl(MemoryEditor &editor,
	                                 memory_explorer_content_mode &content_mode,
	                                 lak::span<byte_t> data,
	                                 bool &update)
	{
		update |=
		  ImGui::RadioButton("Binary", (int *)&content_mode, VIEW_DATA_BINARY);
		ImGui::SameLine();
		update |= ImGui::RadioButton(
		  "Byte Pairs", (int *)&content_mode, VIEW_DATA_BYTE_PAIRS);
		ImGui::SameLine();
		update |=
		  ImGui::RadioButton("Data Image", (int *)&content_mode, VIEW_DATA_IMAGE);
		ImGui::Separator();

		switch (content_mode)
		{
			case VIEW_DATA_BINARY:
				editor.DrawContents(reinterpret_cast<uint8_t *>(data.data()),
				                    data.size());
				break;

			case VIEW_DATA_BYTE_PAIRS:
				byte_pairs_memory_explorer(data, update);
				break;

			case VIEW_DATA_IMAGE:
				image_memory_explorer(data, update);
				break;

			default:
				content_mode = VIEW_DATA_BINARY;
				break;
		}
	}

	static void memory_explorer(bool &update)
	{
		if (!SrcExp.state.file) return;

		static const se::basic_entry_t *last             = nullptr;
		static int data_mode                             = 0;
		static memory_explorer_content_mode content_mode = VIEW_DATA_BINARY;
		static bool raw                                  = true;
		update |= last != SrcExp.view;

		update |= ImGui::RadioButton("EXE", &data_mode, 0);
		ImGui::SameLine();
		update |= ImGui::RadioButton("Header", &data_mode, 1);
		ImGui::SameLine();
		update |= ImGui::RadioButton("Data", &data_mode, 2);
		ImGui::SameLine();
		update |= ImGui::RadioButton("Magic Key", &data_mode, 3);
		ImGui::Separator();

		if (data_mode > 3) data_mode = 0;

		if (data_mode == 1 || data_mode == 2)
		{
			update |= ImGui::Checkbox("Raw", &raw);
			ImGui::SameLine();
		}

		if (data_mode == 0) // EXE
		{
			SrcExp.binary_block.attempt |= ImGui::Button("Save Binary");
			ImGui::SameLine();
			memory_explorer_impl(
			  SrcExp.editor, content_mode, *SrcExp.state.file, update);

			if (content_mode == VIEW_DATA_BINARY)
			{
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

			SrcExp.binary_block.attempt |= ImGui::Button("Save Binary");
			ImGui::SameLine();
			memory_explorer_impl(SrcExp.editor, content_mode, SrcExp.buffer, update);
			if (content_mode == VIEW_DATA_BINARY && update)
				SrcExp.editor.GotoAddrAndHighlight(0, 0);
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

			SrcExp.binary_block.attempt |= ImGui::Button("Save Binary");
			ImGui::SameLine();
			memory_explorer_impl(SrcExp.editor, content_mode, SrcExp.buffer, update);
			if (content_mode == VIEW_DATA_BINARY && update)
				SrcExp.editor.GotoAddrAndHighlight(0, 0);
		}
		else if (data_mode == 3) // _magic_key
		{
			SrcExp.editor.DrawContents(&(se::_magic_key[0]), se::_magic_key.size());
			if (update) SrcExp.editor.GotoAddrAndHighlight(0, 0);
		}

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
#ifdef LAK_OS_APPLE
		LAK_UNUSED(update);
// getting an issue with operator"" _view not compiling correctly
#else
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

#	if defined(LAK_USE_SDL)
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
				case 0x0001:
					spec.format = AUDIO_S16;
					break;
				case 0x0003:
					spec.format = AUDIO_F32;
					break;
				case 0x0006:
					spec.format = AUDIO_S8; /*8bit A-law*/
					break;
				case 0x0007:
					spec.format = AUDIO_S8; /*abit mu-law*/
					break;
				case 0xFFFE: /*subformat*/
					break;
				default:
					break;
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
#	endif

		ImGui::Text("Name: %s",
		            reinterpret_cast<const char *>(audio_data.name.c_str()));
		ImGui::Text("Type: ");
		ImGui::SameLine();
		switch (audio_data.type)
		{
			case se::sound_mode_t::wave:
				ImGui::Text("WAV");
				break;
			case se::sound_mode_t::midi:
				ImGui::Text("MIDI");
				break;
			case se::sound_mode_t::oggs:
				ImGui::Text("OGG");
				break;
			default:
				ImGui::Text("Unknown");
				break;
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
#endif
	}

	static void log_explorer()
	{
		static lak::u8string log_str;
		static const char *log_cstr = nullptr;

		if (ImGui::Button("Refresh"))
		{
			log_str  = lak::to_u8string(lak::debugger.str());
			log_cstr = (const char *)log_str.c_str();
		}
		ImGui::SameLine();
		if (ImGui::Button("Clear"))
		{
			lak::debugger.clear();
			log_str.clear();
			log_cstr = nullptr;
		}

		if (log_cstr != nullptr && log_str.size() > 0)
		{
			if (ImGui::BeginChild("view debug log"))
			{
				ImGui::TextUnformatted(log_cstr, log_cstr + log_str.size());
			}
			ImGui::EndChild();
		}
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
