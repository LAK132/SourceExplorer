#ifndef SRCEXP_CTF_COMMON_HPP
#define SRCEXP_CTF_COMMON_HPP

#include "defines.hpp"
#include "encryption.hpp"

#include "../data_reader.hpp"
#include "../data_ref.hpp"

#include <lak/memory.hpp>
#include <lak/opengl/texture.hpp>
#include <lak/string.hpp>
#include <lak/trace.hpp>
#include <lak/variant.hpp>
#include <lak/vec.hpp>

#include <misc/softraster/texture.h>

#ifdef GetObject
#	undef GetObject
#endif

namespace fs = std::filesystem;

namespace SourceExplorer
{
	enum struct error_type : uint32_t
	{
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

	inline std::ostream &to_string(std::ostream &strm, error_type err)
	{
		switch (err)
		{
			case error_type::invalid_exe_signature:
				return strm << "Invalid EXE Signature";
			case error_type::invalid_pe_signature:
				return strm << "Invalid PE Signature";
			case error_type::invalid_game_signature:
				return strm << "Invalid Game Header";
			case error_type::invalid_state:
				return strm << "Invalid State";
			case error_type::invalid_mode:
				return strm << "Invalid Mode";
			case error_type::invalid_chunk:
				return strm << "Invalid Chunk";
			case error_type::no_mode0:
				return strm << "No MODE0";
			case error_type::no_mode1:
				return strm << "No MODE1";
			case error_type::no_mode2:
				return strm << "No MODE2";
			case error_type::no_mode3:
				return strm << "No MODE3";
			case error_type::out_of_data:
				return strm << "Out Of Data";
			case error_type::inflate_failed:
				return strm << "Inflate Failed";
			case error_type::decrypt_failed:
				return strm << "Decrypt Failed";
			case error_type::no_mode0_decoder:
				return strm << "No MODE0 Decoder";
			case error_type::no_mode1_decoder:
				return strm << "No MODE1 Decoder";
			case error_type::no_mode2_decoder:
				return strm << "No MODE2 Decoder";
			case error_type::no_mode3_decoder:
				return strm << "No MODE3 Decoder";
			default:
				return strm << "Invalid Error Code";
		}
	}

	inline lak::stack_trace error(error_type err, lak::trace trace = {})
	{
		return lak::stack_trace{}
		  .set_message(lak::streamify(err))
		  .add(lak::move(trace));
	}

	inline lak::stack_trace error(lak::u8string err, lak::trace trace = {})
	{
		return lak::stack_trace{}.set_message(err).add(lak::move(trace));
	}

	inline lak::stack_trace error(error_type err,
	                              lak::u8string msg,
	                              lak::trace trace = {})
	{
		return lak::stack_trace{}
		  .set_message(lak::streamify(err, ": ", msg))
		  .add(lak::move(trace));
	}

#define TRY_ASSIGN(A, ...) RES_TRY_TO_TRACE_ASSIGN(A, __VA_ARGS__)

#define TRY(...) RES_TRY_TO_TRACE(__VA_ARGS__)

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
			  SourceExplorer::error(SourceExplorer::error_type::out_of_data,        \
			                        lak::streamify(STRM.remaining().size(),         \
			                                       " Bytes Remaining, Expected ",   \
			                                       expected))};                     \
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
			  SourceExplorer::error(SourceExplorer::error_type::out_of_data,        \
			                        lak::streamify(STRM.remaining().size(),         \
			                                       " Bytes Availible, Expected ",   \
			                                       expected))};                     \
		}                                                                         \
	} while (false)

	template<typename T>
	using result_t = lak::result<T, lak::stack_trace>;
	using error_t  = result_t<lak::monostate>;

	extern bool force_compat;
	extern bool skip_broken_items;
	extern bool open_broken_games;
	extern lak::array<uint8_t> _magic_key;
	extern uint8_t _magic_char;

	template<typename T>
	struct chunk_ptr
	{
		using value_type = T;

		lak::unique_ptr<T> ptr;

		auto &operator=(lak::unique_ptr<T> &&p)
		{
			ptr = lak::move(p);
			return *this;
		}

		template<typename... ARGS>
		error_t view(ARGS &&...args) const
		{
			if (ptr)
			{
				RES_TRY(ptr->view(args...).RES_ADD_TRACE("chunk_ptr::view"));
			}
			return lak::ok_t{};
		}

		operator bool() const { return static_cast<bool>(ptr); }

		auto *operator->() { return ptr.get(); }
		auto *operator->() const { return ptr.get(); }

		auto &operator*() { return *ptr; }
		auto &operator*() const { return *ptr; }
	};

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
	  lak::variant<lak::monostate, lak::opengl::texture, texture_color32_t>;

	struct pack_file_t
	{
		lak::u16string filename;
		lak::array<byte_t> data;
		bool wide;
		uint32_t bingo;
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

	struct game_t;
	struct source_explorer_t;
}

#endif
