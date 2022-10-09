stb = static_library(
  'stb_image',
  [
    'stb_image_write.c',
    'stb_image.c',
  ],
  override_options: 'warning_level=2',
)

subdir('lisk/src')

lak_subprj = subproject('lak')
lak = lak_subprj.get_variable('lak')
lak_dep = lak_subprj.get_variable('lak_dep')
lak_libs = lak_subprj.get_variable('system_libraries')
sdl2_dep = lak_subprj.get_variable('sdl2_dep')

glm_dep = subproject('glm').get_variable('glm_dep')
