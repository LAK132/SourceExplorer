#include "icon.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t icon_t::read(game_t &game, data_reader_t &strm)
	{
		MEMBER_FUNCTION_CHECKPOINT();

		RES_TRY(entry.read(game, strm).RES_ADD_TRACE("icon_t::read"));

		RES_TRY_ASSIGN(auto span =,
		               entry.decode_body().RES_ADD_TRACE("icon_t::read"));

		data_reader_t dstrm(span);

		TRY_ASSIGN(const auto data_begin =, dstrm.peek_u32());
		TRY(dstrm.seek(data_begin));

		std::vector<lak::color4_t> palette;
		palette.resize(16 * 16);

		for (auto &point : palette)
		{
			TRY_ASSIGN(point.b =, dstrm.read_u8());
			TRY_ASSIGN(point.g =, dstrm.read_u8());
			TRY_ASSIGN(point.r =, dstrm.read_u8());
			// TRY_ASSIGN(point.a =, dstrm.read_u8());
			point.a = 0xFF;
			TRY(dstrm.skip(1));
		}

		bitmap.resize({16, 16});

		CHECK_REMAINING(dstrm, bitmap.contig_size());

		for (size_t y = 0; y < bitmap.size().y; ++y)
		{
			for (size_t x = 0; x < bitmap.size().x; ++x)
			{
				bitmap[{x, (bitmap.size().y - 1) - y}] =
				  palette[dstrm.read_u8().UNWRAP()];
			}
		}

		CHECK_REMAINING(dstrm, bitmap.contig_size() / 8);

		for (size_t i = 0; i < bitmap.contig_size() / 8; ++i)
		{
			uint8_t mask = dstrm.read_u8().UNWRAP();
			for (size_t j = 0; j < 8; ++j)
			{
				if (0x1 & (mask >> (7 - j)))
				{
					bitmap[i * j].a = 0x00;
				}
				else
				{
					bitmap[i * j].a = 0xFF;
				}
			}
		}

		return lak::ok_t{};
	}

	error_t icon_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE("0x%zX Icon##%zX", (size_t)entry.ID, entry.position())
		{
			entry.view(srcexp);

			ImGui::Text("Image Size: %zu * %zu",
			            (size_t)bitmap.size().x,
			            (size_t)bitmap.size().y);

			if (ImGui::Button("View Image"))
			{
				srcexp.image = CreateTexture(bitmap, srcexp.graphics_mode);
			}
		}

		return lak::ok_t{};
	}
}
