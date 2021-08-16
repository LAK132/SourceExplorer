#ifndef EXPLORER_ERROR_HPP
#define EXPLORER_ERROR_HPP

#include <lak/debug.hpp>
#include <lak/defer.hpp>
#include <lak/result.hpp>
#include <lak/stdint.hpp>
#include <lak/string.hpp>
#include <lak/trace.hpp>
#include <lak/tuple.hpp>
#include <lak/unicode.hpp>

namespace se
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
		} _value = str_err;

		std::vector<lak::pair<lak::trace, lak::astring>> _trace;

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

		template<typename... ARGS>
		error(lak::trace trace, lak::astring err) : _value(str_err)
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
			ASSERT(!_trace.empty());
			lak::u8string result;
			if (_value == str_err)
			{
				result = lak::streamify("\n",
				                        lak::scoped_indenter::str(),
				                        _trace[0].first,
				                        ": ",
				                        value_string());
			}
			else
			{
				result = lak::streamify("\n",
				                        lak::scoped_indenter::str(),
				                        _trace[0].first,
				                        ": ",
				                        value_string(),
				                        _trace[0].second.empty() ? "" : ": ",
				                        _trace[0].second);
			}
			++lak::debug_indent;
			DEFER(--lak::debug_indent;);
			for (const auto &[trace, str] : lak::span(_trace).subspan(1))
			{
				result += lak::streamify("\n",
				                         lak::scoped_indenter::str(),
				                         trace,
				                         str.empty() ? "" : ": ",
				                         str);
			}
			return result;
		}

		inline friend std::ostream &operator<<(std::ostream &strm,
		                                       const error &err)
		{
			return strm << lak::string_view(err.to_string());
		}
	};

#define MAP_TRACE(ERR, ...)                                                   \
	[&](const auto &err) -> se::error                                           \
	{ return se::error(LINE_TRACE, ERR, err, " " __VA_OPT__(, ) __VA_ARGS__); }

#define APPEND_TRACE(...)                                                     \
	[&](const se::error &err) -> se::error                                      \
	{ return err.append_trace(LINE_TRACE __VA_OPT__(, ) __VA_ARGS__); }

#define CHECK_REMAINING(STRM, EXPECTED)                                       \
	do                                                                          \
	{                                                                           \
		if (STRM.remaining() < (EXPECTED))                                        \
		{                                                                         \
			ERROR("Out Of Data: ",                                                  \
			      STRM.remaining(),                                                 \
			      " Bytes Remaining, Expected ",                                    \
			      (EXPECTED));                                                      \
			return lak::err_t{se::error(LINE_TRACE,                                 \
			                            se::error::out_of_data,                     \
			                            STRM.remaining(),                           \
			                            " Bytes Remaining, Expected ",              \
			                            (EXPECTED))};                               \
		}                                                                         \
	} while (false)

#define CHECK_POSITION(STRM, EXPECTED)                                        \
	do                                                                          \
	{                                                                           \
		if (STRM.size() < (EXPECTED))                                             \
		{                                                                         \
			ERROR("Out Of Data: ",                                                  \
			      STRM.remaining(),                                                 \
			      " Bytes Availible, Expected ",                                    \
			      (EXPECTED));                                                      \
			return lak::err_t{se::error(LINE_TRACE,                                 \
			                            se::error::out_of_data,                     \
			                            STRM.remaining(),                           \
			                            " Bytes Availible, Expected ",              \
			                            (EXPECTED))};                               \
		}                                                                         \
	} while (false)

	template<typename T>
	using result_t = lak::result<T, se::error>;
	using error_t  = se::result_t<lak::monostate>;
}

#endif