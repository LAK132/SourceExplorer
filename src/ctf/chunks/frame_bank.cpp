#include "frame_bank.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	namespace frame
	{
		error_t header_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Header");
		}

		error_t password_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Password");
		}

		error_t palette_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			RES_TRY(entry.read(game, strm).RES_ADD_TRACE("frame::palette_t::read"));

			RES_TRY_ASSIGN(
			  auto span =,
			  entry.decode_body().RES_ADD_TRACE("frame::palette_t::read"));

			data_reader_t pstrm(span);

			TRY_ASSIGN(unknown =, pstrm.read_u32());

			CHECK_REMAINING(pstrm, colors.size() * 4);
			for (auto &color : colors)
			{
				color.r = pstrm.read_u8().UNWRAP();
				color.g = pstrm.read_u8().UNWRAP();
				color.b = pstrm.read_u8().UNWRAP();
				/* color.a = */ pstrm.read_u8().UNWRAP();
				color.a = 255u;
			}

			return lak::ok_t{};
		}

		error_t palette_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE(
			  "0x%zX Frame Palette##%zX", (size_t)entry.ID, entry.position())
			{
				entry.view(srcexp);

				uint8_t index = 0;
				for (const auto &color : colors)
				{
					lak::vec3f_t col = ((lak::vec3f_t)color) / 256.0f;
					char str[3];
					snprintf(str, 3, "%hhX", index++);
					float f[] = {col.x, col.y, col.z};
					ImGui::ColorEdit3(str, f);
				}
			}

			return lak::ok_t{};
		}

		lak::image4_t palette_t::image() const
		{
			lak::image4_t result;

			result.resize({16, 16});
			for (int i = 0; i < 256; ++i) result[i] = colors[i];

			return result;
		}

		error_t object_instance_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			TRY_ASSIGN(info =, strm.read_u16());
			TRY_ASSIGN(handle =, strm.read_u16());
			if (game.old_game)
			{
				TRY_ASSIGN(position.x =, strm.read_s16());
				TRY_ASSIGN(position.y =, strm.read_s16());
			}
			else
			{
				TRY_ASSIGN(position.x =, strm.read_s32());
				TRY_ASSIGN(position.y =, strm.read_s32());
			}
			TRY_ASSIGN(parent_type = (object_parent_type_t), strm.read_u16());
			TRY_ASSIGN(parent_handle =, strm.read_u16()); // object info (?)
			if (!game.old_game)
			{
				TRY_ASSIGN(layer =, strm.read_u16());
				TRY_ASSIGN(unknown =, strm.read_u16());
			}

			return lak::ok_t{};
		}

		error_t object_instance_t::view(source_explorer_t &srcexp) const
		{
			lak::u8string str;
			auto obj = GetObject(srcexp.state, handle);
			if (obj.is_ok() && obj.unwrap().name)
				str += lak::to_u8string(obj.unwrap().name->value);

			LAK_TREE_NODE("0x%zX %s##%zX", (size_t)handle, str.c_str(), (size_t)info)
			{
				ImGui::Text("Handle: 0x%zX", (size_t)handle);
				ImGui::Text("Info: 0x%zX", (size_t)info);
				ImGui::Text(
				  "Position: (%li, %li)", (long)position.x, (long)position.y);
				ImGui::Text("Parent Type: %s (0x%zX)",
				            GetObjectParentTypeString(parent_type),
				            (size_t)parent_type);
				ImGui::Text("Parent Handle: 0x%zX", (size_t)parent_handle);
				ImGui::Text("Layer: 0x%zX", (size_t)layer);
				ImGui::Text("Unknown: 0x%zX", (size_t)unknown);

				if (obj.is_ok())
				{
					RES_TRY(obj.unwrap().view(srcexp).RES_ADD_TRACE(
					  "frame::object_instance_t::view"));
				}

				switch (parent_type)
				{
					case object_parent_type_t::frame_item:
						if (auto parent_obj = GetObject(srcexp.state, parent_handle);
						    parent_obj.is_ok())
						{
							RES_TRY(parent_obj.unwrap().view(srcexp).RES_ADD_TRACE(
							  "frame::object_instance_t::view"));
						}
						break;

					case object_parent_type_t::frame:
						if (auto parent_obj = GetFrame(srcexp.state, parent_handle);
						    parent_obj.is_ok())
						{
							RES_TRY(parent_obj.unwrap().view(srcexp).RES_ADD_TRACE(
							  "frame::object_instance_t::view"));
						}
						break;

					case object_parent_type_t::none:
						[[fallthrough]];
					case object_parent_type_t::qualifier:
						[[fallthrough]];
					default:
						break;
				}
			}

			return lak::ok_t{};
		}

		error_t object_instances_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			RES_TRY(entry.read(game, strm)
			          .RES_ADD_TRACE("frame::object_instances_t::read"));

			RES_TRY_ASSIGN(
			  auto span =,
			  entry.decode_body().RES_ADD_TRACE("frame::object_instances_t::read"));

			data_reader_t hstrm(span);

			TRY_ASSIGN(const auto object_count =, hstrm.read_u32());

			objects.resize(object_count);

			DEBUG("Objects: ", objects.size());

			for (auto &object : objects)
			{
				RES_TRY(object.read(game, hstrm)
				          .RES_ADD_TRACE("frame::object_instances_t::read"));
			}

			return lak::ok_t{};
		}

		error_t object_instances_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE(
			  "0x%zX Object Instances##%zX", (size_t)entry.ID, entry.position())
			{
				entry.view(srcexp);

				for (const auto &object : objects)
				{
					RES_TRY(object.view(srcexp).RES_ADD_TRACE(
					  "frame::object_instances_t::view"));
				}
			}

			return lak::ok_t{};
		}

		error_t fade_in_frame_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Fade In Frame");
		}

		error_t fade_out_frame_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Fade Out Frame");
		}

		error_t fade_in_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Fade In");
		}

		error_t fade_out_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Fade Out");
		}

		error_t events_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Events");
		}

		error_t play_header_r::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Play Head");
		}

		error_t additional_item_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Additional Item");
		}

		error_t additional_item_instance_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Additional Item Instance");
		}

		error_t layers_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Layers");
		}

		error_t virtual_size_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Virtual Size");
		}

		error_t demo_file_path_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Demo File Path");
		}

		error_t random_seed_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			RES_TRY(
			  entry.read(game, strm).RES_ADD_TRACE("frame::random_seed_t::read"));

			RES_TRY_ASSIGN(
			  value =,
			  entry.decode_body().and_then(
			    [](const auto &ref_span)
			    {
				    return data_reader_t(ref_span).read_s16().RES_MAP_TO_TRACE(
				      "frame::random_seed_t::read");
			    }));

			return lak::ok_t{};
		}

		error_t random_seed_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE(
			  "0x%zX Random Seed##%zX", (size_t)entry.ID, entry.position())
			{
				entry.view(srcexp);
				ImGui::Text("Value: %i", (int)value);
			}

			return lak::ok_t{};
		}

		error_t layer_effect_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Layer Effect");
		}

		error_t blueray_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Blueray");
		}

		error_t movement_time_base_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Movement Time Base");
		}

		error_t mosaic_image_table_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Mosaic Image Table");
		}

		error_t effects_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Effects");
		}

		error_t iphone_options_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "iPhone Options");
		}

		error_t chunk_334C_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Chunk 334C");
		}

		error_t item_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			RES_TRY(entry.read(game, strm).RES_ADD_TRACE("frame::item_t::read"));

			DEFER(game.bank_completed = 0.0f);

			data_reader_t reader(entry.raw_body());

			for (bool not_finished = true; not_finished;)
			{
				game.bank_completed =
				  float(double(reader.position()) / double(reader.size()));

				if (reader.remaining().size() < 2) break;

				switch ((chunk_t)reader.peek_u16().UNWRAP())
				{
					case chunk_t::frame_name:
						name = lak::unique_ptr<string_chunk_t>::make();
						RES_TRY(
						  name->read(game, reader).RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_header:
						header = lak::unique_ptr<header_t>::make();
						RES_TRY(
						  header->read(game, reader).RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_password:
						password = lak::unique_ptr<password_t>::make();
						RES_TRY(password->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_palette:
						palette = lak::unique_ptr<palette_t>::make();
						RES_TRY(palette->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_object_instances:
						object_instances = lak::unique_ptr<object_instances_t>::make();
						RES_TRY(object_instances->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_fade_in_frame:
						fade_in_frame = lak::unique_ptr<fade_in_frame_t>::make();
						RES_TRY(fade_in_frame->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_fade_out_frame:
						fade_out_frame = lak::unique_ptr<fade_out_frame_t>::make();
						RES_TRY(fade_out_frame->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_fade_in:
						fade_in = lak::unique_ptr<fade_in_t>::make();
						RES_TRY(fade_in->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_fade_out:
						fade_out = lak::unique_ptr<fade_out_t>::make();
						RES_TRY(fade_out->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_events:
						events = lak::unique_ptr<events_t>::make();
						RES_TRY(
						  events->read(game, reader).RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_play_header:
						play_head = lak::unique_ptr<play_header_r>::make();
						RES_TRY(play_head->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_additional_items:
						additional_item = lak::unique_ptr<additional_item_t>::make();
						RES_TRY(additional_item->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_additional_items_instances:
						additional_item_instance =
						  lak::unique_ptr<additional_item_instance_t>::make();
						RES_TRY(additional_item_instance->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_layers:
						layers = lak::unique_ptr<layers_t>::make();
						RES_TRY(
						  layers->read(game, reader).RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_virtual_size:
						virtual_size = lak::unique_ptr<virtual_size_t>::make();
						RES_TRY(virtual_size->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::demo_file_path:
						demo_file_path = lak::unique_ptr<demo_file_path_t>::make();
						RES_TRY(demo_file_path->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::random_seed:
						random_seed = lak::unique_ptr<random_seed_t>::make();
						RES_TRY(random_seed->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_layer_effect:
						layer_effect = lak::unique_ptr<layer_effect_t>::make();
						RES_TRY(layer_effect->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_bluray:
						blueray = lak::unique_ptr<blueray_t>::make();
						RES_TRY(blueray->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::movement_timer_base:
						movement_time_base = lak::unique_ptr<movement_time_base_t>::make();
						RES_TRY(movement_time_base->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::mosaic_image_table:
						mosaic_image_table = lak::unique_ptr<mosaic_image_table_t>::make();
						RES_TRY(mosaic_image_table->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_effects:
						effects = lak::unique_ptr<effects_t>::make();
						RES_TRY(effects->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_iphone_options:
						iphone_options = lak::unique_ptr<iphone_options_t>::make();
						RES_TRY(iphone_options->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::frame_chunk334C:
						chunk334C = lak::unique_ptr<chunk_334C_t>::make();
						RES_TRY(chunk334C->read(game, reader)
						          .RES_ADD_TRACE("frame::item_t::read"));
						break;

					case chunk_t::last:
						end = lak::unique_ptr<last_t>::make();
						RES_TRY(
						  end->read(game, reader).RES_ADD_TRACE("frame::item_t::read"));
						[[fallthrough]];

					default:
						not_finished = false;
						break;
				}
			}

			if (!reader.empty())
			{
				WARNING("There is still ",
				        reader.remaining().size(),
				        " bytes left in the frame item");
			}

			return lak::ok_t{};
		}

		error_t item_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX '%s'##%zX",
			              (size_t)entry.ID,
			              (name ? lak::strconv<char>(name->value).c_str() : ""),
			              entry.position())
			{
				entry.view(srcexp);

				if (name)
				{
					RES_TRY(name->view(srcexp, "Name", true)
					          .RES_ADD_TRACE("frame::item_t::view"));
				}
				if (header)
				{
					RES_TRY(header->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (password)
				{
					RES_TRY(password->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (palette)
				{
					RES_TRY(palette->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (object_instances)
				{
					RES_TRY(object_instances->view(srcexp).RES_ADD_TRACE(
					  "frame::item_t::view"));
				}
				if (fade_in_frame)
				{
					RES_TRY(
					  fade_in_frame->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (fade_out_frame)
				{
					RES_TRY(
					  fade_out_frame->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (fade_in)
				{
					RES_TRY(fade_in->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (fade_out)
				{
					RES_TRY(fade_out->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (events)
				{
					RES_TRY(events->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (play_head)
				{
					RES_TRY(
					  play_head->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (additional_item)
				{
					RES_TRY(additional_item->view(srcexp).RES_ADD_TRACE(
					  "frame::item_t::view"));
				}
				if (layers)
				{
					RES_TRY(layers->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (layer_effect)
				{
					RES_TRY(
					  layer_effect->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (virtual_size)
				{
					RES_TRY(
					  virtual_size->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (demo_file_path)
				{
					RES_TRY(
					  demo_file_path->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (random_seed)
				{
					RES_TRY(
					  random_seed->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (blueray)
				{
					RES_TRY(blueray->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (movement_time_base)
				{
					RES_TRY(movement_time_base->view(srcexp).RES_ADD_TRACE(
					  "frame::item_t::view"));
				}
				if (mosaic_image_table)
				{
					RES_TRY(mosaic_image_table->view(srcexp).RES_ADD_TRACE(
					  "frame::item_t::view"));
				}
				if (effects)
				{
					RES_TRY(effects->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (iphone_options)
				{
					RES_TRY(
					  iphone_options->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (chunk334C)
				{
					RES_TRY(
					  chunk334C->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
				if (end)
				{
					RES_TRY(end->view(srcexp).RES_ADD_TRACE("frame::item_t::view"));
				}
			}

			return lak::ok_t{};
		}

		error_t handles_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			RES_TRY(entry.read(game, strm).RES_ADD_TRACE("frame::handles_t::read"));

			RES_TRY_ASSIGN(
			  auto span =,
			  entry.decode_body().RES_ADD_TRACE("frame::handles_t::read"));

			data_reader_t hstrm(span);

			handles.resize(hstrm.size() / 2);
			for (auto &handle : handles) handle = hstrm.read_u16().UNWRAP();

			return lak::ok_t{};
		}

		error_t handles_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Frame Handles");
		}

		error_t bank_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			DEFER(game.bank_completed = 0.0f);

			RES_TRY(entry.read(game, strm).RES_ADD_TRACE("frame::bank_t::read"));

			items.clear();

			size_t start_pos = strm.position();

			size_t max_tries = 0U;

			auto read_all_items = [&]() -> error_t
			{
				auto read_item = [&](auto &item) -> error_t
				{
					return item.read(game, strm)
					  .IF_ERR("Failed To Read Item ", items.size())
					  .RES_ADD_TRACE("frame::bank_t::read");
				};

				while (strm.remaining().size() >= 2 &&
				       (chunk_t)strm.peek_u16().UNWRAP() == chunk_t::frame)
				{
					item_t item;
					RES_TRY(read_item(item)
					          .if_ok([&](auto &&) { items.push_back(lak::move(item)); })
					          .or_else(
					            [&](const auto &err) -> error_t
					            {
						            if (max_tries == 0) return lak::err_t{err};
						            ERROR(err);
						            DEBUG("Continuing...");
						            --max_tries;
						            return lak::ok_t{};
					            }));

					game.bank_completed =
					  float(double(strm.position() - start_pos) / double(strm.size()));
				}
				return lak::ok_t{};
			};

			RES_TRY(read_all_items().or_else(
			  [&](const auto &err) -> error_t
			  {
				  if (skip_broken_items) return lak::ok_t{};
				  return lak::err_t{err};
			  }));

			return lak::ok_t{};
		}

		error_t bank_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Frame Bank (%zu Items)##%zX",
			              (size_t)entry.ID,
			              items.size(),
			              entry.position())
			{
				entry.view(srcexp);

				for (const item_t &item : items)
				{
					RES_TRY(item.view(srcexp).RES_ADD_TRACE("frame::bank_t::view"));
				}
			}

			return lak::ok_t{};
		}
	}
}
