project(
	'source-explorer',
	['c', 'cpp'],
	default_options: [
		'b_vscrt=static_from_buildtype',
	],
)

binex_subprj = subproject('binex')
binex_core_dep = binex_subprj.get_variable('binex_core_dep')

c_cpp_args = binex_subprj.get_variable('c_cpp_args')
cpp_args = binex_subprj.get_variable('cpp_args')
c_args = binex_subprj.get_variable('c_args')

override_options = binex_subprj.get_variable('override_options')
override_options_w0 = binex_subprj.get_variable('override_options_w0')
override_options_w1 = binex_subprj.get_variable('override_options_w1')
override_options_w2 = binex_subprj.get_variable('override_options_w2')
override_options_w3 = binex_subprj.get_variable('override_options_w3')
override_options_werror = binex_subprj.get_variable('override_options_werror')

add_project_arguments(c_cpp_args + cpp_args, language: ['cpp'])
add_project_arguments(c_cpp_args + c_args, language: ['c'])

if host_machine.system() == 'windows'
	git_header = custom_target(
		'generate_git_file',
		output: 'git.hpp',
		command: ['generate_git_file.bat', '@OUTPUT@'] ,
		build_by_default: true,
	)
else
	git_header = custom_target(
		'generate_git_file',
		output: 'git.hpp',
		command: ['./generate_git_file.sh', '@OUTPUT@'] ,
		build_by_default: true,
	)
endif

install_directory = host_machine.cpu_family()

subdir('include')
subdir('src')

executable(
	'srcexp',
	srcexp + [git_header],
	install: true,
	install_dir: install_directory,
	override_options: override_options_w3,
	include_directories: include_directories([
		'include',
		'include/lisk/inc',
	]),
	link_with: [
		lisk,
	],
	dependencies: [
		binex_core_dep,
		stb_image_dep,
		stb_image_write_dep,
		imgui_color_text_edit_dep,
	],
)
