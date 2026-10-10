#ifndef LISK_EDITOR_HPP
#define LISK_EDITOR_HPP

#include "lisk_impl.hpp"

struct lisk_editor
{
	static lak::lisk::string lisk_init_script;
	static lak::lisk::string lisk_loop_script;
	static lak::lisk::expression lisk_loop_expr;
	static lak::lisk::string lisk_exception_message;
	static lak::lisk::environment lisk_script_environment;
	static bool run_lisk_script;

	static void draw()
	{
		if (!run_lisk_script)
		{
			if (ImGui::Button("Run"))
			{
				lisk_script_environment = DefaultEnvironment();

				if (const auto result = lak::lisk::root_eval_string(
				      lisk_init_script, lisk_script_environment);
				    result.is_exception())
				{
					lisk_exception_message = result.get_exception().unwrap().message;
				}

				if (const auto tokens = lak::lisk::root_tokenise(lisk_loop_script);
				    !tokens.empty())
				{
					auto loop = lak::lisk::parse(tokens);
					if (loop.is_exception())
					{
						lisk_exception_message = loop.get_exception().unwrap().message;
					}
					else if (loop.is_list())
					{
						run_lisk_script = true;
						lisk_loop_expr  = loop;
					}
				}
			}
		}
		else
		{
			if (ImGui::Button("Stop"))
			{
				run_lisk_script = false;
			}
		}

		if (!lisk_exception_message.empty())
			ImGui::Text("Exception thrown: %s", lisk_exception_message.c_str());

		ImGui::Text("Init:");

		lak::input_text("lisk-init-editor",
		                static_cast<lak::u8string *>(&lisk_init_script),
		                ImGuiInputTextFlags_Multiline);

		ImGui::Text("Loop:");

		lak::input_text("lisk-loop-editor",
		                static_cast<lak::u8string *>(&lisk_loop_script),
		                ImGuiInputTextFlags_Multiline);

		if (run_lisk_script)
		{
			auto result =
			  lak::lisk::eval(lisk_loop_expr, lisk_script_environment, true);
			if (result.is_exception())
			{
				run_lisk_script        = false;
				lisk_exception_message = result.get_exception().unwrap().message;
			}
		}
	}
};

#endif
