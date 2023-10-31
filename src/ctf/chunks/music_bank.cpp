#include "music_bank.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	namespace music
	{
		error_t item_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Music");
		}

		error_t end_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Music Bank End");
		}

		error_t bank_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			DEFER(game.bank_completed = 0.0f);

			RES_TRY(entry.read(game, strm).RES_ADD_TRACE("music::bank_t::read"));

			data_reader_t reader(entry.raw_body());

			TRY_ASSIGN(const auto item_count =, reader.read_u32());

			items.resize(item_count);

			DEBUG("Music Bank Size: ", items.size());

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
					  .RES_ADD_TRACE("music::bank_t::read");
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
				  if (skip_broken_items)
				  {
					  ERROR(err);
					  return lak::ok_t{};
				  }
				  return lak::err_t{err};
			  }));

			if (!reader.empty())
			{
				WARNING("There is still ",
				        reader.remaining().size(),
				        " bytes left in the music bank");
			}

			if (strm.remaining().size() >= 2 &&
			    (chunk_t)strm.peek_u16().UNWRAP() == chunk_t::music_handles)
			{
				end = lak::unique_ptr<end_t>::make();
				RES_TRY_TRACE(end->read(game, strm));
			}

			return lak::ok_t{};
		}

		error_t bank_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Music Bank (%zu Items)##%zX",
			              (size_t)entry.ID,
			              items.size(),
			              entry.position())
			{
				entry.view(srcexp);

				for (const item_t &item : items)
				{
					RES_TRY(item.view(srcexp).RES_ADD_TRACE("music::bank_t::view"));
				}

				if (end)
				{
					RES_TRY(end->view(srcexp).RES_ADD_TRACE("music::bank_t::view"));
				}
			}

			return lak::ok_t{};
		}
	}
}
