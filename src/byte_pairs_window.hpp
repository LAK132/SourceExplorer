#ifndef BYTE_PAIRS_WINDOW_HPP
#define BYTE_PAIRS_WINDOW_HPP

#include "base_window.hpp"

#include "dump.h"
#include "main.h"

template<typename DERIVED>
struct byte_pairs_window
{
	bex::memory_byte_pairs_viewer viewer;

	void main_region(float)
	{
		if (SrcExp->exe.bad()) SrcExp->exe.make_attempt();

		if (SrcExp->exe.attempt)
		{
			srcexp::AttemptFile(
			  SrcExp->exe,
			  [](const fs::path &exe_path) -> lak::file_open_error
			  {
				  lak::debugger.clear();
				  SrcExp->state      = srcexp::game_t{};
				  SrcExp->state.file = srcexp::make_data_ref_ptr(
				    srcexp::data_ref_ptr_t{},
				    lak::read_file(exe_path).EXPECT("failed to load file"));
				  ASSERT(!!SrcExp->state.file);
				  DEBUG("File size: ", SrcExp->state.file->size());
				  SrcExp->loaded = true;
				  return lak::file_open_error::VALID;
			  },
			  false);

			if (SrcExp->exe.bad())
			{
				SrcExp->main_mode =
				  srcexp::instance_t::main_mode_t::normal; // User cancelled
				return;
			}
		}

		if (SrcExp->loaded) viewer.draw(*SrcExp->state.file, false);
	}
};

#endif
