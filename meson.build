project(
  'source-explorer',
  ['c', 'cpp'],
  default_options: [
    'warning_level=3',
    # 'werror=true',
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
    '-DUNICODE',
    '-DWIN32_LEAN_AND_MEAN',
    '-DNOMINMAX',
    '-wd4315',
    '-wd4366',
  ]

  link_args = [
  ]

  if host_machine.cpu_family() == 'x86_64'
    libs = [
      'lib/x64/SDL2main.lib',
      'lib/x64/SDL2.lib',
    ]

    sdl = configure_file(
      copy: true,
      input: 'lib/x64/SDL2.dll',
      output: 'SDL2.dll',
    )
  elif host_machine.cpu_family() == 'x86'
    libs = [
      'lib/x86/SDL2main.lib',
      'lib/x86/SDL2.lib',
    ]

    sdl = configure_file(
      copy: true,
      input: 'lib/x86/SDL2.dll',
      output: 'SDL2.dll',
    )
  else
    error('Unsupported target ' + host_machine.cpu_family())
  endif

  custom_target(
    'generate_git_file',
    input: 'src/git.hpp',
    output: 'git.hpp',
    command: ['generate_git_file.bat', '@INPUT@'] ,
    build_by_default: true,
  )

  cplusplus = cxx.get_define(
    '__cplusplus',
    args: project_args + ['-std:' + version],
  )
else
  version = 'c++20'

  project_args = [
    '-Wfatal-errors',
  ]

  link_args = [
  ]

  libs = []

  cplusplus = cxx.get_define(
    '__cplusplus',
    args: project_args + ['-std=' + version],
  )
endif

if cxx.compute_int(cplusplus) < 202002
  error('C++ version insufficient, expected 202002L got ' + cplusplus)
endif

project_args += [
    '-D_CRT_SECURE_NO_WARNINGS',
    # '-DLAK_USE_WINAPI',
    # '-DLAK_USE_XLIB',
    # '-DLAK_USE_XCB',
    '-DLAK_USE_SDL',
    '-DSDL_MAIN_HANDLED',
]

add_project_arguments(project_args, language: ['c', 'cpp'])

subdir('include')
subdir('src')

executable(
  'srcexp',
  srcexp,
  objects: libs,
  link_args: link_args,
  override_options: 'cpp_std=' + version,
  include_directories: include_directories([
    'include',
    'include/glm',
    'include/imgui',
    'include/imgui/misc/cpp',
    'include/lak/inc',
    'include/lisk/inc',
    'include/SDL',
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
  ]
)
