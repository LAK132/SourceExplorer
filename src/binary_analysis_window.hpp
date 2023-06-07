#ifndef BINARY_ANALYSIS_WINDOW_HPP
#define BINARY_ANALYSIS_WINDOW_HPP

#include "base_window.hpp"

#include "dump.h"
#include "main.h"

struct binary_analysis_window : public base_window<binary_analysis_window>
{
	inline static bool force_update_memory;
	inline static se::data_ref_span_t view_data;

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
				  SrcExp.loaded       = true;
				  force_update_memory = true;
				  return lak::file_open_error::VALID;
			  },
			  false);
		}

		if (SrcExp.loaded)
		{
			base_window::main_region();

			if (force_update_memory) force_update_memory = false;
		}
	}

	static void left_region()
	{
		static base_window::memory_view view;
		force_update_memory |=
		  view.draw(SrcExp.state.file, SrcExp.buffer, force_update_memory);

		static MemoryEditor editor;
		editor.DrawContents(reinterpret_cast<uint8_t *>(SrcExp.buffer.data()),
		                    SrcExp.buffer.size());
	}

	static void right_region()
	{
		static MemoryEditor editor;
		static base_window::memory_explorer_content_mode content_mode;
		base_window::memory_explorer_impl(
		  editor, content_mode, SrcExp.buffer, force_update_memory);
	}
};

#endif
