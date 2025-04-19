#include "header.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	error_t header_t::controls_t::player_control_t::keys_t::read(
	  data_reader_t &strm, size_t button_count)
	{
		if (button_count == 0) return lak::ok_t{};
		TRY_ASSIGN(up =, strm.read_u16());
		if (button_count == 1) return lak::ok_t{};
		TRY_ASSIGN(down =, strm.read_u16());
		if (button_count == 2) return lak::ok_t{};
		TRY_ASSIGN(left =, strm.read_u16());
		if (button_count == 3) return lak::ok_t{};
		TRY_ASSIGN(right =, strm.read_u16());
		if (button_count == 4) return lak::ok_t{};
		TRY_ASSIGN(button1 =, strm.read_u16());
		if (button_count == 5) return lak::ok_t{};
		TRY_ASSIGN(button2 =, strm.read_u16());
		if (button_count == 6) return lak::ok_t{};
		TRY_ASSIGN(button3 =, strm.read_u16());
		if (button_count == 7) return lak::ok_t{};
		TRY_ASSIGN(button4 =, strm.read_u16());

		return lak::ok_t{};
	}

	void header_t::controls_t::player_control_t::keys_t::view() const
	{
		LAK_TREE_NODE("Keys")
		{
			ImGui::Text("Up: 0x%zX", (size_t)up);
			ImGui::Text("Down: 0x%zX", (size_t)down);
			ImGui::Text("Left: 0x%zX", (size_t)left);
			ImGui::Text("Right: 0x%zX", (size_t)right);
			ImGui::Text("Button 1: 0x%zX", (size_t)button1);
			ImGui::Text("Button 2: 0x%zX", (size_t)button2);
			ImGui::Text("Button 3: 0x%zX", (size_t)button3);
			ImGui::Text("Button 4: 0x%zX", (size_t)button4);
		}
	}

	error_t header_t::controls_t::player_control_t::read(data_reader_t &strm,
	                                                     size_t button_count)
	{
		TRY_ASSIGN(control_type = (control_type_t), strm.read_u16());
		RES_TRY(keys.read(strm, button_count)
		          .RES_ADD_TRACE("header_t::controls_t::player_control_t::read"));

		return lak::ok_t{};
	}

	void header_t::controls_t::player_control_t::view() const
	{
		switch (control_type)
		{
			case control_type_t::joystick1:
				ImGui::Text("Joystick 1");
				break;
			case control_type_t::joystick2:
				ImGui::Text("Joystick 2");
				break;
			case control_type_t::joystick3:
				ImGui::Text("Joystick 3");
				break;
			case control_type_t::joystick4:
				ImGui::Text("Joystick 4");
				break;
			case control_type_t::keyboard:
				ImGui::Text("Keyboard");
				break;
			default:
				ImGui::Text("Unknown");
				break;
		}
		keys.view();
	}

	error_t header_t::controls_t::read(data_reader_t &strm, size_t button_count)
	{
		for (auto &control : controls)
			RES_TRY(control.read(strm, button_count)
			          .RES_ADD_TRACE("header_t::controls_t::read"));

		return lak::ok_t{};
	}

	void header_t::controls_t::view() const
	{
		LAK_TREE_NODE("Controls 1") controls[0].view();
		LAK_TREE_NODE("Controls 2") controls[1].view();
		LAK_TREE_NODE("Controls 3") controls[2].view();
		LAK_TREE_NODE("Controls 4") controls[3].view();
	}

	error_t header_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		RES_TRY(entry.read(game, strm).RES_ADD_TRACE("header_t::read"));

		[&]() -> error_t
		{
			RES_TRY_ASSIGN(auto span =,
			               entry.decode_body().RES_ADD_TRACE("header_t::read"));

			data_reader_t dstrm(span);

			switch (dstrm.remaining().size())
			{
				case 0x54:
				{
					// 1.x game

					game.old_game = true;

					TRY_ASSIGN(size =, dstrm.read_u32());
					// TRY_ASSIGN(flags1 = (header_flag1_t), dstrm.read_u16());
					flags1 = header_flag1_t::none;
					// TRY_ASSIGN(flags2 = (header_flag2_t), dstrm.read_u16());
					flags2 = header_flag2_t::none;
					TRY_ASSIGN(const auto mode =, dstrm.read_u16());
					switch (mode)
					{
						case 3:
							graphics_mode = graphics_mode_t::RGB8;
							break;
						case 4:
							graphics_mode = graphics_mode_t::RGB24;
							break;
						case 6:
							graphics_mode = graphics_mode_t::RGB15;
							break;
						case 7:
							graphics_mode = graphics_mode_t::RGB16;
							break;
						default:
							ERROR("Unknown Graphics Mode: ", mode);
							break;
					}
					// TRY_ASSIGN(flags3 = (header_flag3_t), dstrm.read_u16());
					TRY(dstrm.skip(2));
					flags3 = header_flag3_t::none;
					TRY_ASSIGN(window_width =, dstrm.read_u16());
					TRY_ASSIGN(window_height =, dstrm.read_u16());
					TRY_ASSIGN(initial_score =, dstrm.read_u32());
					TRY_ASSIGN(initial_lives =, dstrm.read_u32());
					RES_TRY(controls.read(dstrm, 6).RES_ADD_TRACE("header_t::read"));
					TRY_ASSIGN(border_color.r =, dstrm.read_u8());
					TRY_ASSIGN(border_color.g =, dstrm.read_u8());
					TRY_ASSIGN(border_color.b =, dstrm.read_u8());
					TRY_ASSIGN(border_color.a =, dstrm.read_u8());
					TRY_ASSIGN(number_of_frames =, dstrm.read_u32());
					// TRY_ASSIGN(framerate =, dstrm.read_u32());
					framerate = uint32_t(-1);
					// TRY_ASSIGN(windows_menu_index =, dstrm.read_u8());
					windows_menu_index = uint8_t(-1);
					TRY(dstrm.skip(3));
				}
				break;

				case 0x70:
				{
					// >= 2.0 game

					TRY_ASSIGN(size =, dstrm.read_u32());
					TRY_ASSIGN(flags1 = (header_flag1_t), dstrm.read_u16());
					TRY_ASSIGN(flags2 = (header_flag2_t), dstrm.read_u16());
					TRY_ASSIGN(const auto mode =, dstrm.read_u16());
					switch (mode)
					{
						case 3:
							graphics_mode = graphics_mode_t::RGB8;
							break;
						case 4:
							graphics_mode = graphics_mode_t::RGB24;
							break;
						case 6:
							graphics_mode = graphics_mode_t::RGB15;
							break;
						case 7:
							graphics_mode = graphics_mode_t::RGB16;
							break;
						default:
							ERROR("Unknown Graphics Mode: ", mode);
							break;
					}
					TRY_ASSIGN(flags3 = (header_flag3_t), dstrm.read_u16());
					TRY_ASSIGN(window_width =, dstrm.read_u16());
					TRY_ASSIGN(window_height =, dstrm.read_u16());
					TRY_ASSIGN(initial_score = ~, dstrm.read_u32());
					TRY_ASSIGN(initial_lives = ~, dstrm.read_u32());
					RES_TRY(controls.read(dstrm, 8).RES_ADD_TRACE("header_t::read"));
					TRY_ASSIGN(border_color.r =, dstrm.read_u8());
					TRY_ASSIGN(border_color.g =, dstrm.read_u8());
					TRY_ASSIGN(border_color.b =, dstrm.read_u8());
					TRY_ASSIGN(border_color.a =, dstrm.read_u8());
					TRY_ASSIGN(number_of_frames =, dstrm.read_u32());
					TRY_ASSIGN(framerate =, dstrm.read_u32());
					TRY_ASSIGN(windows_menu_index =, dstrm.read_u8());
					TRY(dstrm.skip(3));
				}
				break;

				default:
					ERROR("Unknown Header Length ", dstrm.remaining().size());
					break;
			}

			return lak::ok_t();
		}()
		           .discard();

		auto init_chunk = [&](auto &chunk)
		{
			chunk = lak::unique_ptr<
			  typename std::remove_reference_t<decltype(chunk)>::value_type>::make();
			return chunk->read(game, strm);
		};

		chunk_t childID  = (chunk_t)-1;
		size_t start_pos = SIZE_MAX;
		for (bool not_finished = true; not_finished;)
		{
			if (strm.size() > 0)
				game.completed =
				  (float)((double)strm.position() / (double)strm.size());

			if (strm.position() == start_pos)
				return lak::err_t{error(lak::streamify("last read chunk (",
				                                       GetTypeString(childID),
				                                       ") didn't move stream head"))};

			start_pos = strm.position();
			TRY_ASSIGN(childID = (chunk_t), strm.peek_u16());

			// the RES_ADD_TRACEs are here so we get line information
			switch (childID)
			{
				case chunk_t::title:
					RES_TRY(init_chunk(title).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::author:
					RES_TRY(init_chunk(author).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::copyright:
					RES_TRY(init_chunk(copyright).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::project_path:
					RES_TRY(init_chunk(project_path).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::output_path:
					RES_TRY(init_chunk(output_path).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::about:
					RES_TRY(init_chunk(about).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::vitalise_preview:
					RES_TRY(
					  init_chunk(vitalise_preview).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::menu:
					RES_TRY(init_chunk(menu).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::extra_path:
					RES_TRY(init_chunk(extension_path).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::extensions:
					RES_TRY(init_chunk(extensions).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::extra_data:
					RES_TRY(init_chunk(extension_data).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::additional_extensions:
					RES_TRY(
					  init_chunk(additional_extensions).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::app_doc:
					RES_TRY(init_chunk(app_doc).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::other_extension:
					RES_TRY(init_chunk(other_extension).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::extensions_list:
					RES_TRY(init_chunk(extension_list).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::icon:
					RES_TRY(init_chunk(icon).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::demo_version:
					RES_TRY(init_chunk(demo_version).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::security_number:
					RES_TRY(init_chunk(security).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::binary_files:
					RES_TRY(init_chunk(binary_files).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::menu_images:
					RES_TRY(init_chunk(menu_images).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::movement_extensions:
					RES_TRY(
					  init_chunk(movement_extensions).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::exe_only:
					RES_TRY(init_chunk(exe).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::protection:
					RES_TRY(init_chunk(protection).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::shaders:
					RES_TRY(init_chunk(shaders).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::extended_header:
					RES_TRY(init_chunk(extended_header).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::spacer:
					RES_TRY(init_chunk(spacer).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::chunk224F:
					RES_TRY(init_chunk(chunk224F).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::title2:
					RES_TRY(init_chunk(title2).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::chunk2253:
					// 16-bytes
					RES_TRY(init_chunk(chunk2253).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::object_names:
					RES_TRY(init_chunk(object_names).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::chunk2255:
					// blank???
					RES_TRY(init_chunk(chunk2255).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::two_five_plus_object_properties:
					// Appears to have sub chunks
					RES_TRY(init_chunk(two_five_plus_object_properties)
					          .RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::chunk2257:
					RES_TRY(init_chunk(chunk2257).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::object_properties:
					RES_TRY(
					  init_chunk(object_properties).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::font_meta:
					RES_TRY(
					  init_chunk(truetype_fonts_meta).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::font_chunk:
					RES_TRY(init_chunk(truetype_fonts).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::shaders2:
					RES_TRY(init_chunk(shaders2).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::global_events:
					RES_TRY(init_chunk(global_events).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::global_strings:
					RES_TRY(init_chunk(global_strings).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::global_string_names:
					RES_TRY(
					  init_chunk(global_string_names).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::global_values:
					RES_TRY(init_chunk(global_values).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::global_value_names:
					RES_TRY(
					  init_chunk(global_value_names).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::frame_handles:
					RES_TRY(init_chunk(frame_handles).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::bank_offsets:
					RES_TRY(init_chunk(bank_offsets).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::frame_bank:
					RES_TRY(init_chunk(frame_bank).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::frame:
					if (!frame_bank)
						frame_bank = lak::unique_ptr<frame::bank_t>::make();
					else
						ERROR("Frame Bank Already Exists");
					while (strm.remaining().size() >= 2 &&
					       (chunk_t)strm.peek_u16().UNWRAP() == chunk_t::frame)
					{
						if (frame_bank->items.emplace_back().read(game, strm).is_err())
							break;
					}
					break;

					// case chunk_t::object_bank2:
					//     RES_TRY(init_chunk(object_bank_2).RES_ADD_TRACE("header_t::read"));
					//     break;

				case chunk_t::object_bank:
				case chunk_t::object_bank2:
					RES_TRY(init_chunk(object_bank).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::image_bank:
					RES_TRY(init_chunk(image_bank).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::sound_bank:
					RES_TRY(init_chunk(sound_bank).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::music_bank:
					RES_TRY(init_chunk(music_bank).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::font_bank:
					RES_TRY(init_chunk(font_bank).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::fusion_3_seed:
					RES_TRY(init_chunk(fusion_3_seed).RES_ADD_TRACE("header_t::read"));
					break;

				case chunk_t::last:
					RES_TRY(init_chunk(last).RES_ADD_TRACE("header_t::read"));
					not_finished = false;
					break;

				default:
					DEBUG("Invalid Chunk: ", (size_t)childID);
					unknown_chunks.emplace_back();
					RES_TRY(unknown_chunks.back()
					          .read(game, strm)
					          .RES_ADD_TRACE("header_t::read"));
					break;
			}
		}

		return lak::ok_t{};
	}

	error_t header_t::view(instance_t &inst) const
	{
		LAK_TREE_NODE("0x%zX Game Header##%zX", (size_t)entry.ID, entry.position())
		{
			entry.view(inst);

			LAK_TREE_NODE("Data")
			{
				controls.view();
				ImGui::Text("Size: 0x%zX", (size_t)size);
				ImGui::Text("Flags 1: 0x%zX", (size_t)flags1);
				ImGui::Text("Flags 2: 0x%zX", (size_t)flags2);
				ImGui::Text("Flags 3: 0x%zX", (size_t)flags3);
				switch (graphics_mode)
				{
					case graphics_mode_t::RGBA32:
						ImGui::Text("Graphics Mode: RGBA32");
						break;
					case graphics_mode_t::BGRA32:
						ImGui::Text("Graphics Mode: BGRA32");
						break;
					case graphics_mode_t::RGB24:
						ImGui::Text("Graphics Mode: RGB24");
						break;
					case graphics_mode_t::BGR24:
						ImGui::Text("Graphics Mode: BGR24");
						break;
					case graphics_mode_t::RGB16:
						ImGui::Text("Graphics Mode: RGB16");
						break;
					case graphics_mode_t::RGB15:
						ImGui::Text("Graphics Mode: RGB15");
						break;
					case graphics_mode_t::RGB8:
						ImGui::Text("Graphics Mode: RGB8");
						break;
					case graphics_mode_t::JPEG:
						ImGui::Text("Graphics Mode: JPEG");
						break;
				}
				ImGui::Text(
				  "Window: %zu * %zu", (size_t)window_width, (size_t)window_height);
				ImGui::Text("Initial Score: 0x%zX", (size_t)initial_score);
				ImGui::Text("Initial Lives: 0x%zX", (size_t)initial_lives);
				{
					lak::vec4f_t col = ((lak::vec4f_t)border_color) / 256.0f;
					float f[]        = {col.r, col.g, col.b, col.a};
					ImGui::ColorEdit4("Border Colour", f);
				}
				ImGui::Text("Number Of Frames: 0x%zX (%zu)",
				            (size_t)number_of_frames,
				            (size_t)number_of_frames);
				ImGui::Text("Framerate: %zu", (size_t)framerate);
				ImGui::Text("Windows Menu Index: 0x%zX", (size_t)windows_menu_index);
			}

			RES_TRY(title.view(inst, "Title", true).RES_ADD_TRACE("header_t::view"));
			RES_TRY(
			  author.view(inst, "Author", true).RES_ADD_TRACE("header_t::view"));
			RES_TRY(copyright.view(inst, "Copyright", true)
			          .RES_ADD_TRACE("header_t::view"));
			RES_TRY(
			  output_path.view(inst, "Output Path").RES_ADD_TRACE("header_t::view"));
			RES_TRY(project_path.view(inst, "Project Path")
			          .RES_ADD_TRACE("header_t::view"));
			RES_TRY(about.view(inst, "About").RES_ADD_TRACE("header_t::view"));

			RES_TRY(vitalise_preview.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(menu.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(extension_path.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(extensions.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(extension_data.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(
			  additional_extensions.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(app_doc.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(other_extension.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(extension_list.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(icon.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(demo_version.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(security.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(binary_files.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(menu_images.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(movement_extensions.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(object_bank_2.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(exe.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(protection.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(shaders.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(shaders2.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(extended_header.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(spacer.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(chunk224F.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(title2.view(inst).RES_ADD_TRACE("header_t::view"));

			RES_TRY(global_events.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(global_strings.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(global_string_names.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(global_values.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(global_value_names.view(inst).RES_ADD_TRACE("header_t::view"));

			RES_TRY(bank_offsets.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(frame_handles.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(frame_bank.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(object_bank.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(image_bank.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(sound_bank.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(music_bank.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(font_bank.view(inst).RES_ADD_TRACE("header_t::view"));

			RES_TRY(chunk2253.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(object_names.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(chunk2255.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(two_five_plus_object_properties.view(inst).RES_ADD_TRACE(
			  "header_t::view"));
			RES_TRY(chunk2257.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(object_properties.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(truetype_fonts_meta.view(inst).RES_ADD_TRACE("header_t::view"));
			RES_TRY(truetype_fonts.view(inst).RES_ADD_TRACE("header_t::view"));

			for (auto &unk : unknown_strings)
			{
				RES_TRY(unk.view(inst).RES_ADD_TRACE("header_t::view"));
			}

			for (auto &unk : unknown_compressed)
			{
				RES_TRY(unk.view(inst).RES_ADD_TRACE("header_t::view"));
			}

			for (auto &unk : unknown_chunks)
			{
				RES_TRY(unk
				          .basic_view(inst,
				                      (lak::astring("Unknown ") +
				                       std::to_string(unk.entry.position()))
				                        .c_str())
				          .RES_ADD_TRACE("header_t::view"));
			}

			RES_TRY(fusion_3_seed.view(inst).RES_ADD_TRACE("header_t::view"));

			RES_TRY(last.view(inst).RES_ADD_TRACE("header_t::view"));
		}

		return lak::ok_t{};
	}
}
