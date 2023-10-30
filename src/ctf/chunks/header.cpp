#include "header.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t header_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		RES_TRY(entry.read(game, strm).RES_ADD_TRACE("header_t::read"));

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

	error_t header_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE("0x%zX Game Header##%zX", (size_t)entry.ID, entry.position())
		{
			entry.view(srcexp);

			RES_TRY(
			  title.view(srcexp, "Title", true).RES_ADD_TRACE("header_t::view"));
			RES_TRY(
			  author.view(srcexp, "Author", true).RES_ADD_TRACE("header_t::view"));
			RES_TRY(copyright.view(srcexp, "Copyright", true)
			          .RES_ADD_TRACE("header_t::view"));
			RES_TRY(output_path.view(srcexp, "Output Path")
			          .RES_ADD_TRACE("header_t::view"));
			RES_TRY(project_path.view(srcexp, "Project Path")
			          .RES_ADD_TRACE("header_t::view"));
			RES_TRY(about.view(srcexp, "About").RES_ADD_TRACE("header_t::view"));

			RES_TRY(vitalise_preview.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(menu.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(extension_path.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(extensions.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(extension_data.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(
			  additional_extensions.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(app_doc.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(other_extension.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(extension_list.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(icon.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(demo_version.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(security.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(binary_files.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(menu_images.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(
			  movement_extensions.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(object_bank_2.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(exe.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(protection.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(shaders.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(shaders2.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(extended_header.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(spacer.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(chunk224F.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(title2.view(srcexp).RES_ADD_TRACE("header_t::view"));

			RES_TRY(global_events.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(global_strings.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(
			  global_string_names.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(global_values.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(global_value_names.view(srcexp).RES_ADD_TRACE("header_t::view"));

			RES_TRY(frame_handles.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(frame_bank.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(object_bank.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(image_bank.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(sound_bank.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(music_bank.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(font_bank.view(srcexp).RES_ADD_TRACE("header_t::view"));

			RES_TRY(chunk2253.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(object_names.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(chunk2255.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(two_five_plus_object_properties.view(srcexp).RES_ADD_TRACE(
			  "header_t::view"));
			RES_TRY(chunk2257.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(object_properties.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(
			  truetype_fonts_meta.view(srcexp).RES_ADD_TRACE("header_t::view"));
			RES_TRY(truetype_fonts.view(srcexp).RES_ADD_TRACE("header_t::view"));

			for (auto &unk : unknown_strings)
			{
				RES_TRY(unk.view(srcexp).RES_ADD_TRACE("header_t::view"));
			}

			for (auto &unk : unknown_compressed)
			{
				RES_TRY(unk.view(srcexp).RES_ADD_TRACE("header_t::view"));
			}

			for (auto &unk : unknown_chunks)
			{
				RES_TRY(unk
				          .basic_view(srcexp,
				                      (lak::astring("Unknown ") +
				                       std::to_string(unk.entry.position()))
				                        .c_str())
				          .RES_ADD_TRACE("header_t::view"));
			}

			RES_TRY(last.view(srcexp).RES_ADD_TRACE("header_t::view"));
		}

		return lak::ok_t{};
	}
}
