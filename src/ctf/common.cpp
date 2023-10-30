#include "common.hpp"

#include "explorer.hpp"

namespace SourceExplorer
{
	result_t<data_ref_span_t> data_point_t::decode(const chunk_t ID,
	                                               const encoding_t mode) const
	{
		return Decode(data, ID, mode);
	}
}
