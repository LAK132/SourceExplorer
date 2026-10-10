#include "lisk_editor.hpp"

lak::lisk::string lisk_editor::lisk_init_script;
lak::lisk::string lisk_editor::lisk_loop_script;
lak::lisk::expression lisk_editor::lisk_loop_expr;
lak::lisk::string lisk_editor::lisk_exception_message;
lak::lisk::environment lisk_editor::lisk_script_environment;
bool lisk_editor::run_lisk_script = false;
