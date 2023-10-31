#include "image_bank.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	namespace image
	{
		error_t item_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			bool optimised_image =
			  game.game.extended_header &&
			  ((game.game.extended_header->build_flags &
			    build_flags_t::optimize_image_size) != build_flags_t::none) &&
			  ((game.game.extended_header->build_flags & build_flags_t::unknown3) ==
			   build_flags_t::none);

			const auto strm_start = strm.position();
			if (game.ccn)
			{
				CHECKPOINT();

				RES_TRY(entry.read(game, strm, true, 10)
				          .RES_ADD_TRACE("image::item_t::read"));

				const uint16_t gmode = entry.handle >> 16;
				switch (gmode)
				{
					case 0:
						graphics_mode = graphics_mode_t::RGBA32;
						flags         = image_flag_t::none;
						break;

					case 3:
						graphics_mode = graphics_mode_t::RGB24;
						flags         = image_flag_t::none;
						break;

					case 5:
						graphics_mode = graphics_mode_t::JPEG;
						flags         = image_flag_t::none;
						break;

					default:
						WARNING("Unknown Graphics Mode: ", gmode);
						break;
				}

				data_position = 0;

				if (!game.old_game && game.product_build >= 284) --entry.handle;

				auto hstrm = data_reader_t(entry.raw_head());

				TRY_ASSIGN([[maybe_unused]] const auto unk1 =, hstrm.read_u16());
				TRY_ASSIGN(size.x =, hstrm.read_u16());
				TRY_ASSIGN(size.y =, hstrm.read_u16());
				TRY_ASSIGN([[maybe_unused]] const auto unk2 =, hstrm.read_u16());
				TRY_ASSIGN([[maybe_unused]] const auto unk3 =, hstrm.read_u16());
			}
			else
			{
				data_ref_span_t span;
				if (optimised_image)
				{
					CHECKPOINT();

					const size_t header_size = 0x24;
					RES_TRY(entry.read(game, strm, false, header_size)
					          .RES_ADD_TRACE("image::item_t::read"));

					data_position = strm.position();

					RES_TRY_ASSIGN(span =,
					               entry.decode_head(header_size)
					                 .RES_ADD_TRACE("image::item_t::read"));

					if (span.size() < header_size)
						return lak::err_t{error(error_type::out_of_data)};
				}
				else
				{
					CHECKPOINT();

					RES_TRY(
					  entry.read(game, strm, true).RES_ADD_TRACE("image::item_t::read"));

					if (!game.old_game && game.product_build >= 284) --entry.handle;

					RES_TRY_ASSIGN(span =,
					               entry.decode_body(176 + (game.old_game ? 16 : 80))
					                 .RES_ADD_TRACE("image::item_t::read"));
				}
				auto istrm = data_reader_t(span);

				DEBUG("Handle: ", entry.handle);

				if (game.old_game)
				{
					TRY_ASSIGN(checksum =, istrm.read_u16());
				}
				else
				{
					TRY_ASSIGN(checksum =, istrm.read_u32());
				}
				TRY_ASSIGN(reference =, istrm.read_u32());
				if (optimised_image) TRY(istrm.skip(4));
				TRY_ASSIGN(data_size =, istrm.read_u32());
				TRY_ASSIGN(size.x =, istrm.read_u16());
				TRY_ASSIGN(size.y =, istrm.read_u16());
				TRY_ASSIGN(const uint8_t gmode =, istrm.read_u8());
				switch (gmode)
				{
					case 2:
						graphics_mode = graphics_mode_t::RGB8;
						break;
					case 3:
						graphics_mode = graphics_mode_t::RGB8;
						break;
					case 4:
						graphics_mode = graphics_mode_t::BGR24;
						break;
					case 6:
						graphics_mode = graphics_mode_t::RGB15;
						break;
					case 7:
						graphics_mode = graphics_mode_t::RGB16;
						break;
					case 8:
						graphics_mode = graphics_mode_t::BGRA32;
						break;
				}
				TRY_ASSIGN(flags = (image_flag_t), istrm.read_u8());
#if 0
				if (graphics_mode == graphics_mode_t::RGB8)
				{
					TRY_ASSIGN(palette_entries =, istrm.read_u8());
					for (size_t i = 0; i < palette.size();
					     ++i) // where is this size coming from???
						palette[i] = ColorFrom32bitRGBA(istrm); // not sure if RGBA or BGRA
					TRY_ASSIGN(count =, strm.read_u32());
				}
#endif
				if (!game.old_game)
				{
					TRY_ASSIGN(unknown =, istrm.read_u16());
				}
				TRY_ASSIGN(hotspot.x =, istrm.read_u16());
				TRY_ASSIGN(hotspot.y =, istrm.read_u16());
				TRY_ASSIGN(action.x =, istrm.read_u16());
				TRY_ASSIGN(action.y =, istrm.read_u16());

				if (!game.old_game) transparent = ColorFrom32bitRGBA(istrm).UNWRAP();

				if (optimised_image)
				{
					ASSERT_EQUAL(strm.position(), data_position);
					TRY_ASSIGN(entry.body.data =, strm.read_ref_span(data_size));
					data_position = 0;

					const auto strm_end = strm.position();
					TRY(strm.seek(strm_start));
					TRY_ASSIGN(entry.ref_span =,
					           strm.read_ref_span(strm_end - strm_start));
					DEBUG("Corrected Ref Span Size: ", entry.ref_span.size());
				}
				else
				{
					data_position = istrm.position();
				}
			}

			switch (graphics_mode)
			{
				case graphics_mode_t::RGBA32:
				case graphics_mode_t::BGRA32:
					padding = 0U;
					break;
				case graphics_mode_t::RGB24:
				case graphics_mode_t::BGR24:
					if ((flags & image_flag_t::RLET) != image_flag_t::none)
						padding = (size.x * 3U) % 2;
					else if (optimised_image)
						padding = (size.x * 3U) % 2;
					else if (game.ccn)
						padding = uint16_t(lak::slack<size_t>(size.x * 3U, 4));
					else if (game.old_game)
						padding = ((size.x * 3U) % 2) * 3U;
					else if (game.product_build < 280)
						padding = ((size.x * 3U) % 2) * 3U;
					else
						padding = (size.x % 2) * 3U;
					break;
				case graphics_mode_t::RGB16:
				case graphics_mode_t::RGB15:
					padding = 0U;
					break;
				case graphics_mode_t::RGB8:
					if ((flags & image_flag_t::RLET) != image_flag_t::none)
						padding = size.x % 2;
					else if (optimised_image)
						padding = size.x % 2;
					else if (game.ccn)
						padding = uint16_t(lak::slack<size_t>(size.x, 4));
					else if (game.old_game)
						padding = size.x % 2;
					else if (game.product_build < 280)
						padding = size.x % 2;
					else
						padding = size.x % 2;
					break;
				case graphics_mode_t::JPEG:
					padding = 0U;
					break;
			}

			if (game.ccn)
				alpha_padding = 0;
			else
				alpha_padding = uint16_t(lak::slack<size_t>(size.x, 4));

			return lak::ok_t{};
		}

		error_t item_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Image##%zX", (size_t)entry.handle, entry.position())
			{
				entry.view(srcexp);

				ImGui::Text("Checksum: 0x%zX", (size_t)checksum);
				ImGui::Text("Reference: 0x%zX", (size_t)reference);
				ImGui::Text("Data Size: 0x%zX", (size_t)data_size);
				ImGui::Text("Image Size: (%zu, %zu)", (size_t)size.x, (size_t)size.y);
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
				ImGui::Text("Image Flags: %s (0x%zX)",
				            GetImageFlagString(flags).c_str(),
				            (size_t)flags);
				ImGui::Text("Unknown: 0x%zX", (size_t)unknown);
				ImGui::Text(
				  "Hotspot: (%zu, %zu)", (size_t)hotspot.x, (size_t)hotspot.y);
				ImGui::Text("Action: (%zu, %zu)", (size_t)action.x, (size_t)action.y);
				{
					lak::vec4f_t col = ((lak::vec4f_t)transparent) / 255.0f;
					ImGui::ColorEdit4("Transparent", &col.x);
				}
				ImGui::Text("Data Position: 0x%zX", data_position);
				ImGui::Text("Padding: 0x%zX", size_t(padding));
				ImGui::Text("Alpha Padding: 0x%zX", size_t(alpha_padding));

				if (ImGui::Button("View Image"))
				{
					image(srcexp.dump_color_transparent)
					  .if_ok(
					    [&](lak::image4_t &&img)
					    { srcexp.image = CreateTexture(img, srcexp.graphics_mode); })
					  .IF_ERR("Failed To Read Image Data")
					  .discard();
				}
			}

			return lak::ok_t{};
		}

		result_t<data_ref_span_t> item_t::image_data() const
		{
			MEMBER_FUNCTION_CHECKPOINT();

			RES_TRY_ASSIGN(
			  auto span =,
			  entry.decode_body().RES_ADD_TRACE("image::item_t::image_data"));

			data_reader_t strm(span);

			TRY(strm.seek(data_position));

			if ((flags & image_flag_t::LZX) == image_flag_t::LZX)
			{
				if (entry.mode == encoding_t::mode4)
				{
					return lak::ok_t{strm.read_remaining_ref_span()};
				}
				else
				{
					TRY_ASSIGN([[maybe_unused]] const uint32_t decompressed_length =,
					           strm.read_u32());

					TRY_ASSIGN(const uint32_t compressed_length =, strm.read_u32());

					CHECK_REMAINING(strm, compressed_length);

					return strm.read_ref_span(compressed_length)
					  .RES_MAP_TO_TRACE("item_t::image_data: read filed")
					  .map(
					    [](const data_ref_span_t &ref_span) -> data_ref_span_t
					    {
						    return lak::ok_or_err(
						      Inflate(ref_span, false, false)
						        .map_err([&](auto &&) { return ref_span; }));
					    });
				}
			}
			else
			{
				return lak::ok_t{strm.read_remaining_ref_span()};
			}
		}

		bool item_t::need_palette() const
		{
			return graphics_mode == graphics_mode_t::RGB8;
		}

		result_t<lak::image4_t> item_t::image(
		  const bool color_transparent, const lak::color4_t palette[256]) const
		{
			MEMBER_FUNCTION_CHECKPOINT();

			lak::image4_t img = {};

			RES_TRY_ASSIGN(auto span =,
			               image_data().RES_ADD_TRACE("image::item_t::image"));

			if (graphics_mode == graphics_mode_t::JPEG)
			{
				int x, y, n;
				uint8_t *data =
				  stbi_load_from_memory(reinterpret_cast<const uint8_t *>(span.data()),
				                        static_cast<int>(span.size()),
				                        &x,
				                        &y,
				                        &n,
				                        4);
				DEFER(stbi_image_free(data));

				if (x != size.x || y != size.y)
				{
					return lak::err_t{
					  error(lak::streamify("jpeg decode failed, expected size (",
					                       size.x,
					                       ", ",
					                       size.y,
					                       ") got (",
					                       x,
					                       ", ",
					                       y,
					                       ")"))};
				}
				else
				{
					img = lak::image_from_rgba32(reinterpret_cast<const byte_t *>(data),
					                             lak::vec2s_t{size_t(x), size_t(y)});
				}
			}
			else
			{
				data_reader_t strm(span);

				img.resize(lak::vec2s_t(size));

				[[maybe_unused]] size_t bytes_read;
				if ((flags & (image_flag_t::RLE | image_flag_t::RLEW |
				              image_flag_t::RLET)) != image_flag_t::none)
				{
					RES_TRY_ASSIGN(bytes_read =,
					               ReadRLE(strm, img, graphics_mode, padding, palette));
				}
				else
				{
					RES_TRY_ASSIGN(bytes_read =,
					               ReadRGB(strm, img, graphics_mode, padding, palette));
				}

				if ((flags & image_flag_t::RGBA) != image_flag_t::none)
				{
					// we already read the alpha data with the colour data
				}
				else if ((flags & image_flag_t::alpha) != image_flag_t::none)
				{
					RES_TRY(ReadAlpha(strm, img, alpha_padding));
				}
				else if (color_transparent)
				{
					ReadTransparent(transparent, img);
				}
				else
				{
					for (size_t i = 0; i < img.contig_size(); ++i) img[i].a = 255;
				}

				if (!strm.empty())
					WARNING(strm.remaining().size(), " Bytes Left Over In Image Data");
			}

			return lak::move_ok(img);
		}

		error_t end_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Image Bank End");
		}

		error_t bank_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			DEFER(game.bank_completed = 0.0f);

			RES_TRY(entry.read(game, strm).RES_ADD_TRACE("image::bank_t::read"));

			data_reader_t reader(entry.raw_body());

			uint32_t item_count = 0;
			if (game.ccn)
			{
				TRY_ASSIGN([[maybe_unused]] const auto unknown =, reader.read_u16());
				TRY_ASSIGN(item_count =, reader.read_u16());
			}
			else
			{
				TRY_ASSIGN(item_count =, reader.read_u32());
			}

			items.resize(item_count);

			DEBUG("Image Bank Size: ", items.size());

			size_t max_tries = max_item_read_fails;

			auto read_all_items = [&]() -> error_t
			{
				auto read_item = [&](auto &item) -> error_t
				{
					return item.read(game, reader)
					  .IF_ERR("Failed To Read Item ",
					          (&item - items.data()),
					          " Of ",
					          items.size())
					  .RES_ADD_TRACE("image::bank_t::read");
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
				        " bytes left in the image bank");
			}

			if (strm.remaining().size() >= 2 &&
			    (chunk_t)strm.peek_u16().UNWRAP() == chunk_t::image_handles)
			{
				end = lak::unique_ptr<end_t>::make();
				RES_TRY_TRACE(end->read(game, strm));
			}

			return lak::ok_t{};
		}

		error_t bank_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Image Bank (%zu Items)##%zX",
			              (size_t)entry.ID,
			              items.size(),
			              entry.position())
			{
				entry.view(srcexp);

				for (const item_t &item : items)
				{
					RES_TRY(item.view(srcexp).RES_ADD_TRACE("image::bank_t::view"));
				}

				if (end)
				{
					RES_TRY(end->view(srcexp).RES_ADD_TRACE("image::bank_t::view"));
				}
			}

			return lak::ok_t{};
		}
	}
}
