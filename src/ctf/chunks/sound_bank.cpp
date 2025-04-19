#include "sound_bank.hpp"

#include "../explorer.hpp"

namespace srcexp
{
	namespace sound
	{
		error_t item_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			const auto start = strm.position();

			if (game.old_game)
			{
				const size_t header_size = 0x18;
				RES_TRY(entry.read(game, strm, false));
				RES_TRY_ASSIGN(data_reader_t hstrm =, entry.decode_body(header_size));
				CHECK_REMAINING(hstrm, header_size);
				TRY_ASSIGN(checksum =, hstrm.read_u32());
				TRY_ASSIGN(references =, hstrm.read_u32());
				TRY_ASSIGN(decomp_len =, hstrm.read_u32());
				TRY_ASSIGN(const uint32_t _flags =, hstrm.read_u32());
				flags = static_cast<flags_t>(_flags);
				TRY_ASSIGN(frequency =, hstrm.read_u32());
				TRY_ASSIGN(name_len =, hstrm.read_u32());
			}
			else if (game.host == host_system_t::android ||
			         game.host == host_system_t::ios ||
			         game.host == host_system_t::html5 ||
			         game.host == host_system_t::flash)
			{
				entry.read_init(game);
				RES_TRY(entry.read_head(game, strm, 0, false)
				          .RES_ADD_TRACE("sound::item_t::read"));

				TRY_ASSIGN(entry.handle =, strm.read_u16());
				DEBUG_EXPR(entry.handle);

				size_t char_size = game.unicode ? sizeof(char16_t) : sizeof(char8_t);

				if (game.host == host_system_t::android)
				{
					TRY_ASSIGN(entry.handle =, strm.read_u16());
					TRY_ASSIGN(const uint32_t _flags =, strm.read_u16());
					flags = static_cast<flags_t>(_flags);
					TRY(strm.skip(4));
					TRY_ASSIGN(frequency =, strm.read_u32());
					if ((flags & flags_t::has_name) == flags_t::has_name)
					{
						TRY_ASSIGN(name_len =, strm.read_u16());
						CHECK_REMAINING(strm, (name_len * char_size));
						strm.skip((name_len * char_size)).unwrap();
					}
					decomp_len = 0U;
				}
				else if (game.host == host_system_t::ios)
				{
					TRY_ASSIGN(entry.handle =, strm.read_u16());
					TRY_ASSIGN(name_len =, strm.read_u16());
					CHECK_REMAINING(strm, (name_len * char_size) + 4);
					strm.skip((name_len * char_size) + 4).unwrap();
					TRY_ASSIGN(decomp_len =, strm.read_u32());
				}
				else if (game.host == host_system_t::html5)
				{
					TRY_ASSIGN(entry.handle =, strm.read_u16());
					TRY(strm.skip(1));
					TRY_ASSIGN(frequency =, strm.read_u32());
					TRY_ASSIGN(name_len =, strm.read_u16());
					CHECK_REMAINING(strm, (name_len * char_size));
					strm.skip((name_len * char_size)).unwrap();
					decomp_len = 0U;
				}
				else if (game.host == host_system_t::flash)
				{
					TRY_ASSIGN(entry.handle =, strm.read_u16());
					TRY_ASSIGN(name_len =, strm.read_u16());
					CHECK_REMAINING(strm, (name_len * char_size));
					strm.skip((name_len * char_size)).unwrap();
					decomp_len = 0U;
				}
				else
					ASSERT_UNREACHABLE();

				const auto pos = strm.position();
				strm.seek(start).UNWRAP();
				TRY_ASSIGN(entry.head.data =, strm.read_ref_span(pos - start));

				RES_TRY(entry
				          .read_body(game,
				                     strm,
				                     /* compressed */ false,
				                     {decomp_len})
				          .RES_ADD_TRACE("sound::item_t::read"));
			}
			else
			{
				const size_t header_size = 0x18;
				entry.read_init(game);

				RES_TRY(entry.read_head(game, strm, header_size, /* has_handle */ true)
				          .RES_ADD_TRACE("sound::item_t::read"));

				data_reader_t hstrm{entry.raw_head()};
				CHECK_REMAINING(hstrm, header_size);
				TRY_ASSIGN(checksum =, hstrm.read_u32());
				TRY_ASSIGN(references =, hstrm.read_u32());
				TRY_ASSIGN(decomp_len =, hstrm.read_u32());
				TRY_ASSIGN(const uint32_t _flags =, hstrm.read_u32());
				flags = static_cast<flags_t>(_flags);
				TRY_ASSIGN(frequency =, hstrm.read_u32());
				TRY_ASSIGN(name_len =, hstrm.read_u32());
				if ((flags & flags_t::decompressed) == flags_t::decompressed)
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

		error_t item_t::view(instance_t &inst) const
		{
			LAK_TREE_NODE(
			  "0x%zX %s##%zX", (size_t)entry.ID, "Sound", entry.position())
			{
				entry.view(inst);
				ImGui::Text("Checksum: 0x%zX", (size_t)checksum);
				ImGui::Text("References: 0x%zX", (size_t)references);
				ImGui::Text("Decompressed Length: 0x%zX", (size_t)decomp_len);
				ImGui::Text("Flags: 0x%zX", (size_t)flags);
				ImGui::Text("Frequency: 0x%zX", (size_t)frequency);
				ImGui::Text("Name Length: 0x%zX", (size_t)name_len);
			}

			return lak::ok_t{};
		}

		error_t end_t::view(instance_t &inst) const
		{
			return basic_view(inst, "Sound Bank End");
		}

		error_t bank_t::read(game_t &game, data_reader_t &strm)
		{
			MEMBER_FUNCTION_CHECKPOINT();

			DEFER(game.bank_completed = 0.0f);

			RES_TRY(entry.read(game, strm).RES_ADD_TRACE("sound::bank_t::read"));

			data_reader_t reader(entry.raw_body());

			uint32_t item_count;
			if (game.host == host_system_t::android ||
			    game.host == host_system_t::ios ||
			    game.host == host_system_t::html5 ||
			    game.host == host_system_t::flash)
			{
				TRY_ASSIGN([[maybe_unused]] const uint16_t some_count =,
				           reader.read_u16());
				TRY_ASSIGN(item_count =, reader.read_u16());
			}
			else
			{
				TRY_ASSIGN(item_count =, reader.read_u32());
			}

			if (item_count > 0xFF'FF)
			{
				ERROR("Not Sure About That Fam");
				return lak::err_t{error(error_type::out_of_data)};
			}

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

		error_t bank_t::view(instance_t &inst) const
		{
			LAK_TREE_NODE("0x%zX Sound Bank (%zu Items)##%zX",
			              (size_t)entry.ID,
			              items.size(),
			              entry.position())
			{
				entry.view(inst);

				for (const item_t &item : items)
				{
					RES_TRY(item.view(inst).RES_ADD_TRACE("sound::bank_t::view"));
				}

				if (end)
				{
					RES_TRY(end->view(inst).RES_ADD_TRACE("sound::bank_t::view"));
				}
			}

			return lak::ok_t{};
		}
	}
}
