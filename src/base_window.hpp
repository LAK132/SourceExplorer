#ifndef BASE_WINDOW_HPP
#define BASE_WINDOW_HPP

#include <lak/bit_reader.hpp>
#include <lak/imgui/backend.hpp>
#include <lak/imgui/widgets.hpp>
#include <lak/span_manip.hpp>
#include <lak/string_literals/view.hpp>

#include <binex/basic_window.hpp>
#include <binex/widgets.hpp>

#include "main.h"

#include "ctf/explorer.hpp"

template<typename DERIVED>
struct base_window : bex::basic_window<DERIVED>
{
	struct memory_view : public bex::memory_region_selector
	{
		bool draw(srcexp::data_ref_span_t &view_data, bool force_update)
		{
			return draw(view_data.source_span(), view_data, force_update);
		}

		bool draw(const srcexp::data_ref_span_t &parent_data,
		          srcexp::data_ref_span_t &view_data,
		          bool force_update)
		{
			return draw(static_cast<lak::span<byte_t>>(parent_data),
			            static_cast<lak::span<byte_t> &>(view_data),
			            force_update);
		}

		using bex::memory_region_selector::draw;
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
		auto mode_check = [](srcexp::instance_t::main_mode_t mode,
		                     const char *mode_name) -> bool
		{
			bool result = false;
			if (bool set = SrcExp->main_mode == mode;
			    (result |= ImGui::Checkbox(mode_name, &set)) && set)
				SrcExp->main_mode = mode;
			return result;
		};

		return bool(
		  int(mode_check(srcexp::instance_t::main_mode_t::normal, "Normal Mode")) |
		  mode_check(srcexp::instance_t::main_mode_t::byte_pairs, "Byte Pairs") |
		  mode_check(srcexp::instance_t::main_mode_t::binary_analysis,
		             "Binary Analysis") |
		  mode_check(srcexp::instance_t::main_mode_t::testing, "Testing"));
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

	bool crypto()
	{
		bool updated   = false;
		int magic_char = srcexp::_magic_char;
		if (ImGui::InputInt("Magic Char (u8)", &magic_char))
		{
			srcexp::_magic_char = static_cast<uint8_t>(magic_char);
			srcexp::GetEncryptionKey(SrcExp->state);
			updated = true;
		}
		if (ImGui::Button("Generate Crypto Key"))
		{
			srcexp::GetEncryptionKey(SrcExp->state);
			updated = true;
		}
		return updated;
	}

	struct memory_explorer_t
	{
		const srcexp::basic_entry_t *last = nullptr;
		int data_mode                     = 0;
		bool raw                          = true;

		void draw(bool &update)
		{
			if (!SrcExp->state.file) return;

			update |= last != SrcExp->view;
			DEFER(last = SrcExp->view);
			DEFER(update = false);

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
				SrcExp->binary_block.attempt |= ImGui::Button("Save Binary");
				ImGui::SameLine();
				SrcExp->viewer.draw(*SrcExp->state.file, update);

				if (SrcExp->viewer.content_mode ==
				    bex::memory_viewer::VIEW_DATA_BINARY)
				{
					if (update && SrcExp->view != nullptr)
					{
						SCOPED_CHECKPOINT(__func__, "::EXE");
						auto ref_span = SrcExp->view->ref_span;
						while (ref_span._source &&
						       lak::not_equal_to{}(ref_span._source.get(),
						                           SrcExp->state.file.get()))
						{
							CHECKPOINT();
							ref_span = ref_span.parent_span();
						}
						if (ref_span._source && !ref_span.empty())
						{
							const auto from = ref_span.position().UNWRAP();
							SrcExp->viewer.editor.GotoAddrAndHighlight(
							  from, from + ref_span.size());
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
				if (update && SrcExp->view != nullptr)
					SrcExp->buffer =
					  raw ? SrcExp->view->head.data
					      : SrcExp->view->decode_head()
					          .or_else(
					            [&](const auto &err)
					              -> srcexp::result_t<srcexp::data_ref_span_t>
					            {
						            ERROR(err);
						            return lak::ok_t{SrcExp->view->head.data};
					            })
					          .UNWRAP();

				SrcExp->binary_block.attempt |= ImGui::Button("Save Binary");
				ImGui::SameLine();
				SrcExp->viewer.draw(SrcExp->buffer, update);
			}
			else if (data_mode == 2) // Body
			{
				if (update && SrcExp->view != nullptr)
					SrcExp->buffer =
					  raw ? SrcExp->view->body.data
					      : SrcExp->view->decode_body()
					          .or_else(
					            [&](const auto &err)
					              -> srcexp::result_t<srcexp::data_ref_span_t>
					            {
						            ERROR(err);
						            return lak::ok_t{SrcExp->view->body.data};
					            })
					          .UNWRAP();

				SrcExp->binary_block.attempt |= ImGui::Button("Save Binary");
				ImGui::SameLine();
				SrcExp->viewer.draw(SrcExp->buffer, update);
			}
			else if (data_mode == 3) // _magic_key
			{
				SrcExp->viewer.editor.DrawContents(&(srcexp::_magic_key[0]),
				                                   srcexp::_magic_key.size());
				if (update) SrcExp->viewer.editor.GotoAddrAndHighlight(0, 0);
			}
		}
	} _memory_explorer;

	void memory_explorer(bool &update) { _memory_explorer.draw(update); }

	struct image_explorer_t
	{
		float scale = 1.0f;
		void draw(bool &update)
		{
			ImGui::DragFloat("Scale", &scale, 0.1f, 0.1f, 10.0f);
			ImGui::Separator();
			srcexp::ViewImage(*SrcExp, scale);
			update = false;
		}
	} _image_explorer;

	void image_explorer(bool &update) { _image_explorer.draw(update); }

	struct audio_explorer_t
	{
		struct audio_data_t
		{
			lak::u8string name;
			lak::u16string u16name;
			srcexp::sound_mode_t type = (srcexp::sound_mode_t)0;
			uint32_t checksum         = 0;
			uint32_t references       = 0;
			uint32_t decomp_len       = 0;
			uint32_t reserved         = 0;
			uint32_t name_len         = 0;
			uint16_t format           = 0;
			uint16_t channel_count    = 0;
			uint32_t sample_rate      = 0;
			uint32_t byte_rate        = 0;
			uint16_t block_align      = 0;
			uint16_t bits_per_sample  = 0;
			uint16_t unknown          = 0;
			uint32_t chunk_size       = 0;
			lak::array<byte_t> data;
		};

		const srcexp::basic_entry_t *last = nullptr;
		audio_data_t audio_data;

#if defined(LAK_USE_SDL)
		size_t audio_size = 0;
		bool playing      = false;
		SDL_AudioSpec audio_spec;
		SDL_AudioDeviceID audio_device = 0;
		SDL_AudioSpec audio_specGot;
#endif

		void draw(bool &update)
		{
#ifdef LAK_OS_APPLE
			LAK_UNUSED(update);
// getting an issue with operator"" _view not compiling correctly
#else
			update |= last != SrcExp->view;

			if (update && SrcExp->view != nullptr)
			{
				CHECKPOINT();

				srcexp::data_reader_t audio(SrcExp->view->decode_body().UNWRAP());
				audio_data = audio_data_t{};
				if (SrcExp->state.old_game)
				{
					CHECKPOINT();
					audio_data.checksum   = audio.read_u16().UNWRAP();
					audio_data.references = audio.read_u32().UNWRAP();
					audio_data.decomp_len = audio.read_u32().UNWRAP();
					audio_data.type = (srcexp::sound_mode_t)audio.read_u32().UNWRAP();
					audio_data.reserved = audio.read_u32().UNWRAP();
					audio_data.name_len = audio.read_u32().UNWRAP();

					audio_data.name =
					  audio.read_exact_c_str<char8_t>(audio_data.name_len).UNWRAP();

					if (audio_data.type == srcexp::sound_mode_t::wave)
					{
						audio_data.format          = audio.read_u16().UNWRAP();
						audio_data.channel_count   = audio.read_u16().UNWRAP();
						audio_data.sample_rate     = audio.read_u32().UNWRAP();
						audio_data.byte_rate       = audio.read_u32().UNWRAP();
						audio_data.block_align     = audio.read_u16().UNWRAP();
						audio_data.bits_per_sample = audio.read_u16().UNWRAP();
						audio_data.unknown         = audio.read_u16().UNWRAP();
						audio_data.chunk_size      = audio.read_u32().UNWRAP();
						audio_data.data =
						  audio.read<byte_t>(audio_data.chunk_size).UNWRAP();
					}
				}
				else
				{
					CHECKPOINT();
					srcexp::data_reader_t header(SrcExp->view->decode_head().UNWRAP());

					audio_data.checksum   = header.read_u32().UNWRAP();
					audio_data.references = header.read_u32().UNWRAP();
					audio_data.decomp_len = header.read_u32().UNWRAP();
					audio_data.type = (srcexp::sound_mode_t)header.read_u32().UNWRAP();
					audio_data.reserved = header.read_u32().UNWRAP();
					audio_data.name_len = header.read_u32().UNWRAP();

					if (SrcExp->state.unicode)
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
						audio_data.type = srcexp::sound_mode_t::oggs;
					else if (lak::string_view(lak::span(peek)) != "RIFF"_view)
						audio_data.type = srcexp::sound_mode_t(-1);

					if (audio_data.type == srcexp::sound_mode_t::wave)
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
						audio_data.data =
						  audio.read<byte_t>(audio_data.chunk_size).UNWRAP();
					}
				}
			}

#	if defined(LAK_USE_SDL)
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
					default:     break;
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
					audio_device = SDL_OpenAudioDevice(
					  nullptr, false, &audio_spec, &audio_specGot, 0);

				audio_size = audio_data.data.size();
				SDL_QueueAudio(audio_device,
				               audio_data.data.data(),
				               static_cast<Uint32>(audio_size));
				SDL_PauseAudioDevice(audio_device, 0);
				playing = true;
			}

			if (playing && (ImGui::Button("Stop") ||
			                (SDL_GetQueuedAudioSize(audio_device) == 0)))
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
				case srcexp::sound_mode_t::wave: ImGui::Text("WAV"); break;
				case srcexp::sound_mode_t::midi: ImGui::Text("MIDI"); break;
				case srcexp::sound_mode_t::oggs: ImGui::Text("OGG"); break;
				default:                         ImGui::Text("Unknown"); break;
			}
			ImGui::Text("Data Size: 0x%zX", (size_t)audio_data.data.size());
			ImGui::Text("Format: 0x%zX", (size_t)audio_data.format);
			ImGui::Text("Channel Count: %zu", (size_t)audio_data.channel_count);
			ImGui::Text("Sample Rate: %zu", (size_t)audio_data.sample_rate);
			ImGui::Text("Byte Rate: %zu", (size_t)audio_data.byte_rate);
			ImGui::Text("Block Align: 0x%zX", (size_t)audio_data.block_align);
			ImGui::Text("Bits Per Sample: %zu", (size_t)audio_data.bits_per_sample);
			ImGui::Text("Chunk Size: 0x%zX", (size_t)audio_data.chunk_size);

			last   = SrcExp->view;
			update = false;
#endif
		}
	} _audio_explorer;

	void audio_explorer(bool &update) { _audio_explorer.draw(update); }

	struct log_explorer_t
	{
		lak::u8string log_str;
		const char *log_cstr = nullptr;

		void draw()
		{
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
	} _log_explorer;

	void log_explorer() { _log_explorer.draw(); }
};

#endif
