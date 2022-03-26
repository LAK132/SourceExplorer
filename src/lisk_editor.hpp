#ifndef LISK_EDITOR_HPP
#define LISK_EDITOR_HPP

#include "lisk_impl.hpp"

struct lisk_editor
{
	static lisk::string lisk_init_script;
	static lisk::string lisk_loop_script;
	static lisk::expression lisk_loop_expr;
	static lisk::string lisk_exception_message;
	static lisk::environment lisk_script_environment;
	static bool run_lisk_script;

	static void draw()
	{
		if (!run_lisk_script)
		{
			if (ImGui::Button("Run"))
			{
				lisk_script_environment = DefaultEnvironment();

				if (const auto result = lisk::root_eval_string(
				      lisk_init_script, lisk_script_environment);
				    result.is_exception())
				{
					lisk_exception_message = result.as_exception().message;
				}

				if (const auto tokens = lisk::root_tokenise(lisk_loop_script);
				    !tokens.empty())
				{
					auto loop = lisk::parse(tokens);
					if (loop.is_exception())
					{
						lisk_exception_message = loop.as_exception().message;
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

		lak::input_text(
		  "lisk-init-editor", &lisk_init_script, ImGuiInputTextFlags_Multiline);

		ImGui::Text("Loop:");

		lak::input_text(
		  "lisk-loop-editor", &lisk_loop_script, ImGuiInputTextFlags_Multiline);

		if (run_lisk_script)
		{
			auto result = lisk::eval(lisk_loop_expr, lisk_script_environment, true);
			if (result.is_exception())
			{
				run_lisk_script        = false;
				lisk_exception_message = result.as_exception().message;
			}
		}
	}
};

#endif
