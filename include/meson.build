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

lmdb = static_library(
  'lmdb',
  [
    'lmdb/libraries/liblmdb/mdb.c',
    'lmdb/libraries/liblmdb/midl.c',
  ],
  override_options: [
    'warning_level=0',
    'werror=false',
  ],
)

stb = static_library(
  'stb_image',
  [
    'stb_image_write.c',
    'stb_image.c',
  ],
  override_options: 'warning_level=2',
)

lmdb_include_directory = include_directories('lmdb/libraries/liblmdb')

subdir('lak/inc/sdl')
subdir('lak/src')
subdir('lak/src/lmdb')
subdir('lak/src/opengl')
subdir('lak/src/windowing')
subdir('lak/src/tests')
subdir('lak/src/tests/windowing')
subdir('lak/src/sqlite3')
subdir('lisk/src')
