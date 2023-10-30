#include "compressed.hpp"

#include "../explorer.hpp"

namespace SourceExplorer
{
	error_t compressed_chunk_t::view(source_explorer_t &srcexp) const
	{
		LAK_TREE_NODE(
		  "0x%zX Unknown Compressed##%zX", (size_t)entry.ID, entry.position())
		{
			entry.view(srcexp);

			if (ImGui::Button("View Compressed"))
			{
				RES_TRY_ASSIGN(
				  auto span =,
				  entry.decode_body().RES_ADD_TRACE("compressed_chunk_t::view"));

				data_reader_t strm(span);

				TRY(strm.skip(8));

				Inflate(strm.read_remaining_ref_span(), false, false)
				  .if_ok([&](auto ref_span) { srcexp.buffer = ref_span; })
				  .IF_ERR("Inflate Failed")
				  .discard();
			}
		}

		return lak::ok_t{};
	}
}
