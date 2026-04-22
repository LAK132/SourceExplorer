#ifndef BINARY_ANALYSIS_WINDOW_HPP
#define BINARY_ANALYSIS_WINDOW_HPP

#include "base_window.hpp"

#include "dump.h"
#include "main.h"

void ImGui::ShowDemoWindow(bool *p_open);

template<typename DERIVED>
struct binary_analysis_window
{
	using memory_view = typename base_window<DERIVED>::memory_view;

	bool force_update_memory;
	srcexp::data_ref_span_t view_data;
	bool demo_window = false;
	memory_view view;
	MemoryEditor editor;
	bex::memory_viewer viewer;

	void file_menu()
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("Open...", nullptr))
			{
				DEBUG("Open");
				SrcExp->exe.make_attempt();
			}

			ImGui::Checkbox("Demo Window", &demo_window);

			ImGui::EndMenu();
		}
	}

	void menu_bar(float)
	{
		file_menu();
		static_cast<DERIVED *>(this)->mode_select_menu();
		static_cast<DERIVED *>(this)->debug_menu();
	}

	void main_region(float frame_time)
	{
		if (SrcExp->exe.attempt)
		{
			srcexp::AttemptFile(
			  SrcExp->exe,
			  [&](const fs::path &exe_path) -> lak::file_open_error
			  {
				  lak::debugger.clear();
				  SrcExp->state      = srcexp::game_t{};
				  SrcExp->state.file = srcexp::make_data_ref_ptr(
				    srcexp::data_ref_ptr_t{},
				    lak::read_file(exe_path).EXPECT("failed to load file"));
				  ASSERT(!!SrcExp->state.file);
				  DEBUG("File size: ", SrcExp->state.file->size());
				  SrcExp->loaded      = true;
				  force_update_memory = true;
				  return lak::file_open_error::VALID;
			  },
			  false);
		}

		if (SrcExp->loaded)
		{
			static_cast<DERIVED *>(this)->main_region(frame_time);

			if (force_update_memory) force_update_memory = false;
		}
		else
		{
			if (demo_window) ImGui::ShowDemoWindow(&demo_window);
		}
	}

	void left_region(float)
	{
		force_update_memory |=
		  view.draw(SrcExp->state.file, SrcExp->buffer, force_update_memory);

		editor.DrawContents(reinterpret_cast<uint8_t *>(SrcExp->buffer.data()),
		                    SrcExp->buffer.size());
	}

	void right_region(float)
	{
		viewer.draw(SrcExp->buffer, force_update_memory);
	}
};

#endif
