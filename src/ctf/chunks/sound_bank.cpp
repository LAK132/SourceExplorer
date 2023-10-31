#include "sound_bank.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	namespace sound
	{
		error_t item_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();
			const size_t header_size = 0x18;

			const auto start = strm.position();

			if (game.old_game)
			{
				RES_TRY(entry.read(game, strm, false));
				RES_TRY_ASSIGN(data_reader_t hstrm =, entry.decode_body(header_size));
				CHECK_REMAINING(hstrm, header_size);
				TRY_ASSIGN(checksum =, hstrm.read_u32());
				TRY_ASSIGN(references =, hstrm.read_u32());
				TRY_ASSIGN(decomp_len =, hstrm.read_u32());
				TRY_ASSIGN(type =, hstrm.read_u32());
				TRY_ASSIGN(reserved =, hstrm.read_u32());
				TRY_ASSIGN(name_len =, hstrm.read_u32());
			}
			else
			{
				entry.read_init(game);

				RES_TRY(entry.read_head(game, strm, header_size, /* has_handle */ true)
				          .RES_ADD_TRACE("sound::item_t::read"));

				data_reader_t hstrm{entry.raw_head()};
				CHECK_REMAINING(hstrm, header_size);
				TRY_ASSIGN(checksum =, hstrm.read_u32());
				TRY_ASSIGN(references =, hstrm.read_u32());
				TRY_ASSIGN(decomp_len =, hstrm.read_u32());
				TRY_ASSIGN(type =, hstrm.read_u32());
				TRY_ASSIGN(reserved =, hstrm.read_u32());
				TRY_ASSIGN(name_len =, hstrm.read_u32());
				if (type == 0x21)
				{
					RES_TRY(entry
					          .read_body(game,
					                     strm,
					                     /* compressed */ false,
					                     {decomp_len})
					          .RES_ADD_TRACE("sound::item_t::read"));
				}
				else
				{
					RES_TRY(entry
					          .read_body(game,
					                     strm,
					                     /* compressed */ false)
					          .RES_ADD_TRACE("sound::item_t::read"));
				}
			}

			const auto size = strm.position() - start;
			strm.seek(start).UNWRAP();
			entry.ref_span = strm.read_ref_span(size).UNWRAP();
			DEBUG("Ref Span Size: ", entry.ref_span.size());

			return lak::ok_t{};
		}

		error_t item_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE(
			  "0x%zX %s##%zX", (size_t)entry.ID, "Sound", entry.position())
			{
				entry.view(srcexp);
				ImGui::Text("Checksum: 0x%zX", (size_t)checksum);
				ImGui::Text("References: 0x%zX", (size_t)references);
				ImGui::Text("Decompressed Length: 0x%zX", (size_t)decomp_len);
				ImGui::Text("Type: 0x%zX", (size_t)type);
				ImGui::Text("Reserved: 0x%zX", (size_t)reserved);
				ImGui::Text("Name Length: 0x%zX", (size_t)name_len);
			}

			return lak::ok_t{};
		}

		error_t end_t::view(source_explorer_t &srcexp) const
		{
			return basic_view(srcexp, "Sound Bank End");
		}

		error_t bank_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			DEFER(game.bank_completed = 0.0f);

			RES_TRY(entry.read(game, strm).RES_ADD_TRACE("sound::bank_t::read"));

			if (game.ccn)
			{
				WARNING(":TODO: HACK FIX FOR CCN SOUND BANK");
				return lak::ok_t{};
			}

			data_reader_t reader(entry.raw_body());

			TRY_ASSIGN(const auto item_count =, reader.read_u32());

			items.resize(item_count);

			DEBUG("Sound Bank Size: ", items.size());

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
					  .RES_ADD_TRACE("sound::bank_t::read");
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
				        " bytes left in the sound bank");
			}

			if (strm.remaining().size() >= 2 &&
			    (chunk_t)strm.peek_u16().UNWRAP() == chunk_t::sound_handles)
			{
				end = lak::unique_ptr<end_t>::make();
				RES_TRY_TRACE(end->read(game, strm));
			}

			return lak::ok_t{};
		}

		error_t bank_t::view(source_explorer_t &srcexp) const
		{
			LAK_TREE_NODE("0x%zX Sound Bank (%zu Items)##%zX",
			              (size_t)entry.ID,
			              items.size(),
			              entry.position())
			{
				entry.view(srcexp);

				for (const item_t &item : items)
				{
					RES_TRY(item.view(srcexp).RES_ADD_TRACE("sound::bank_t::view"));
				}

				if (end)
				{
					RES_TRY(end->view(srcexp).RES_ADD_TRACE("sound::bank_t::view"));
				}
			}

			return lak::ok_t{};
		}
	}
}
