#ifndef BYTE_PAIRS_WINDOW_HPP
#define BYTE_PAIRS_WINDOW_HPP

#include "base_window.hpp"

#include "dump.h"
#include "main.h"

struct byte_pairs_window : public base_window<byte_pairs_window>
{
	static void main_region()
	{
		if (SrcExp.exe.bad()) SrcExp.exe.make_attempt();

		if (SrcExp.exe.attempt)
		{
			se::AttemptFile(
			  SrcExp.exe,
			  [](const fs::path &exe_path) -> lak::file_open_error
			  {
				  lak::debugger.clear();
				  SrcExp.state      = se::game_t{};
				  SrcExp.state.file = se::make_data_ref_ptr(
				    se::data_ref_ptr_t{},
				    lak::read_file(exe_path).EXPECT("failed to load file"));
				  ASSERT(SrcExp.state.file != nullptr);
				  DEBUG("File size: ", SrcExp.state.file->size());
				  SrcExp.loaded = true;
				  return lak::file_open_error::VALID;
			  },
			  false);

			if (SrcExp.exe.bad())
			{
				se_main_mode = se_main_mode_t::normal; // User cancelled
				return;
			}
		}

		if (SrcExp.loaded)
			base_window::byte_pairs_memory_explorer(*SrcExp.state.file, false);
	}
};

#endif
