project(
  'source-explorer',
  ['c', 'cpp'],
  default_options: [
    'warning_level=3',
    'werror=true',
    'b_vscrt=static_from_buildtype',
    # 'b_sanitize=address'
  ],
)

cxx = meson.get_compiler('cpp')

if cxx.get_id() == 'msvc'
  version = 'c++latest'

  project_args = [
    '-Zc:__cplusplus',
    '-Zc:rvalueCast',
    '-Zc:wchar_t',
    '-Zc:ternary',
    '-Zc:preprocessor',
    '-DUNICODE',
    '-DWIN32_LEAN_AND_MEAN',
    '-DNOMINMAX',
    '-wd4315',
    '-wd4366',
    '-Wv:18',
  ]

  git_header = custom_target(
    'generate_git_file',
    output: 'git.hpp',
    command: ['generate_git_file.bat', '@OUTPUT@'] ,
    build_by_default: true,
  )

  macro_test_args = project_args + ['-std:' + version]
else
  version = 'c++2a'

  project_args = [
    '-Wfatal-errors',
    '-mavx',
  ]

  macro_test_args = project_args + ['-std=' + version]
endif

project_args += [
  '-D_CRT_SECURE_NO_WARNINGS',
  '-DSDL_MAIN_HANDLED',
]

if get_option('lak_backend') == 'sdl'
  project_args += [ '-DLAK_USE_SDL' ]
elif get_option('lak_backend') == 'win32'
  project_args += [ '-DLAK_USE_WINAPI' ]
endif

cplusplus = cxx.get_define('__cplusplus', args: macro_test_args)

if cxx.compute_int(cplusplus) < 202002
  error('C++ version insufficient, expected 202002L got ' + cplusplus)
endif

add_project_arguments(project_args, language: ['cpp'])

subdir('include')
subdir('src')

sdl2_dll = configure_file(
  copy: true,
  input: sdl2_dll,
  output: 'SDL2.dll',
)

executable(
  'srcexp',
  srcexp + [git_header],
  override_options: 'cpp_std=' + version,
  include_directories: include_directories([
    'include',
    'include/glm',
    'include/imgui',
    'include/imgui/misc/cpp',
    'include/lak/inc',
    'include/lisk/inc',
  ]),
  link_with: [
    lak,
    lakopengl,
    lakwindowing,
    lisk,
    stb,
    imgui,
  ],
  link_whole: [
    laktest,
  ],
  dependencies: [
    sdl2,
  ],
)
