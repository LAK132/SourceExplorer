#ifndef SRCEXP_DATA_REF_HPP
#define SRCEXP_DATA_REF_HPP

#include <lak/array.hpp>
#include <lak/memory.hpp>
#include <lak/span.hpp>

namespace SourceExplorer
{
	struct _data_ref
	{
		lak::shared_ptr<_data_ref> _parent = {};
		lak::span<byte_t> _parent_span     = {};
		lak::array<byte_t> _data           = {};

		_data_ref()                             = default;
		_data_ref(const _data_ref &)            = default;
		_data_ref(_data_ref &&)                 = default;
		_data_ref &operator=(const _data_ref &) = default;
		_data_ref &operator=(_data_ref &&)      = default;

		_data_ref(lak::array<byte_t> data)
		: _parent(), _parent_span(), _data(lak::move(data))
		{
		}

		_data_ref(const lak::shared_ptr<_data_ref> &parent,
		          size_t offset,
		          size_t count,
		          lak::array<byte_t> data)
		: _parent(parent), _data(lak::move(data))
		{
			ASSERT(_parent);
			_parent_span = lak::span(_parent->_data).subspan(offset, count);
		}

		inline lak::shared_ptr<_data_ref> parent() const { return _parent; }
		inline lak::span<byte_t> parent_span() const { return _parent_span; }
		inline size_t size() const { return _data.size(); }
		inline const byte_t *data() const { return _data.data(); }
		inline byte_t *data() { return _data.data(); }
		inline const lak::array<byte_t> &get() const { return _data; }
		inline lak::array<byte_t> &get() { return _data; }

		inline operator lak::span<const byte_t>() const
		{
			return lak::span(_data);
		}
		inline operator lak::span<byte_t>() { return lak::span(_data); }
	};

	using data_ref_ptr_t = lak::shared_ptr<_data_ref>;

	static data_ref_ptr_t make_data_ref_ptr(lak::array<byte_t> data)
	{
		return lak::shared_ptr<_data_ref>::make(lak::move(data));
	}

	static data_ref_ptr_t make_data_ref_ptr(data_ref_ptr_t parent,
	                                        size_t offset,
	                                        size_t count,
	                                        lak::array<byte_t> data)
	{
		return lak::shared_ptr<_data_ref>::make(
		  parent, offset, count, lak::move(data));
	}

	struct data_ref_span_t : lak::span<byte_t>
	{
		data_ref_ptr_t _source = {};

		data_ref_span_t()                                   = default;
		data_ref_span_t(const data_ref_span_t &)            = default;
		data_ref_span_t &operator=(const data_ref_span_t &) = default;

		data_ref_span_t(data_ref_ptr_t src,
		                size_t offset = 0,
		                size_t count  = lak::dynamic_extent)
		: lak::span<byte_t>(
		    src ? lak::span<byte_t>(src->get()).subspan(offset, count)
		        : lak::span<byte_t>()),
		  _source(src)
		{
		}

		data_ref_span_t ref_subspan(size_t offset = 0,
		                            size_t count  = lak::dynamic_extent) const
		{
			if (!_source) return {};
			return data_ref_span_t(
			  _source, size_t(data() - _source->data()) + offset, count);
		}

		data_ref_span_t source_span() const
		{
			return _source ? data_ref_span_t{_source} : data_ref_span_t{};
		}

		data_ref_span_t parent_span() const
		{
			if (!_source || !_source->_parent) return {};
			return data_ref_span_t(_source->_parent,
			                       _source->_parent_span.begin() -
			                         _source->_parent->_data.begin(),
			                       _source->_parent_span.size());
		}

		lak::result<size_t> position() const
		{
			if (!_source) return lak::err_t{};
			return lak::ok_t{size_t(data() - _source->data())};
		}

		lak::result<size_t> root_position() const
		{
			if (!_source) return lak::err_t{};
			if (!_source->_parent)
				return lak::ok_t{size_t(data() - _source->data())};
			return parent_span().root_position();
		}

		void reset()
		{
			static_cast<lak::span<byte_t> &>(*this) = {};
			_source.reset();
		}
	};

	static data_ref_ptr_t make_data_ref_ptr(data_ref_span_t parent,
	                                        lak::array<byte_t> data)
	{
		if (!parent._source)
			return SourceExplorer::make_data_ref_ptr(lak::move(data));
		else
			return lak::shared_ptr<_data_ref>::make(parent._source,
			                                        parent.position().unwrap(),
			                                        parent.size(),
			                                        lak::move(data));
	}

	static data_ref_ptr_t copy_data_ref_ptr(data_ref_span_t parent)
	{
		if (!parent._source)
			return {};
		else
			return SourceExplorer::make_data_ref_ptr(
			  parent, lak::array<byte_t>(parent.begin(), parent.end()));
	}
}

#endif
