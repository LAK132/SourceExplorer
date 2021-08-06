imgui = static_library(
  'dearimgui',
  [
    'imgui/imgui.cpp',
    'imgui/imgui_demo.cpp',
    'imgui/imgui_draw.cpp',
    'imgui/imgui_tables.cpp',
    'imgui/imgui_widgets.cpp',
    'imgui/examples/imgui_impl_softraster.cpp',
  ],
  override_options: 'cpp_std=' + version,
)

stb = static_library(
  'stb_image',
  [
    'stb_image_write.c',
    'stb_image.c',
  ],
  override_options: 'warning_level=2',
)

sdl_include_directory = include_directories('SDL')

subdir('lak/src')
subdir('lak/src/windowing')
subdir('lak/src/sqlite3')
subdir('lisk/src')
