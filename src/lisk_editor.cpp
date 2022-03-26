#include "lisk_editor.hpp"

lisk::string lisk_editor::lisk_init_script;
lisk::string lisk_editor::lisk_loop_script;
lisk::expression lisk_editor::lisk_loop_expr;
lisk::string lisk_editor::lisk_exception_message;
lisk::environment lisk_editor::lisk_script_environment;
bool lisk_editor::run_lisk_script = false;
