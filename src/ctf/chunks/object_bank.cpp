#include "object_bank.hpp"

#include "../../tostring.hpp"
#include "../explorer.hpp"

#include <lak/utility.hpp>

namespace SourceExplorer
{
	namespace object
	{
		error_t effect_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Effect");
		}

		error_t shape_t::read(game_t &, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			TRY_ASSIGN(border_size =, strm.read_u16());
			TRY_ASSIGN(border_color.r =, strm.read_u8());
			TRY_ASSIGN(border_color.g =, strm.read_u8());
			TRY_ASSIGN(border_color.b =, strm.read_u8());
			TRY_ASSIGN(border_color.a =, strm.read_u8());
			TRY_ASSIGN(shape = (shape_type_t), strm.read_u16());
			TRY_ASSIGN(fill = (fill_type_t), strm.read_u16());

			line     = line_flags_t::none;
			gradient = gradient_flags_t::horizontal;
			handle   = 0xFFFF;

			if (shape == shape_type_t::line)
			{
				TRY_ASSIGN(line = (line_flags_t), strm.read_u16());
			}
			else if (fill == fill_type_t::solid)
			{
				TRY_ASSIGN(color1.r =, strm.read_u8());
				TRY_ASSIGN(color1.g =, strm.read_u8());
				TRY_ASSIGN(color1.b =, strm.read_u8());
				TRY_ASSIGN(color1.a =, strm.read_u8());
			}
			else if (fill == fill_type_t::gradient)
			{
				TRY_ASSIGN(color1.r =, strm.read_u8());
				TRY_ASSIGN(color1.g =, strm.read_u8());
				TRY_ASSIGN(color1.b =, strm.read_u8());
				TRY_ASSIGN(color1.a =, strm.read_u8());
				TRY_ASSIGN(color2.r =, strm.read_u8());
				TRY_ASSIGN(color2.g =, strm.read_u8());
				TRY_ASSIGN(color2.b =, strm.read_u8());
				TRY_ASSIGN(color2.a =, strm.read_u8());
				TRY_ASSIGN(gradient = (gradient_flags_t), strm.read_u16());
			}
			else if (fill == fill_type_t::motif)
			{
				TRY_ASSIGN(handle =, strm.read_u16());
			}

			return lak::ok_t{};
		}

		error_t shape_t::view(source_explorer_t &) const
		{
			LAK_TREE_NODE("Shape")
			{
				ImGui::Text("Border Size: 0x%zX", (size_t)border_size);
				lak::vec4f_t col = ((lak::vec4f_t)border_color) / 255.0f;
				ImGui::ColorEdit4("Border Color", &col.x);
				ImGui::Text("Shape: 0x%zX", (size_t)shape);
				ImGui::Text("Fill: 0x%zX", (size_t)fill);

				if (shape == shape_type_t::line)
				{
					ImGui::Text("Line: 0x%zX", (size_t)line);
				}
				else if (fill == fill_type_t::solid)
				{
					col = ((lak::vec4f_t)color1) / 255.0f;
					ImGui::ColorEdit4("Fill Color", &col.x);
				}
				else if (fill == fill_type_t::gradient)
				{
					col = ((lak::vec4f_t)color1) / 255.0f;
					ImGui::ColorEdit4("Gradient Color 1", &col.x);
					col = ((lak::vec4f_t)color2) / 255.0f;
					ImGui::ColorEdit4("Gradient Color 2", &col.x);
					ImGui::Text("Gradient Flags: 0x%zX", (size_t)gradient);
				}
				else if (fill == fill_type_t::motif)
				{
					ImGui::Text("Handle: 0x%zX", (size_t)handle);
				}
			}

			return lak::ok_t{};
		}

		error_t quick_backdrop_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			RES_TRY(entry.read(game, strm)
			          .RES_ADD_TRACE("object::quick_backdrop_t::read"));

			RES_TRY_ASSIGN(
			  auto span =,
			  entry.decode_body().RES_ADD_TRACE("object::quick_backdrop_t::read"));

			data_reader_t qstrm(span);

			TRY_ASSIGN(size =, qstrm.read_u32());
			TRY_ASSIGN(obstacle =, qstrm.read_u16());
			TRY_ASSIGN(collision =, qstrm.read_u16());
			if (game.old_game)
			{
				TRY_ASSIGN(dimension.x =, qstrm.read_u16());
				TRY_ASSIGN(dimension.y =, qstrm.read_u16());
			}
			else
			{
				TRY_ASSIGN(dimension.x =, qstrm.read_u32());
				TRY_ASSIGN(dimension.y =, qstrm.read_u32());
			}
			RES_TRY(shape.read(game, qstrm)
			          .RES_ADD_TRACE("object::quick_backdrop_t::read"));

			return lak::ok_t{};
		}

		error_t quick_backdrop_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Properties (Quick Backdrop)##%zX",
			              (size_t)entry.ID,
			              entry.position())
			{
				entry.view(srcexp);

				ImGui::Text("Size: 0x%zX", (size_t)size);
				ImGui::Text("Obstacle: 0x%zX", (size_t)obstacle);
				ImGui::Text("Collision: 0x%zX", (size_t)collision);
				ImGui::Text(
				  "Dimension: (%li, %li)", (long)dimension.x, (long)dimension.y);

				RES_TRY(
				  shape.view(srcexp).RES_ADD_TRACE("object::quick_backdrop_t::view"));

				ImGui::Text("Handle: 0x%zX", (size_t)shape.handle);
				if (shape.handle < 0xFFFF)
				{
					RES_TRY(
					  GetImage(srcexp.state, shape.handle)
					    .RES_ADD_TRACE("object::quick_backdrop_t::view: bad image")
					    .and_then([&](const auto &img) { return img.view(srcexp); })
					    .RES_ADD_TRACE("object::quick_backdrop_t::view"));
				}
			}

			return lak::ok_t{};
		}

		error_t backdrop_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			RES_TRY(
			  entry.read(game, strm).RES_ADD_TRACE("object::backdrop_t::read"));

			RES_TRY_ASSIGN(
			  auto span =,
			  entry.decode_body().RES_ADD_TRACE("object::backdrop_t::read"));

			data_reader_t bstrm(span);

			TRY_ASSIGN(size =, bstrm.read_u32());
			TRY_ASSIGN(obstacle =, bstrm.read_u16());
			TRY_ASSIGN(collision =, bstrm.read_u16());
			if (game.old_game && bstrm.remaining().size() >= 6)
			{
				TRY_ASSIGN(dimension.x =, bstrm.read_u16());
				TRY_ASSIGN(dimension.y =, bstrm.read_u16());
			}
			else if (!game.old_game)
			{
				TRY_ASSIGN(dimension.x =, bstrm.read_u32());
				TRY_ASSIGN(dimension.y =, bstrm.read_u32());
			}
			// if (!game.old_game) bstrm.skip(2);
			TRY_ASSIGN(handle =, bstrm.read_u16());

			return lak::ok_t{};
		}

		error_t backdrop_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE(
			  "0x%zX Properties (Backdrop)##%zX", (size_t)entry.ID, entry.position())
			{
				entry.view(srcexp);

				ImGui::Text("Size: 0x%zX", (size_t)size);
				ImGui::Text("Obstacle: 0x%zX", (size_t)obstacle);
				ImGui::Text("Collision: 0x%zX", (size_t)collision);
				ImGui::Text(
				  "Dimension: (%li, %li)", (long)dimension.x, (long)dimension.y);
				ImGui::Text("Handle: 0x%zX", (size_t)handle);
				if (handle < 0xFFFF)
				{
					RES_TRY(
					  GetImage(srcexp.state, handle)
					    .RES_ADD_TRACE("object::backdrop_t::view: bad image")
					    .and_then([&](const auto &img) { return img.view(srcexp); })
					    .RES_ADD_TRACE("object::backdrop_t::view"));
				}
			}

			return lak::ok_t{};
		}

		error_t animation_direction_t::read(game_t &, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			DEBUG("Position: ", strm.position(), "/", strm.size());

			CHECK_REMAINING(strm, 8U);

			TRY_ASSIGN(min_speed =, strm.read_u8());
			TRY_ASSIGN(max_speed =, strm.read_u8());
			TRY_ASSIGN(repeat =, strm.read_u16());
			TRY_ASSIGN(back_to =, strm.read_u16());
			// handles.resize(strm.read_u16()); // :TODO: what's going on here? how
			// did anaconda do this?
			TRY_ASSIGN(const auto handle_count =, strm.read_u8());
			handles.resize(handle_count);
			TRY(strm.skip(1));

			DEBUG("Min Speed: ", min_speed);
			DEBUG("Max Speed: ", max_speed);
			DEBUG("Repeat: ", repeat);
			DEBUG("Back To: ", back_to);
			DEBUG("Handle Count: ", handles.size());

			DEBUG("Position: ", strm.position(), "/", strm.size());

			CHECK_REMAINING(strm, handles.size() * 2);
			for (auto &handle : handles) handle = strm.read_u16().UNWRAP();

			return lak::ok_t{};
		}

		error_t animation_direction_t::view(source_explorer_t &srcexp) const
		{
			ImGui::Text("Min Speed: %d", (int)min_speed);
			ImGui::Text("Max Speed: %d", (int)max_speed);
			ImGui::Text("Repeat: 0x%zX", (size_t)repeat);
			ImGui::Text("Back To: 0x%zX", (size_t)back_to);
			ImGui::Text("Frames: 0x%zX", handles.size());

			int index = 0;
			for (const auto &handle : handles)
			{
				ImGui::PushID(index++);
				DEFER(ImGui::PopID());

				GetImage(srcexp.state, handle)
				  .RES_ADD_TRACE("object::animation_direction_t::view: bad handle")
				  .and_then(
				    [&](auto &img)
				    {
					    return img.view(srcexp).RES_ADD_TRACE(
					      "object::animation_direction_t::view: bad image");
				    })
				  .if_err(
				    [](const auto &err)
				    {
					    ImGui::Text("Invalid Image/Handle");
					    ImGui::Text(
					      "%s",
					      reinterpret_cast<const char *>(lak::streamify(err).c_str()));
				    })
				  .discard();
			}

			return lak::ok_t{};
		}

		error_t animation_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			DEBUG("Position: ", strm.position(), "/", strm.size());

			CHECK_REMAINING(strm, offsets.size() * 2);

			const size_t begin = strm.position();

			size_t index = 0;
			for (auto &offset : offsets)
			{
				TRY_ASSIGN(offset =, strm.read_u16());

				if (offset != 0)
				{
					CHECK_POSITION(strm, begin + offset);
					const size_t pos = strm.position();
					TRY(strm.seek(begin + offset));
					DEFER(strm.seek(pos).UNWRAP(););
					RES_TRY(directions[index]
					          .read(game, strm)
					          .RES_ADD_TRACE("object::animation_t::read"));
				}
				++index;
			}

			TRY(strm.seek(begin + (offsets.size() * 2)));

			return lak::ok_t{};
		}

		error_t animation_t::view(source_explorer_t &srcexp) const
		{
			size_t index = 0;
			for (const auto &direction : directions)
			{
				if (direction.handles.size() > 0)
				{
					LAK_TREE_NODE("Animation Direction 0x%zX", index)
					{
						RES_TRY(direction.view(srcexp).RES_ADD_TRACE(
						  "object::animation_t::view"));
					}
				}
				++index;
			}

			return lak::ok_t{};
		}

		error_t animation_header_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			const size_t begin = strm.position();

			DEBUG("Position: ", strm.position());

			TRY_ASSIGN(size =, strm.read_u16());
			DEBUG("Size: ", size);

			TRY_ASSIGN(const auto offset_count =, strm.read_u16());
			offsets.resize(offset_count);
			animations.resize(offsets.size());

			DEBUG("Animation Count: ", animations.size());

			size_t index = 0;
			for (auto &offset : offsets)
			{
				DEBUG("Animation: ", index, "/", offsets.size());
				TRY_ASSIGN(offset =, strm.read_u16());

				if (offset != 0)
				{
					CHECK_POSITION(strm, begin + offset);
					const size_t pos = strm.position();
					TRY(strm.seek(begin + offset));
					DEFER(strm.seek(pos).UNWRAP());
					RES_TRY(animations[index]
					          .read(game, strm)
					          .RES_ADD_TRACE("object::animation_header_t::read"));
				}
				++index;
			}

			TRY(strm.seek(begin + size));

			return lak::ok_t{};
		}

		error_t animation_header_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("Animations")
			{
				ImGui::Separator();
				ImGui::Text("Size: 0x%zX", (size_t)size);
				ImGui::Text("Animations: %zu", animations.size());
				size_t index = 0;
				for (const auto &animation : animations)
				{
					// TODO: figure out what this was meant to be checking
					if (/*animation.offsets[index] > 0 */ true)
					{
						LAK_TREE_NODE("Animation 0x%zX", index)
						{
							ImGui::Separator();
							RES_TRY(animation.view(srcexp).RES_ADD_TRACE(
							  "object::animation_header_t::view"));
						}
					}
					++index;
				}
			}

			return lak::ok_t{};
		}

		error_t common_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			RES_TRY(entry.read(game, strm).RES_ADD_TRACE("object::common_t::read"));

			RES_TRY_ASSIGN(
			  auto span =,
			  entry.decode_body().RES_ADD_TRACE("object::common_t::read"));

			data_reader_t cstrm(span);

			// if (game.product_build >= 288 && !game.old_game)
			//     mode = game_mode_t::_288;
			// if (game.product_build >= 284 && !game.old_game && !game.compat)
			//     mode = game_mode_t::_284;
			// else
			//     mode = game_mode_t::_OLD;
			mode = _mode;

			// used for offsets.
			const size_t begin = cstrm.position();

			TRY_ASSIGN(size =, cstrm.read_u32());

			DEBUG("Size: ", size);

			CHECK_REMAINING(cstrm, size - 4);

			if (mode == game_mode_t::_288 || mode == game_mode_t::_284)
			{
				// are these in the right order?
				TRY_ASSIGN(animations_offset =, cstrm.read_u16());
				TRY_ASSIGN(movements_offset =, cstrm.read_u16());
				TRY_ASSIGN(version =, cstrm.read_u16());
				TRY_ASSIGN(counter_offset =, cstrm.read_u16());
				TRY_ASSIGN(system_offset =, cstrm.read_u16());
			}
			// else if (mode == game_mode_t::_284)
			// {
			//     counter_offset = cstrm.read_u16();
			//     version = cstrm.read_u16();
			//     cstrm.skip(2);
			//     movements_offset = cstrm.read_u16();
			//     extension_offset = cstrm.read_u16();
			//     animations_offset = cstrm.read_u16();
			// }
			else
			{
				TRY_ASSIGN(movements_offset =, cstrm.read_u16());
				TRY_ASSIGN(animations_offset =, cstrm.read_u16());
				TRY_ASSIGN(version =, cstrm.read_u16());
				TRY_ASSIGN(counter_offset =, cstrm.read_u16());
				TRY_ASSIGN(system_offset =, cstrm.read_u16());
			}

			if (cstrm.empty()) return lak::ok_t{};
			TRY_ASSIGN(flags =, cstrm.read_u32());

#if 1
			TRY(cstrm.skip(8 * 2));
#else
			qualifiers.clear();
			qualifiers.reserve(8);
			for (size_t i = 0; i < 8; ++i)
			{
				TRY_ASSIGN(int16_t qualifier =, cstrm.read_s16());
				if (qualifier == -1) break;
				qualifiers.push_back(qualifier);
			}
#endif

			if (mode == game_mode_t::_284)
			{
				TRY_ASSIGN(system_offset =, cstrm.read_u16());
			}
			else
			{
				TRY_ASSIGN(extension_offset =, cstrm.read_u16());
			}

			TRY_ASSIGN(values_offset =, cstrm.read_u16());
			TRY_ASSIGN(strings_offset =, cstrm.read_u16());
			TRY_ASSIGN(new_flags =, cstrm.read_u32());
			TRY_ASSIGN(preferences =, cstrm.read_u32());
			TRY_ASSIGN(identifier =, cstrm.read_u32());
			TRY_ASSIGN(back_color.r =, cstrm.read_u8());
			TRY_ASSIGN(back_color.g =, cstrm.read_u8());
			TRY_ASSIGN(back_color.b =, cstrm.read_u8());
			TRY_ASSIGN(back_color.a =, cstrm.read_u8());
			TRY_ASSIGN(fade_in_offset =, cstrm.read_u32());
			TRY_ASSIGN(fade_out_offset =, cstrm.read_u32());

			if (animations_offset > 0)
			{
				DEBUG("Animations Offset: ", animations_offset);
				TRY(cstrm.seek(begin + animations_offset));
				animations = lak::unique_ptr<animation_header_t>::make();
				RES_TRY(animations->read(game, cstrm)
				          .RES_ADD_TRACE("object::common_t::read"));
			}

			return lak::ok_t{};
		}

		error_t common_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE(
			  "0x%zX Properties (Common)##%zX", (size_t)entry.ID, entry.position())
			{
				entry.view(srcexp);

				if (mode == game_mode_t::_288 || mode == game_mode_t::_284)
				{
					ImGui::Text("Animations Offset: 0x%zX", (size_t)animations_offset);
					ImGui::Text("Movements Offset: 0x%zX", (size_t)movements_offset);
					ImGui::Text("Version: 0x%zX", (size_t)version);
					ImGui::Text("Counter Offset: 0x%zX", (size_t)counter_offset);
					ImGui::Text("System Offset: 0x%zX", (size_t)system_offset);
					ImGui::Text("Flags: 0x%zX", (size_t)flags);
					ImGui::Text("Extension Offset: 0x%zX", (size_t)extension_offset);
				}
				// else if (mode == game_mode_t::_284)
				// {
				// 	ImGui::Text("Counter Offset: 0x%zX", (size_t)counter_offset);
				// 	ImGui::Text("Version: 0x%zX", (size_t)version);
				// 	ImGui::Text("Movements Offset: 0x%zX", (size_t)movements_offset);
				// 	ImGui::Text("Extension Offset: 0x%zX", (size_t)extension_offset);
				// 	ImGui::Text("Animations Offset: 0x%zX",
				// 	            (size_t)animations_offset);
				// 	ImGui::Text("Flags: 0x%zX", (size_t)flags);
				// 	ImGui::Text("System Offset: 0x%zX", (size_t)system_offset);
				// }
				else
				{
					ImGui::Text("Movements Offset: 0x%zX", (size_t)movements_offset);
					ImGui::Text("Animations Offset: 0x%zX", (size_t)animations_offset);
					ImGui::Text("Version: 0x%zX", (size_t)version);
					ImGui::Text("Counter Offset: 0x%zX", (size_t)counter_offset);
					ImGui::Text("System Offset: 0x%zX", (size_t)system_offset);
					ImGui::Text("Flags: 0x%zX", (size_t)flags);
					ImGui::Text("Extension Offset: 0x%zX", (size_t)extension_offset);
				}
				ImGui::Text("Values Offset: 0x%zX", (size_t)values_offset);
				ImGui::Text("Strings Offset: 0x%zX", (size_t)strings_offset);
				ImGui::Text("New Flags: 0x%zX", (size_t)new_flags);
				ImGui::Text("Preferences: 0x%zX", (size_t)preferences);
				ImGui::Text("Identifier: 0x%zX", (size_t)identifier);
				ImGui::Text("Fade In Offset: 0x%zX", (size_t)fade_in_offset);
				ImGui::Text("Fade Out Offset: 0x%zX", (size_t)fade_out_offset);

				lak::vec3f_t col = ((lak::vec3f_t)back_color) / 256.0f;
				ImGui::ColorEdit3("Background Color", &col.x);

				if (animations)
				{
					RES_TRY(
					  animations->view(srcexp).RES_ADD_TRACE("object::common_t::view"));
				}
			}

			return lak::ok_t{};
		}

		error_t item_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			RES_TRY(entry.read(game, strm).RES_ADD_TRACE("object::item_t::read"));

			RES_TRY_ASSIGN(
			  auto span =,
			  entry.decode_body().RES_ADD_TRACE("object::item_t::read"));

			data_reader_t dstrm(span);

			TRY_ASSIGN(handle =, dstrm.read_u16());
			TRY_ASSIGN(type = (object_type_t), dstrm.read_s16());
			TRY(dstrm.read_u16()); // flags
			TRY(dstrm.read_u16()); // "no longer used"
			TRY_ASSIGN(ink_effect =, dstrm.read_u32());
			TRY_ASSIGN(ink_effect_param =, dstrm.read_u32());

			// :TODO: refactor out this lambda

			[[maybe_unused]] auto err =
			  [&]() -> error_t
			{
				for (bool not_finished = true; not_finished;)
				{
					TRY_ASSIGN(const auto chunk_id = (chunk_t), strm.peek_u16());
					switch (chunk_id)
					{
						case chunk_t::object_name:
							name = lak::unique_ptr<string_chunk_t>::make();
							RES_TRY(
							  name->read(game, strm).RES_ADD_TRACE("object::item_t::read"));
							break;

						case chunk_t::object_properties:
							switch (type)
							{
								case object_type_t::quick_backdrop:
									quick_backdrop = lak::unique_ptr<quick_backdrop_t>::make();
									RES_TRY(quick_backdrop->read(game, strm)
									          .RES_ADD_TRACE("object::item_t::read"));
									break;

								case object_type_t::backdrop:
									backdrop = lak::unique_ptr<backdrop_t>::make();
									RES_TRY(backdrop->read(game, strm)
									          .RES_ADD_TRACE("object::item_t::read"));
									break;

								default:
									common = lak::unique_ptr<common_t>::make();
									RES_TRY(common->read(game, strm)
									          .RES_ADD_TRACE("object::item_t::read"));
									break;
							}
							break;

						case chunk_t::object_effect:
							effect = lak::unique_ptr<effect_t>::make();
							RES_TRY(effect->read(game, strm)
							          .RES_ADD_TRACE("object::item_t::read"));
							break;

						case chunk_t::last:
							end = lak::unique_ptr<last_t>::make();
							RES_TRY(
							  end->read(game, strm).RES_ADD_TRACE("object::item_t::read"));
							[[fallthrough]];

						default:
							not_finished = false;
							break;
					}
				}

				return lak::ok_t{};
			}()
			             .IF_ERR("Failed To Read Child Chunks");

			return lak::ok_t{};
		}

		error_t item_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX %s '%s'##%zX",
			              (size_t)entry.ID,
			              GetObjectTypeString(type),
			              (name ? lak::strconv<char>(name->value).c_str() : ""),
			              entry.position())
			{
				entry.view(srcexp);

				ImGui::Text("Handle: 0x%zX", (size_t)handle);
				ImGui::Text("Type: 0x%zX", (size_t)type);
				ImGui::Text("Ink Effect: 0x%zX", (size_t)ink_effect);
				ImGui::Text("Ink Effect Parameter: 0x%zX", (size_t)ink_effect_param);

				if (name)
				{
					RES_TRY(name->view(srcexp, "Name", true)
					          .RES_ADD_TRACE("object::item_t::view"));
				}

				if (quick_backdrop)
				{
					RES_TRY(quick_backdrop->view(srcexp).RES_ADD_TRACE(
					  "object::item_t::view"));
				}
				if (backdrop)
				{
					RES_TRY(
					  backdrop->view(srcexp).RES_ADD_TRACE("object::item_t::view"));
				}
				if (common)
				{
					RES_TRY(common->view(srcexp).RES_ADD_TRACE("object::item_t::view"));
				}

				if (effect)
				{
					RES_TRY(effect->view(srcexp).RES_ADD_TRACE("object::item_t::view"));
				}
				if (end)
				{
					RES_TRY(end->view(srcexp).RES_ADD_TRACE("object::item_t::view"));
				}
			}

			return lak::ok_t{};
		}

		std::unordered_map<uint32_t, lak::array<lak::u16string>>
		item_t::image_handles() const
		{
			std::unordered_map<uint32_t, lak::array<lak::u16string>> result;

			if (quick_backdrop)
				result[quick_backdrop->shape.handle].emplace_back(u"Backdrop");
			if (backdrop) result[backdrop->handle].emplace_back(u"Quick Backdrop");
			if (common && common->animations)
			{
				for (size_t animIndex = 0;
				     animIndex < common->animations->animations.size();
				     ++animIndex)
				{
					const auto animation = common->animations->animations[animIndex];
					for (int i = 0; i < 32; ++i)
					{
						if (animation.offsets[i] > 0)
						{
							for (size_t frame = 0;
							     frame < animation.directions[i].handles.size();
							     ++frame)
							{
								auto h = animation.directions[i].handles[frame];
								result[h].emplace_back(
								  u"Animation-"_str + SourceExplorer::to_u16string(animIndex) +
								  u" Direction-"_str + SourceExplorer::to_u16string(i) +
								  u" Frame-"_str + SourceExplorer::to_u16string(frame));
							}
						}
					}
				}
			}

			return result;
		}

		error_t bank_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			DEFER(game.bank_completed = 0.0f);

			RES_TRY(entry.read(game, strm).RES_ADD_TRACE("object::bank_t::read"));

			data_reader_t reader(entry.raw_body());

			TRY_ASSIGN(const auto item_count =, reader.read_u32());

			items.resize(item_count);

			DEBUG("Object Bank Size: ", items.size());

			size_t max_tries = 0U;

			auto read_all_items = [&]() -> error_t
			{
				auto read_item = [&](auto &item) -> error_t
				{
					return item.read(game, reader)
					  .IF_ERR("Failed To Read Item ",
					          (&item - items.data()),
					          " Of ",
					          items.size())
					  .RES_ADD_TRACE("object::bank_t::read");
				};

				for (auto &item : items)
				{
					RES_TRY(read_item(item).or_else(
					  [&](const auto &err) -> error_t
					  {
						  if (max_tries == 0) return lak::err_t{err};
						  ERROR(err);
						  DEBUG("Continuing...");
						  --max_tries;
						  return lak::ok_t{};
					  }));

					game.bank_completed =
					  float(double(reader.position()) / double(reader.size()));
				}
				return lak::ok_t{};
			};

			RES_TRY(read_all_items().or_else(
			  [&](const auto &err) -> error_t
			  {
				  if (skip_broken_items) return lak::ok_t{};
				  return lak::err_t{err};
			  }));

			if (!reader.empty())
			{
				WARNING("There is still ",
				        reader.remaining().size(),
				        " bytes left in the object bank");
			}

			return lak::ok_t{};
		}

		error_t bank_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Object Bank (%zu Items)##%zX",
			              (size_t)entry.ID,
			              items.size(),
			              entry.position())
			{
				entry.view(srcexp);

				for (const item_t &item : items)
				{
					RES_TRY(item.view(srcexp).RES_ADD_TRACE("object::bank_t::view"));
				}
			}

			return lak::ok_t{};
		}
	}
}
