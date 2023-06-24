stb = static_library(
  'stb_image',
  [
    'stb_image_write.c',
    'stb_image.c',
  ],
  override_options: 'warning_level=2',
)

subdir('lisk/src')

binex_subprj = subproject('binex')
binex_core_dep = binex_subprj.get_variable('binex_core_dep')
