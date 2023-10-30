#include "basic.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t chunk_entry_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		const auto strm_ref_span = strm.peek_remaining_ref_span();

		const auto start = strm.position();

		old = game.old_game;
		TRY_ASSIGN(ID = (chunk_t), strm.read_u16());
		TRY_ASSIGN(mode = (encoding_t), strm.read_u16());

		DEBUG("Type: ", GetTypeString(ID), " (", uintmax_t(ID), ")");
		if (GetTypeString(ID) == lak::astring("INVALID"))
			WARNING("Invalid Type Detected (", uintmax_t(ID), ")");
		DEBUG("Mode: ", (uint16_t)mode);
		DEBUG("Position: ", start);
		DEBUG("Position: ", strm_ref_span.position().UNWRAP());
		DEBUG("Root Position: ", strm_ref_span.root_position().UNWRAP());

		if ((mode == encoding_t::mode2 || mode == encoding_t::mode3) &&
		    _magic_key.size() < 256)
			GetEncryptionKey(game);

		TRY_ASSIGN(const auto chunk_size =, strm.read_u32());
		const auto chunk_data_end = strm.position() + chunk_size;

		CHECK_REMAINING(strm, chunk_size);

		if (mode == encoding_t::mode1)
		{
			TRY_ASSIGN(body.expected_size =, strm.read_u32());

			if (game.old_game)
			{
				if (chunk_size > 4)
				{
					TRY_ASSIGN(body.data =, strm.read_ref_span(chunk_size - 4));
				}
				else
					body.data.reset();
			}
			else
			{
				TRY_ASSIGN(const auto data_size =, strm.read_u32());

				TRY_ASSIGN(body.data =, strm.read_ref_span(data_size));

				if (strm.position() > chunk_data_end)
				{
					ERROR("Read Too Much Data");
				}
				else if (strm.position() < chunk_data_end)
				{
					ERROR("Read Too Little Data");
				}

				TRY(strm.seek(chunk_data_end));
			}
		}
		else
		{
			body.expected_size = 0;
			TRY_ASSIGN(body.data =, strm.read_ref_span(chunk_size));
		}

		const auto size = strm.position() - start;
		strm.seek(start).UNWRAP();
		ref_span = strm.read_ref_span(size).UNWRAP();
		DEBUG("Ref Span Size: ", ref_span.size());

		return lak::ok_t{};
	}

	void chunk_entry_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE("Entry Information##%zX", position())
		{
			if (old)
				ImGui::Text("Old Entry");
			else
				ImGui::Text("New Entry");
			ImGui::Text("Position: 0x%zX", position());
			ImGui::Text("Size: 0x%zX", ref_span.size());
			ImGui::Text("End: 0x%zX", position() + ref_span.size());

			ImGui::Text("ID: 0x%zX", (size_t)ID);
			ImGui::Text("Mode: MODE%zu", (size_t)mode);

			ImGui::Text("Head Position: 0x%zX", head.position());
			ImGui::Text("Head Expected Size: 0x%zX", head.expected_size);
			ImGui::Text("Head Size: 0x%zX", head.data.size());
			ImGui::Text("Head End: 0x%zX", head.position() + head.data.size());

			ImGui::Text("Body Position: 0x%zX", body.position());
			ImGui::Text("Body Expected Size: 0x%zX", body.expected_size);
			ImGui::Text("Body Size: 0x%zX", body.data.size());
			ImGui::Text("Body End: 0x%zX", body.position() + body.data.size());
		}

		if (ImGui::Button("View Memory")) srcexp.view = this;
	}

	void item_entry_t::read_init(game_t &game)
	{
		old  = game.old_game;
		mode = encoding_t::mode0;
	}

	error_t item_entry_t::read_head(game_t &game,
	                                data_reader_t &strm,
	                                size_t size,
	                                bool has_handle)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		DEBUG("Header Size: ", size);

		if (has_handle)
		{
			TRY_ASSIGN(handle =, strm.read_u32());
		}
		else
			handle = 0xFF'FF'FF'FF;
		DEBUG("Handle: ", handle);

		new_item = strm.peek_u32().unwrap_or(0U) == 0xFF'FF'FF'FF;
		if (!game.old_game && size > 0)
		{
			TRY_ASSIGN(head.data =, strm.read_ref_span(size));
			head.expected_size = 0;
		}

		return lak::ok_t{};
	}

	error_t item_entry_t::read_body(game_t &game,
	                                data_reader_t &strm,
	                                bool compressed,
	                                lak::optional<size_t> size)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		DEBUG("Compressed: ", compressed);

		if (new_item)
		{
			DEBUG(LAK_BRIGHT_YELLOW "New Item" LAK_SGR_RESET);
			mode       = encoding_t::mode4;
			compressed = false;
		}

		if (game.old_game || compressed)
		{
			TRY_ASSIGN(body.expected_size =, strm.read_u32());
		}
		else
			body.expected_size = 0;
		DEBUG("Body Expected Size: ", body.expected_size);

		size_t data_size = 0;
		if (size)
		{
			data_size = *size;
		}
		else if (game.old_game)
		{
			const size_t old_start = strm.position();
			// Figure out exactly how long the compressed data is
			// :TODO: It should be possible to figure this out without
			// actually decompressing it... surely...
			RES_TRY_ASSIGN(
			  const auto raw =,
			  StreamDecompress(strm, static_cast<unsigned int>(body.expected_size))
			    .RES_ADD_TRACE("item_entry_t::read"));

			if (raw.size() != body.expected_size)
			{
				WARNING("Actual decompressed size (",
				        raw.size(),
				        ") was not equal to the expected size (",
				        body.expected_size,
				        ").");
			}
			data_size = strm.position() - old_start;
			strm.seek(old_start).UNWRAP();
			DEBUG("Data Size: ", data_size);
		}
		else if (!new_item)
		{
			DEBUG("Pos: ", strm.position());
			TRY_ASSIGN(data_size =, strm.read_u32());
			DEBUG("Data Size: ", data_size);
		}

		CHECK_REMAINING(strm, data_size);
		TRY_ASSIGN(body.data =, strm.read_ref_span(data_size));
		DEBUG("Data Size: ", body.data.size());

		// hack because one of MMF1.5 or tinf_uncompress is a bitch
		if (game.old_game) mode = encoding_t::mode1;

		return lak::ok_t{};
	}

	error_t item_entry_t::read(game_t &game,
	                           data_reader_t &strm,
	                           bool compressed,
	                           size_t header_size,
	                           bool has_handle)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		const auto start = strm.position();
		read_init(game);
		RES_TRY(read_head(game, strm, header_size, has_handle));
		RES_TRY(read_body(game, strm, compressed));

		const auto size = strm.position() - start;
		strm.seek(start).UNWRAP();
		ref_span = strm.read_ref_span(size).UNWRAP();
		DEBUG("Ref Span Size: ", ref_span.size());

		return lak::ok_t{};
	}

	void item_entry_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE("Entry Information##%zX", position())
		{
			if (old)
				ImGui::Text("Old Entry");
			else
				ImGui::Text("New Entry");
			ImGui::Text("Position: 0x%zX", position());
			ImGui::Text("Size: 0x%zX", ref_span.size());
			ImGui::Text("End: 0x%zX", position() + ref_span.size());

			ImGui::Text("Handle: 0x%zX", (size_t)handle);

			ImGui::Text("Head Position: 0x%zX", head.position());
			ImGui::Text("Head Expected Size: 0x%zX", head.expected_size);
			ImGui::Text("Head Size: 0x%zX", head.data.size());
			ImGui::Text("Head End: 0x%zX", head.position() + head.data.size());

			ImGui::Text("Body Position: 0x%zX", body.position());
			ImGui::Text("Body Expected Size: 0x%zX", body.expected_size);
			ImGui::Text("Body Size: 0x%zX", body.data.size());
			ImGui::Text("Body End: 0x%zX", body.position() + body.data.size());
		}

		if (ImGui::Button("View Memory")) srcexp.view = this;
	}

	result_t<data_ref_span_t> basic_entry_t::decode_body(size_t max_size) const
	{
		MEMBER_FUNCTION_CHECKPOINT();

		if (old)
		{
			switch (mode)
			{
				case encoding_t::mode0:
					return lak::ok_t{body.data};

				case encoding_t::mode1:
				{
					data_reader_t reader(body.data);
					TRY_ASSIGN(const uint8_t magic =, reader.read_u8());
					TRY_ASSIGN(const uint16_t len =, reader.read_u16());
					if (magic == 0x0F && (size_t)len == body.expected_size)
					{
						auto result = reader.read_remaining_ref_span(max_size);
						DEBUG("Size: ", result.size());
						return lak::ok_t{result};
					}
					else
					{
						return Inflate(body.data,
						               true,
						               true,
						               std::min(body.expected_size, max_size))
						  .RES_ADD_TRACE("MODE1 Failed To Inflate")
						  .if_ok([](const auto &ref_span)
						         { DEBUG("Size: ", ref_span.size()); });
					}
				}

				case encoding_t::mode2:
					return lak::err_t{error(error_type::no_mode2_decoder)};

				case encoding_t::mode3:
					return lak::err_t{error(error_type::no_mode3_decoder)};

				default:
					return lak::err_t{error(error_type::invalid_mode)};
			}
		}
		else
		{
			switch (mode)
			{
				case encoding_t::mode4:
				{
					return LZ4DecodeReadSize(body.data)
					  .RES_ADD_TRACE("LZ4 Decode Failed")
					  .if_ok([](const auto &ref_span)
					         { DEBUG("Size: ", ref_span.size()); });
				}

				case encoding_t::mode3:
					[[fallthrough]];
				case encoding_t::mode2:
				{
					return Decrypt(body.data, ID, mode)
					  .RES_ADD_TRACE("MODE2/3 Failed To Decrypt")
					  .if_ok([](const auto &ref_span)
					         { DEBUG("Size: ", ref_span.size()); });
				}

				case encoding_t::mode1:
				{
					return Inflate(body.data, false, false, max_size)
					  .RES_ADD_TRACE("MODE1 Failed To Inflate")
					  .if_ok([](const auto &ref_span)
					         { DEBUG("Size: ", ref_span.size()); });
				}

				case encoding_t::mode0:
					[[fallthrough]];
				default:
				{
					if (body.data.size() > 0 && uint8_t(body.data[0]) == 0x78)
					{
						return lak::ok_t{lak::ok_or_err(
						  Inflate(body.data, false, false, max_size)
						    .if_ok(
						      [](const auto &ref_span)
						      {
							      if (ref_span.size() == 0)
								      WARNING("Inflated Data Was Empty");
							      DEBUG("Size: ", ref_span.size());
						      })
						    .map_err(
						      [this](const auto &err)
						      {
							      WARNING("Guess MODE1 Failed To Inflate: ", err);
							      DEBUG("Size: ", body.data.size());
							      return body.data;
						      }))};
					}
					else
					{
						return lak::ok_t{body.data};
					}
				}
			}
		}
	}

	result_t<data_ref_span_t> basic_entry_t::decode_head(size_t max_size) const
	{
		MEMBER_FUNCTION_CHECKPOINT();

		if (old)
		{
			switch (mode)
			{
				case encoding_t::mode0:
					return lak::err_t{error(error_type::no_mode0_decoder)};
				case encoding_t::mode1:
					return lak::err_t{error(error_type::no_mode1_decoder)};
				case encoding_t::mode2:
					return lak::err_t{error(error_type::no_mode2_decoder)};
				case encoding_t::mode3:
					return lak::err_t{error(error_type::no_mode3_decoder)};
				default:
					return lak::err_t{error(error_type::invalid_mode)};
			}
		}
		else
		{
			switch (mode)
			{
				case encoding_t::mode3:
				case encoding_t::mode2:
				{
					// :TODO: this was originally body not head, check that this change
					// is correct.
					return Decrypt(head.data, ID, mode)
					  .RES_ADD_TRACE("MODE2/3 Failed To Decrypt")
					  .if_ok([](const auto &ref_span)
					         { DEBUG("Size: ", ref_span.size()); });
				}

				case encoding_t::mode1:
				{
					return Inflate(head.data, false, false, max_size)
					  .RES_ADD_TRACE("MODE1 Failed To Inflate")
					  .if_ok([](const auto &ref_span)
					         { DEBUG("Size: ", ref_span.size()); });
				}

				case encoding_t::mode4:
					[[fallthrough]];
				case encoding_t::mode0:
					[[fallthrough]];
				default:
				{
					if (head.data.size() > 0 && uint8_t(head.data[0]) == 0x78)
					{
						return lak::ok_t{lak::ok_or_err(
						  Inflate(head.data, false, false, max_size)
						    .if_ok(
						      [](const auto &ref_span)
						      {
							      if (ref_span.size() == 0)
								      WARNING("Inflated Data Was Empty");
							      DEBUG("Size: ", ref_span.size());
						      })
						    .map_err(
						      [this](const auto &err)
						      {
							      WARNING("Guess MODE1 Failed To Inflate: ", err);
							      DEBUG("Size: ", head.data.size());
							      return head.data;
						      }))};
					}
					else
					{
						return lak::ok_t{head.data};
					}
				}
			}
		}
	}

	const data_ref_span_t &basic_entry_t::raw_body() const { return body.data; }

	const data_ref_span_t &basic_entry_t::raw_head() const { return head.data; }

	error_t basic_chunk_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();
		error_t result = entry.read(game, strm);
		return result;
	}

	error_t basic_chunk_t::basic_view(source_explorer_t &srcexp,
	                                  const char *name) const
	{
		LAK_TREE_NODE("0x%zX %s##%zX", (size_t)entry.ID, name, entry.position())
		{
			entry.view(srcexp);
		}

		return lak::ok_t{};
	}

	error_t basic_item_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();
		error_t result = entry.read(game, strm, true);
		return result;
	}

	error_t basic_item_t::basic_view(source_explorer_t &srcexp,
	                                 const char *name) const
	{
		LAK_TREE_NODE("0x%zX %s##%zX", (size_t)entry.ID, name, entry.position())
		{
			entry.view(srcexp);
		}

		return lak::ok_t{};
	}
}