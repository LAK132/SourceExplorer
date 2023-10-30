#ifndef SRCEXP_DATA_READER_HPP
#define SRCEXP_DATA_READER_HPP

#include "data_ref.hpp"

#include <lak/binary_reader.hpp>
#include <lak/debug.hpp>
#include <lak/result.hpp>

namespace SourceExplorer
{
	struct data_reader_t : public lak::binary_reader
	{
		data_ref_ptr_t _source;

		data_reader_t(data_ref_ptr_t src)
		: lak::binary_reader(src ? lak::span<const byte_t>(src->get())
		                         : lak::span<const byte_t>()),
		  _source(src)
		{
		}

		data_reader_t(data_ref_span_t src)
		: lak::binary_reader((lak::span<byte_t>)src), _source(src._source)
		{
		}

		data_ref_span_t peek_remaining_ref_span(size_t max_size = SIZE_MAX)
		{
			ASSERT(_source);
			const size_t offset = remaining().begin() - _source->_data.data();
			const size_t size   = std::min(remaining().size(), max_size);
			return data_ref_span_t(_source, offset, size);
		}

		lak::result<data_ref_span_t> read_ref_span(size_t size)
		{
			if (!_source) return lak::err_t{};
			if (size > remaining().size()) return lak::err_t{};
			const size_t offset = remaining().begin() - _source->_data.data();
			skip(size).UNWRAP();
			return lak::ok_t{data_ref_span_t(_source, offset, size)};
		}

		data_ref_span_t read_remaining_ref_span(size_t max_size = SIZE_MAX)
		{
			ASSERT(_source);
			const size_t offset = remaining().begin() - _source->_data.data();
			const size_t size   = std::min(remaining().size(), max_size);
			skip(size).UNWRAP();
			return data_ref_span_t(_source, offset, size);
		}

		lak::result<data_ref_ptr_t> read_ref_ptr(size_t size)
		{
			return read_ref_span(size).map(copy_data_ref_ptr);
		}

		data_ref_ptr_t copy_remaining()
		{
			return copy_data_ref_ptr(read_remaining_ref_span());
		}

		data_ref_ptr_t read_remaining_ref_ptr(size_t max_size,
		                                      lak::array<byte_t> data)
		{
			ASSERT(_source);
			const size_t offset = remaining().begin() - _source->_data.data();
			const size_t size   = std::min(remaining().size(), max_size);
			skip(size).UNWRAP();
			return make_data_ref_ptr(_source, offset, size, lak::move(data));
		}
	};
}

#endif
