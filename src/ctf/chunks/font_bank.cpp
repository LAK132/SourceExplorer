#include "font_bank.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	namespace font
	{
		error_t item_t::read(game_t &game, data_reader_t &strm)
		{
			if (game.cruf)
			{
				// u32 height
				// u32 width
				// u32 escapement
				// u32 orientation
				// u32 weight
				// u8 italic
				// u8 underline
				// u8 strikeout
				// u8 charset
				// u8 out_precision
				// u8 clip_precision
				// u8 quality
				// u8 pitch_and_family
				// char * 32 face_name

				entry.read_init(game);
				RES_TRY(entry.read_head(game, strm, 0U, true));
				RES_TRY(entry.read_body(game,
				                        strm,
				                        false,
				                        {/* height */ 4 +
				                         /* width */ 4 +
				                         /* escapement */ 4 +
				                         /* orientation */ 4 +
				                         /* weight */ 4 +
				                         /* italic */ 1 +
				                         /* underline */ 1 +
				                         /* strikeout */ 1 +
				                         /* charset */ 1 +
				                         /* out_precision */ 1 +
				                         /* clip_precision */ 1 +
				                         /* quality */ 1 +
				                         /* pitch_and_family */ 1 +
				                         /* face_name */ 32}));
				return lak::ok_t{};
			}
			else
			{
				return basic_item_t::read(game, strm);
			}
		}

		error_t item_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Font");
		}

		error_t end_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Font Bank End");
		}

		error_t bank_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			DEFER(game.bank_completed = 0.0f);

			RES_TRY(entry.read(game, strm).RES_ADD_TRACE("font::bank_t::read"));

			data_reader_t reader(entry.raw_body());

			TRY_ASSIGN(const auto item_count =, reader.read_u32());

			items.resize(item_count);

			DEBUG("Font Bank Size: ", items.size());

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
					  .RES_ADD_TRACE("font::bank_t::read");
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
				        " bytes left in the font bank");
			}

			if (strm.remaining().size() >= 2 &&
			    (chunk_t)strm.peek_u16().UNWRAP() == chunk_t::font_handles)
			{
				end = lak::unique_ptr<end_t>::make();
				RES_TRY_TRACE(end->read(game, strm));
			}

			return lak::ok_t{};
		}

		error_t bank_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Font Bank (%zu Items)##%zX",
			              (size_t)entry.ID,
			              items.size(),
			              entry.position())
			{
				entry.view(srcexp);

				for (const item_t &item : items)
				{
					RES_TRY(item.view(srcexp).RES_ADD_TRACE("font::bank_t::view"));
				}

				if (end)
				{
					RES_TRY(end->view(srcexp).RES_ADD_TRACE("font::bank_t::view"));
				}
			}

			return lak::ok_t{};
		}
	}
}
