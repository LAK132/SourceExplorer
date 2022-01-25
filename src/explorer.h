// Copyright (c) Mathias Kaerlev 2012, LAK132 2019

// This file is part of Anaconda.

// Anaconda is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// Anaconda is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with Anaconda.  If not, see <http://www.gnu.org/licenses/>.

#ifndef EXPLORER_H
#define EXPLORER_H

#include "imgui_impl_lak.h"
#include "imgui_utils.hpp"
#include <imgui/misc/softraster/texture.h>
#include <imgui_memory_editor.h>
#include <imgui_stdlib.h>

#include "defines.h"
#include "encryption.h"
#include "stb_image.h"

#include <lak/binary_reader.hpp>
#include <lak/binary_writer.hpp>
#include <lak/debug.hpp>
#include <lak/file.hpp>
#include <lak/opengl/state.hpp>
#include <lak/opengl/texture.hpp>
#include <lak/result.hpp>
#include <lak/stdint.hpp>
#include <lak/strconv.hpp>
#include <lak/string.hpp>
#include <lak/tinflate.hpp>
#include <lak/trace.hpp>
#include <lak/unicode.hpp>

#include <assert.h>
#include <atomic>
#include <fstream>
#include <iostream>
#include <istream>
#include <iterator>
#include <memory>
#include <optional>
#include <stack>
#include <stdint.h>
#include <unordered_map>
#include <variant>
#include <vector>

#ifdef GetObject
#	undef GetObject
#endif

namespace fs = std::filesystem;

namespace SourceExplorer
{
	struct error
	{
		enum value_t : uint32_t
		{
			str_err,

			invalid_exe_signature,
			invalid_pe_signature,
			invalid_game_signature,
			invalid_pack_count,

			invalid_state,
			invalid_mode,
			invalid_chunk,

			no_mode0,
			no_mode1,
			no_mode2,
			no_mode3,

			out_of_data,

			inflate_failed,
			decrypt_failed,

			no_mode0_decoder,
			no_mode1_decoder,
			no_mode2_decoder,
			no_mode3_decoder,
		};

		std::vector<lak::pair<lak::trace, lak::u8string>> _trace;
		value_t _value = str_err;

		// error() {}
		// error(const error &other) : value(other.value), trace(other.trace) {}
		// error &operator=(const error &other)
		// {
		//   value = other.value;
		//   trace = other.trace;
		//   return *this;
		// }
		error()              = default;
		error(error &&)      = default;
		error(const error &) = default;
		error &operator=(error &&) = default;
		error &operator=(const error &) = default;

		template<typename... ARGS>
		error(lak::trace trace, value_t value, ARGS &&...args) : _value(value)
		{
			append_trace(lak::move(trace), lak::forward<ARGS>(args)...);
		}

		error(lak::trace trace, lak::u8string err) : _value(str_err)
		{
			append_trace(lak::move(trace), lak::move(err));
		}

		template<typename... ARGS>
		error &append_trace(lak::trace trace, ARGS &&...args)
		{
			_trace.emplace_back(lak::move(trace), lak::streamify(args...));
			return *this;
		}

		template<typename... ARGS>
		error append_trace(lak::trace trace, ARGS &&...args) const
		{
			error result = *this;
			result.append_trace(lak::move(trace), lak::streamify(args...));
			return result;
		}

		inline lak::u8string value_string() const
		{
			switch (_value)
			{
				case str_err: return lak::as_u8string(_trace[0].second).to_string();
				case invalid_exe_signature:
					return lak::as_u8string("Invalid EXE Signature").to_string();
				case invalid_pe_signature:
					return lak::as_u8string("Invalid PE Signature").to_string();
				case invalid_game_signature:
					return lak::as_u8string("Invalid Game Header").to_string();
				case invalid_state:
					return lak::as_u8string("Invalid State").to_string();
				case invalid_mode: return lak::as_u8string("Invalid Mode").to_string();
				case invalid_chunk:
					return lak::as_u8string("Invalid Chunk").to_string();
				case no_mode0: return lak::as_u8string("No MODE0").to_string();
				case no_mode1: return lak::as_u8string("No MODE1").to_string();
				case no_mode2: return lak::as_u8string("No MODE2").to_string();
				case no_mode3: return lak::as_u8string("No MODE3").to_string();
				case out_of_data: return lak::as_u8string("Out Of Data").to_string();
				case inflate_failed:
					return lak::as_u8string("Inflate Failed").to_string();
				case decrypt_failed:
					return lak::as_u8string("Decrypt Failed").to_string();
				case no_mode0_decoder:
					return lak::as_u8string("No MODE0 Decoder").to_string();
				case no_mode1_decoder:
					return lak::as_u8string("No MODE1 Decoder").to_string();
				case no_mode2_decoder:
					return lak::as_u8string("No MODE2 Decoder").to_string();
				case no_mode3_decoder:
					return lak::as_u8string("No MODE3 Decoder").to_string();
				default: return lak::as_u8string("Invalid Error Code").to_string();
			}
		}

		inline lak::u8string to_string() const
		{
			++lak::debug_indent;
			DEFER(--lak::debug_indent;);

			lak::u8string result;

			if (_trace.size() >= 2)
			{
				const auto &[trace, str] = _trace.back();

				result += lak::streamify("\n",
				                         lak::scoped_indenter::str(),
				                         trace,
				                         str.empty() ? "" : ": ",
				                         str);

				++lak::debug_indent;
			}
			if (_trace.size() >= 3)
			{
				for (size_t i = _trace.size() - 1; i-- > 1;)
				{
					const auto &[trace, str] = _trace[i];
					result += lak::streamify("\n",
					                         lak::scoped_indenter::str(),
					                         trace,
					                         str.empty() ? "" : ": ",
					                         str);
				}
			}
			if (_trace.size() >= 1)
			{
				const auto &[trace, str] = _trace.front();
				result += lak::streamify("\n", lak::scoped_indenter::str(), trace);
				if (_value != str_err) result += lak::streamify(": ", value_string());
				if (!str.empty()) result += lak::streamify(": ", str);
			}
			if (_trace.size() >= 2) --lak::debug_indent;

			return result;
		}

		inline friend std::ostream &operator<<(std::ostream &strm,
		                                       const error &err)
		{
			return strm << lak::string_view(err.to_string());
		}
	};

#define MAP_TRACE(ERR, ...)                                                   \
	[&](const auto &err) -> SourceExplorer::error                               \
	{                                                                           \
		static_assert(!lak::is_same_v<SourceExplorer::error,                      \
			                            lak::remove_cvref_t<decltype(err)>>);       \
		if constexpr (lak::is_same_v<lak::monostate,                              \
			                           lak::remove_cvref_t<decltype(err)>>)         \
		{                                                                         \
			return SourceExplorer::error(LINE_TRACE,                                \
				                           ERR __VA_OPT__(, ) __VA_ARGS__);           \
		}                                                                         \
		else                                                                      \
		{                                                                         \
			return SourceExplorer::error(                                           \
				LINE_TRACE, ERR, "" __VA_OPT__(, ) __VA_ARGS__, err);                 \
		}                                                                         \
	}

#define MAP_ERRCODE(CODE, ...) map_err(MAP_TRACE(CODE, __VA_ARGS__))

#define MAP_ERR(...) MAP_ERRCODE(SourceExplorer::error::str_err, __VA_ARGS__)

#define TRY_ASSIGN(A, ...)                                                    \
	RES_TRY_ASSIGN(A, __VA_ARGS__.MAP_ERR("assign failed"))

#define TRY(...) RES_TRY(__VA_ARGS__.MAP_ERR())

#define APPEND_TRACE(...)                                                     \
	[&](const SourceExplorer::error &err) -> SourceExplorer::error              \
	{ return err.append_trace(LINE_TRACE __VA_OPT__(, ) __VA_ARGS__); }

#define MAP_SE_ERR(...) map_err(APPEND_TRACE(__VA_ARGS__))

#define TRY_SE_ASSIGN(A, ...)                                                 \
	RES_TRY_ASSIGN(A, __VA_ARGS__.MAP_SE_ERR("assign failed"))

#define TRY_SE(...) RES_TRY(__VA_ARGS__.MAP_SE_ERR())

#define CHECK_REMAINING(STRM, EXPECTED)                                       \
	do                                                                          \
	{                                                                           \
		if (auto expected = (EXPECTED); STRM.remaining().size() < expected)       \
		{                                                                         \
			ERROR("Out Of Data: ",                                                  \
			      STRM.remaining().size(),                                          \
			      " Bytes Remaining, Expected ",                                    \
			      expected);                                                        \
			return lak::err_t{                                                      \
			  SourceExplorer::error(LINE_TRACE,                                     \
			                        SourceExplorer::error::out_of_data,             \
			                        STRM.remaining().size(),                        \
			                        " Bytes Remaining, Expected ",                  \
			                        expected)};                                     \
		}                                                                         \
	} while (false)

#define CHECK_POSITION(STRM, EXPECTED)                                        \
	do                                                                          \
	{                                                                           \
		if (auto expected = (EXPECTED); STRM.size() < expected)                   \
		{                                                                         \
			ERROR("Out Of Data: ",                                                  \
			      STRM.remaining().size(),                                          \
			      " Bytes Availible, Expected ",                                    \
			      expected);                                                        \
			return lak::err_t{                                                      \
			  SourceExplorer::error(LINE_TRACE,                                     \
			                        SourceExplorer::error::out_of_data,             \
			                        STRM.remaining().size(),                        \
			                        " Bytes Availible, Expected ",                  \
			                        expected)};                                     \
		}                                                                         \
	} while (false)

	template<typename T>
	using result_t = lak::result<T, error>;
	using error_t  = result_t<lak::monostate>;

	extern bool force_compat;
	extern std::vector<uint8_t> _magic_key;
	extern uint8_t _magic_char;

	enum class game_mode_t : uint8_t
	{
		_OLD,
		_284,
		_288,
		_290 // might be 292?
	};
	extern game_mode_t _mode;

	struct game_t;
	struct source_explorer_t;

	using texture_t =
	  std::variant<std::monostate, lak::opengl::texture, texture_color32_t>;

	struct pack_file_t
	{
		std::u16string filename;
		lak::array<byte_t> data;
		bool wide;
		uint32_t bingo;
	};

	struct _data_ref
	{
		std::shared_ptr<_data_ref> _parent = {};
		lak::span<byte_t> _parent_span     = {};
		lak::array<byte_t> _data           = {};

		_data_ref()                  = default;
		_data_ref(const _data_ref &) = default;
		_data_ref(_data_ref &&)      = default;
		_data_ref &operator=(const _data_ref &) = default;
		_data_ref &operator=(_data_ref &&) = default;

		_data_ref(lak::array<byte_t> data)
		: _parent(), _parent_span(), _data(lak::move(data))
		{
		}

		_data_ref(const std::shared_ptr<_data_ref> &parent,
		          size_t offset,
		          size_t count,
		          lak::array<byte_t> data)
		: _parent(parent), _data(lak::move(data))
		{
			ASSERT(_parent);
			_parent_span = lak::span(_parent->_data).subspan(offset, count);
		}

		inline std::shared_ptr<_data_ref> parent() const { return _parent; }
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

	using data_ref_ptr_t = std::shared_ptr<_data_ref>;

	static data_ref_ptr_t make_data_ref_ptr(lak::array<byte_t> data)
	{
		FUNCTION_CHECKPOINT();
		return std::make_shared<_data_ref>(lak::move(data));
	}

	static data_ref_ptr_t make_data_ref_ptr(data_ref_ptr_t parent,
	                                        size_t offset,
	                                        size_t count,
	                                        lak::array<byte_t> data)
	{
		FUNCTION_CHECKPOINT();
		return std::make_shared<_data_ref>(parent, offset, count, lak::move(data));
	}

	struct data_ref_span_t : lak::span<byte_t>
	{
		data_ref_ptr_t _source;

		data_ref_span_t()                        = default;
		data_ref_span_t(const data_ref_span_t &) = default;
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

		data_ref_span_t parent_span() const
		{
			FUNCTION_CHECKPOINT("data_ref_span_t::");
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
			FUNCTION_CHECKPOINT("data_ref_span_t::");
			static_cast<lak::span<byte_t> &>(*this) = {};
			_source.reset();
		}
	};

	static data_ref_ptr_t make_data_ref_ptr(data_ref_span_t parent,
	                                        lak::array<byte_t> data)
	{
		FUNCTION_CHECKPOINT();
		if (!parent._source)
			return make_data_ref_ptr(lak::move(data));
		else
			return std::make_shared<_data_ref>(parent._source,
			                                   parent.position().unwrap(),
			                                   parent.size(),
			                                   lak::move(data));
	}

	static data_ref_ptr_t copy_data_ref_ptr(data_ref_span_t parent)
	{
		FUNCTION_CHECKPOINT();
		if (!parent._source)
			return {};
		else
			return make_data_ref_ptr(
			  parent, lak::array<byte_t>(parent.begin(), parent.end()));
	}

	struct data_reader_t : lak::binary_reader
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
			FUNCTION_CHECKPOINT("data_reader_t::");
			ASSERT(_source);
			const size_t offset = remaining().begin() - _source->_data.data();
			const size_t size   = std::min(remaining().size(), max_size);
			DEBUG("Offset: ", offset);
			DEBUG("Size: ", size);
			return data_ref_span_t(_source, offset, size);
		}

		lak::result<data_ref_span_t> read_ref_span(size_t size)
		{
			FUNCTION_CHECKPOINT("data_reader_t::");
			if (!_source) return lak::err_t{};
			if (size > remaining().size()) return lak::err_t{};
			const size_t offset = remaining().begin() - _source->_data.data();
			skip(size).UNWRAP();
			return lak::ok_t{data_ref_span_t(_source, offset, size)};
		}

		data_ref_span_t read_remaining_ref_span(size_t max_size = SIZE_MAX)
		{
			FUNCTION_CHECKPOINT("data_reader_t::");
			ASSERT(_source);
			const size_t offset = remaining().begin() - _source->_data.data();
			const size_t size   = std::min(remaining().size(), max_size);
			DEBUG("Offset: ", offset);
			DEBUG("Size: ", size);
			skip(size).UNWRAP();
			return data_ref_span_t(_source, offset, size);
		}

		lak::result<data_ref_ptr_t> read_ref_ptr(size_t size)
		{
			FUNCTION_CHECKPOINT("data_reader_t::");
			return read_ref_span(size).map(copy_data_ref_ptr);
		}

		data_ref_ptr_t copy_remaining()
		{
			FUNCTION_CHECKPOINT("data_reader_t::");
			return copy_data_ref_ptr(read_remaining_ref_span());
		}

		data_ref_ptr_t read_remaining_ref_ptr(size_t max_size,
		                                      lak::array<byte_t> data)
		{
			FUNCTION_CHECKPOINT("data_reader_t::");
			ASSERT(_source);
			const size_t offset = remaining().begin() - _source->_data.data();
			const size_t size   = std::min(remaining().size(), max_size);
			skip(size).UNWRAP();
			return make_data_ref_ptr(_source, offset, size, lak::move(data));
		}
	};

	struct data_point_t
	{
		data_ref_span_t data;
		size_t expected_size;
		size_t position() const
		{
			return lak::ok_or_err(
			  data.position().map_err([](auto &&) -> size_t { return SIZE_MAX; }));
		}
		result_t<data_ref_span_t> decode(const chunk_t ID,
		                                 const encoding_t mode) const;
	};

	struct basic_entry_t
	{
		union
		{
			uint32_t handle;
			chunk_t ID;
		};
		encoding_t mode;
		bool old;

		data_ref_span_t ref_span;
		data_point_t head;
		data_point_t body;

		size_t position() const
		{
			return lak::ok_or_err(ref_span.position().map_err([](auto &&) -> size_t
			                                                  { return SIZE_MAX; }));
		}

		const data_ref_span_t &raw_head() const;
		const data_ref_span_t &raw_body() const;
		result_t<data_ref_span_t> decode_head(size_t max_size = SIZE_MAX) const;
		result_t<data_ref_span_t> decode_body(size_t max_size = SIZE_MAX) const;
	};

	struct chunk_entry_t : public basic_entry_t
	{
		error_t read(game_t &game, data_reader_t &strm);
		void view(source_explorer_t &srcexp) const;
	};

	struct item_entry_t : public basic_entry_t
	{
		error_t read(game_t &game,
		             data_reader_t &strm,
		             bool compressed,
		             size_t headersize = 0,
		             bool has_handle   = true);
		void view(source_explorer_t &srcexp) const;
	};

	struct basic_chunk_t
	{
		chunk_entry_t entry;

		error_t read(game_t &game, data_reader_t &strm);
		error_t basic_view(source_explorer_t &srcexp, const char *name) const;
	};

	struct basic_item_t
	{
		item_entry_t entry;

		error_t read(game_t &game, data_reader_t &strm);
		error_t basic_view(source_explorer_t &srcexp, const char *name) const;
	};

	struct string_chunk_t : public basic_chunk_t
	{
		mutable std::u16string value;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp,
		             const char *name,
		             const bool preview = false) const;

		lak::u16string u16string() const;
		lak::u8string u8string() const;
		lak::astring astring() const;
	};

	struct strings_chunk_t : public basic_chunk_t
	{
		mutable std::vector<std::u16string> values;

		error_t read(game_t &game, data_reader_t &strm);
		error_t basic_view(source_explorer_t &srcexp, const char *name) const;
		error_t view(source_explorer_t &srcexp) const;
	};

	struct compressed_chunk_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct vitalise_preview_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct menu_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct extension_path_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct extensions_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct global_events_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct extension_data_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct additional_extensions_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct application_doc_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct other_extension_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct global_values_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct global_strings_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct extension_list_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct icon_t : public basic_chunk_t
	{
		lak::image4_t bitmap;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};

	struct demo_version_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct security_number_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct binary_file_t
	{
		lak::u8string name;
		data_ref_span_t data;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};

	struct binary_files_t : public basic_chunk_t
	{
		lak::array<binary_file_t> items;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};

	struct menu_images_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct global_value_names_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct global_string_names_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct movement_extensions_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct object_bank2_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct exe_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct protection_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct shaders_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct extended_header_t : public basic_chunk_t
	{
		uint32_t flags;
		uint32_t build_type;
		uint32_t build_flags;
		uint16_t screen_ratio_tolerance;
		uint16_t screen_angle;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};

	struct spacer_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct chunk_224F_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct title2_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct chunk_2253_item_t
	{
		size_t position;

		uint16_t ID;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};

	struct chunk_2253_t : public basic_chunk_t
	{
		lak::array<chunk_2253_item_t> items;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};

	struct object_names_t : public strings_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct chunk_2255_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct two_five_plus_object_properties_t : public basic_chunk_t
	{
		lak::array<item_entry_t> items;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};

	struct chunk_2257_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct object_properties_t : public basic_chunk_t
	{
		lak::array<item_entry_t> items;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};

	struct truetype_fonts_meta_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	struct truetype_fonts_t : public basic_chunk_t
	{
		lak::array<item_entry_t> items;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};

	struct last_t : public basic_chunk_t
	{
		error_t view(source_explorer_t &srcexp) const;
	};

	namespace object
	{
		struct effect_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct shape_t
		{
			fill_type_t fill;
			shape_type_t shape;
			line_flags_t line;
			gradient_flags_t gradient;
			uint16_t border_size;
			lak::color4_t border_color;
			lak::color4_t color1, color2;
			uint16_t handle;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct quick_backdrop_t : public basic_chunk_t
		{
			uint32_t size;
			uint16_t obstacle;
			uint16_t collision;
			lak::vec2u32_t dimension;
			shape_t shape;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct backdrop_t : public basic_chunk_t
		{
			uint32_t size;
			uint16_t obstacle;
			uint16_t collision;
			lak::vec2u32_t dimension;
			uint16_t handle;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct animation_direction_t
		{
			lak::array<uint16_t> handles;
			uint8_t min_speed;
			uint8_t max_speed;
			uint16_t repeat;
			uint16_t back_to;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct animation_t
		{
			lak::array<uint16_t, 32> offsets;
			lak::array<animation_direction_t, 32> directions;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct animation_header_t
		{
			uint16_t size;
			lak::array<uint16_t> offsets;
			lak::array<animation_t> animations;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct common_t : public basic_chunk_t
		{
			uint32_t size;

			uint16_t movements_offset;
			uint16_t animations_offset;
			uint16_t counter_offset;
			uint16_t system_offset;
			uint32_t fade_in_offset;
			uint32_t fade_out_offset;
			uint16_t values_offset;
			uint16_t strings_offset;
			uint16_t extension_offset;

			std::unique_ptr<animation_header_t> animations;

			uint16_t version;
			uint32_t flags;
			uint32_t new_flags;
			uint32_t preferences;
			uint32_t identifier;
			lak::color4_t back_color;

			game_mode_t mode;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		// ObjectInfo + ObjectHeader
		struct item_t : public basic_chunk_t // OBJHEAD
		{
			std::unique_ptr<string_chunk_t> name;
			std::unique_ptr<effect_t> effect;
			std::unique_ptr<last_t> end;

			uint16_t handle;
			object_type_t type;
			uint32_t ink_effect;
			uint32_t ink_effect_param;

			std::unique_ptr<quick_backdrop_t> quick_backdrop;
			std::unique_ptr<backdrop_t> backdrop;
			std::unique_ptr<common_t> common;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;

			std::unordered_map<uint32_t, std::vector<std::u16string>> image_handles()
			  const;
		};

		// aka FrameItems
		struct bank_t : public basic_chunk_t
		{
			lak::array<item_t> items;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};
	}

	namespace frame
	{
		struct header_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct password_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct palette_t : public basic_chunk_t
		{
			uint32_t unknown;
			lak::array<lak::color4_t, 256> colors;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;

			lak::image4_t image() const;
		};

		struct object_instance_t
		{
			uint16_t handle;
			uint16_t info;
			lak::vec2i32_t position;
			object_parent_type_t parent_type;
			uint16_t parent_handle;
			uint16_t layer;
			uint16_t unknown;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct object_instances_t : public basic_chunk_t
		{
			lak::array<object_instance_t> objects;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct fade_in_frame_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct fade_out_frame_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct fade_in_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct fade_out_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct events_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct play_header_r : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct additional_item_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct additional_item_instance_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct layers_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct virtual_size_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct demo_file_path_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct random_seed_t : public basic_chunk_t
		{
			int16_t value;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct layer_effect_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct blueray_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct movement_time_base_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct mosaic_image_table_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct effects_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct iphone_options_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct chunk_334C_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct item_t : public basic_chunk_t
		{
			std::unique_ptr<string_chunk_t> name;
			std::unique_ptr<header_t> header;
			std::unique_ptr<password_t> password;
			std::unique_ptr<palette_t> palette;
			std::unique_ptr<object_instances_t> object_instances;
			std::unique_ptr<fade_in_frame_t> fade_in_frame;
			std::unique_ptr<fade_out_frame_t> fade_out_frame;
			std::unique_ptr<fade_in_t> fade_in;
			std::unique_ptr<fade_out_t> fade_out;
			std::unique_ptr<events_t> events;
			std::unique_ptr<play_header_r> play_head;
			std::unique_ptr<additional_item_t> additional_item;
			std::unique_ptr<additional_item_instance_t> additional_item_instance;
			std::unique_ptr<layers_t> layers;
			std::unique_ptr<virtual_size_t> virtual_size;
			std::unique_ptr<demo_file_path_t> demo_file_path;
			std::unique_ptr<random_seed_t> random_seed;
			std::unique_ptr<layer_effect_t> layer_effect;
			std::unique_ptr<blueray_t> blueray;
			std::unique_ptr<movement_time_base_t> movement_time_base;
			std::unique_ptr<mosaic_image_table_t> mosaic_image_table;
			std::unique_ptr<effects_t> effects;
			std::unique_ptr<iphone_options_t> iphone_options;
			std::unique_ptr<chunk_334C_t> chunk334C;
			std::unique_ptr<last_t> end;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct handles_t : public basic_chunk_t
		{
			lak::array<uint16_t> handles;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct bank_t : public basic_chunk_t
		{
			lak::array<item_t> items;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};
	}

	namespace image
	{
		struct item_t : public basic_item_t
		{
			uint32_t checksum; // uint16_t for old
			uint32_t reference;
			uint32_t data_size;
			lak::vec2u16_t size;
			graphics_mode_t graphics_mode; // uint8_t
			image_flag_t flags;            // uint8_t
			uint16_t unknown;              // not for old
			lak::vec2u16_t hotspot;
			lak::vec2u16_t action;
			lak::color4_t transparent; // not for old
			size_t data_position;

			size_t pad_to;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;

			result_t<data_ref_span_t> image_data() const;
			bool need_palette() const;
			result_t<lak::image4_t> image(
			  const bool color_transparent,
			  const lak::color4_t palette[256] = nullptr) const;
		};

		struct end_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct bank_t : public basic_chunk_t
		{
			lak::array<item_t> items;
			std::unique_ptr<end_t> end;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};
	}

	namespace font
	{
		struct item_t : public basic_item_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct end_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct bank_t : public basic_chunk_t
		{
			lak::array<item_t> items;
			std::unique_ptr<end_t> end;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};
	}

	namespace sound
	{
		struct item_t : public basic_item_t
		{
			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};

		struct end_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct bank_t : public basic_chunk_t
		{
			lak::array<item_t> items;
			std::unique_ptr<end_t> end;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};
	}

	namespace music
	{
		struct item_t : public basic_item_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct end_t : public basic_chunk_t
		{
			error_t view(source_explorer_t &srcexp) const;
		};

		struct bank_t : public basic_chunk_t
		{
			lak::array<item_t> items;
			std::unique_ptr<end_t> end;

			error_t read(game_t &game, data_reader_t &strm);
			error_t view(source_explorer_t &srcexp) const;
		};
	}

	template<typename T>
	struct chunk_ptr
	{
		using value_type = T;

		std::unique_ptr<T> ptr;

		auto &operator=(std::unique_ptr<T> &&p)
		{
			ptr = lak::move(p);
			return *this;
		}

		template<typename... ARGS>
		error_t view(ARGS &&...args) const
		{
			if (ptr)
			{
				RES_TRY(ptr->view(args...).MAP_SE_ERR("chunk_ptr::view"));
			}
			return lak::ok_t{};
		}

		operator bool() const { return static_cast<bool>(ptr); }

		auto *operator->() { return ptr.get(); }
		auto *operator->() const { return ptr.get(); }

		auto &operator*() { return *ptr; }
		auto &operator*() const { return *ptr; }
	};

	struct header_t : public basic_chunk_t
	{
		chunk_ptr<string_chunk_t> title;
		chunk_ptr<string_chunk_t> author;
		chunk_ptr<string_chunk_t> copyright;
		chunk_ptr<string_chunk_t> output_path;
		chunk_ptr<string_chunk_t> project_path;

		chunk_ptr<vitalise_preview_t> vitalise_preview;
		chunk_ptr<menu_t> menu;
		chunk_ptr<extension_path_t> extension_path;
		chunk_ptr<extensions_t> extensions; // deprecated
		chunk_ptr<extension_data_t> extension_data;
		chunk_ptr<additional_extensions_t> additional_extensions;
		chunk_ptr<application_doc_t> app_doc;
		chunk_ptr<other_extension_t> other_extension;
		chunk_ptr<extension_list_t> extension_list;
		chunk_ptr<icon_t> icon;
		chunk_ptr<demo_version_t> demo_version;
		chunk_ptr<security_number_t> security;
		chunk_ptr<binary_files_t> binary_files;
		chunk_ptr<menu_images_t> menu_images;
		chunk_ptr<string_chunk_t> about;
		chunk_ptr<movement_extensions_t> movement_extensions;
		chunk_ptr<object_bank2_t> object_bank_2;
		chunk_ptr<exe_t> exe;
		chunk_ptr<protection_t> protection;
		chunk_ptr<shaders_t> shaders;
		chunk_ptr<shaders_t> shaders2;
		chunk_ptr<extended_header_t> extended_header;
		chunk_ptr<spacer_t> spacer;
		chunk_ptr<chunk_224F_t> chunk224F;
		chunk_ptr<title2_t> title2;

		chunk_ptr<global_events_t> global_events;
		chunk_ptr<global_strings_t> global_strings;
		chunk_ptr<global_string_names_t> global_string_names;
		chunk_ptr<global_values_t> global_values;
		chunk_ptr<global_value_names_t> global_value_names;

		chunk_ptr<frame::handles_t> frame_handles;
		chunk_ptr<frame::bank_t> frame_bank;
		chunk_ptr<object::bank_t> object_bank;
		chunk_ptr<image::bank_t> image_bank;
		chunk_ptr<sound::bank_t> sound_bank;
		chunk_ptr<music::bank_t> music_bank;
		chunk_ptr<font::bank_t> font_bank;

		// Recompiled games (?):
		chunk_ptr<chunk_2253_t> chunk2253;
		chunk_ptr<object_names_t> object_names;
		chunk_ptr<chunk_2255_t> chunk2255;
		chunk_ptr<two_five_plus_object_properties_t>
		  two_five_plus_object_properties;
		chunk_ptr<chunk_2257_t> chunk2257;
		chunk_ptr<object_properties_t> object_properties;
		chunk_ptr<truetype_fonts_meta_t> truetype_fonts_meta;
		chunk_ptr<truetype_fonts_t> truetype_fonts;

		// Unknown chunks:
		lak::array<basic_chunk_t> unknown_chunks;
		lak::array<strings_chunk_t> unknown_strings;
		lak::array<compressed_chunk_t> unknown_compressed;

		chunk_ptr<last_t> last;

		error_t read(game_t &game, data_reader_t &strm);
		error_t view(source_explorer_t &srcexp) const;
	};

	struct game_t
	{
		static std::atomic<float> completed;
		static std::atomic<float> bank_completed;

		lak::astring game_path;
		lak::astring game_dir;

		data_ref_ptr_t file;

		lak::array<pack_file_t> pack_files;
		uint64_t data_pos;
		uint16_t num_header_sections;
		uint16_t num_sections;

		product_code_t runtime_version;
		uint16_t runtime_sub_version;
		uint32_t product_version;
		uint32_t product_build;

		std::stack<chunk_t> state;

		bool unicode            = false;
		bool old_game           = false;
		bool compat             = false;
		bool cnc                = false;
		bool recompiled         = false;
		bool two_five_plus_game = false;
		bool ccn                = false;
		lak::array<uint8_t> protection;

		header_t game;

		lak::u16string project;
		lak::u16string title;
		lak::u16string copyright;

		std::unordered_map<uint32_t, size_t> image_handles;
		std::unordered_map<uint16_t, size_t> object_handles;
	};

	struct file_state_t
	{
		fs::path path;
		bool valid;
		bool attempt;
	};

	struct source_explorer_t
	{
		lak::graphics_mode graphics_mode;

		game_t state;

		bool loaded                 = false;
		bool baby_mode              = true;
		bool dump_color_transparent = true;
		file_state_t exe;
		file_state_t images;
		file_state_t sorted_images;
		file_state_t sounds;
		file_state_t music;
		file_state_t shaders;
		file_state_t binary_files;
		file_state_t appicon;
		file_state_t error_log;
		file_state_t binary_block;

		MemoryEditor editor;

		const basic_entry_t *view = nullptr;
		texture_t image;
		data_ref_span_t buffer;
	};

	error_t LoadGame(source_explorer_t &srcexp);

	void GetEncryptionKey(game_t &game_state);

	error_t ParsePEHeader(data_reader_t &strm);

	error_t ParseGameHeader(data_reader_t &strm, game_t &game_state);

	result_t<size_t> ParsePackData(data_reader_t &strm, game_t &game_state);

	texture_t CreateTexture(const lak::image4_t &bitmap,
	                        const lak::graphics_mode mode);

	void ViewImage(source_explorer_t &srcexp, const float scale = 1.0f);

	const char *GetTypeString(const basic_entry_t &entry);

	const char *GetObjectTypeString(object_type_t type);

	const char *GetObjectParentTypeString(object_parent_type_t type);

	result_t<data_ref_span_t> Decode(data_ref_span_t encoded,
	                                 chunk_t ID,
	                                 encoding_t mode);

	result_t<data_ref_span_t> Inflate(data_ref_span_t compressed,
	                                  bool skip_header,
	                                  bool anaconda,
	                                  size_t max_size = SIZE_MAX);

	result_t<data_ref_span_t> LZ4Decode(data_ref_span_t compressed,
	                                    unsigned int out_size);

	result_t<data_ref_span_t> LZ4DecodeReadSize(data_ref_span_t compressed);

	result_t<data_ref_span_t> StreamDecompress(data_reader_t &strm,
	                                           unsigned int out_size);

	result_t<data_ref_span_t> Decrypt(data_ref_span_t encrypted,
	                                  chunk_t ID,
	                                  encoding_t mode);

	result_t<frame::item_t &> GetFrame(game_t &game, uint16_t handle);

	result_t<object::item_t &> GetObject(game_t &game, uint16_t handle);

	result_t<image::item_t &> GetImage(game_t &game, uint32_t handle);
}
#endif
