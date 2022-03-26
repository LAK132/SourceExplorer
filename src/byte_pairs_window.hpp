#ifndef BYTE_PAIR_WINDOW_HPP
#define BYTE_PAIR_WINDOW_HPP

#include "base_window.hpp"

#include "dump.h"
#include "main.h"

struct byte_pairs_window : public base_window<byte_pairs_window>
{
	static void main_region()
	{
		auto load = [](se::file_state_t &file_state)
		{
			lak::debugger.clear();
			auto code = lak::open_file_modal(file_state.path, false);
			if (code.is_ok() && code.unwrap() == lak::file_open_error::VALID)
			{
				SrcExp.state      = se::game_t{};
				SrcExp.state.file = se::make_data_ref_ptr(
				  se::data_ref_ptr_t{},
				  lak::read_file(file_state.path).EXPECT("failed to load file"));
				ASSERT(SrcExp.state.file != nullptr);
				return true;
			}
			return code.is_err() ||
			       code.unwrap() != lak::file_open_error::INCOMPLETE;
		};

		auto manip = []
		{
			DEBUG("File size: ", SrcExp.state.file->size());
			SrcExp.loaded = true;
			return true;
		};

		if (!SrcExp.loaded || SrcExp.exe.attempt)
		{
			SrcExp.exe.attempt = true;
			se::Attempt(SrcExp.exe, load, manip);
			if (!SrcExp.loaded && !SrcExp.exe.attempt)
			{
				se_main_mode = se_main_mode_t::normal; // User cancelled
				return;
			}
		}

		if (SrcExp.loaded)
			base_window::byte_pairs_memory_explorer(
			  SrcExp.state.file->data(), SrcExp.state.file->size(), false);
	}
};

#endif
