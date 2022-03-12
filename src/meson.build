glyphy_sources = [
  'glyphy-arc.cc',
  'glyphy-arc-bezier.hh',
  'glyphy-arcs.cc',
  'glyphy-arcs-bezier.hh',
  'glyphy-blob.cc',
  'glyphy-common.hh',
  'glyphy-extents.cc',
  'glyphy-geometry.hh',
  'glyphy-outline.cc',
  'glyphy-sdf.cc',
  'glyphy-shaders.cc',
]

glyphy_headers = [
  'glyphy.h',
  'glyphy-freetype.h',
  'glyphy-harfbuzz.h',
]

glyphy_shaders = [
  [ 'glyphy-common.glsl', 'glyphy-common-glsl.h', 'glyphy_common_glsl' ],
  [ 'glyphy-sdf.glsl', 'glyphy-sdf-glsl.h', 'glyphy_sdf_glsl' ],
]

stringize = find_program('stringize')

glyphy_shader_sources = []

foreach p: glyphy_shaders
  glsl = p.get(0)
  header = p.get(1)
  decl = p.get(2)
  glyphy_shader_sources += custom_target('@0@ shader'.format(glsl),
    input: glsl,
    output: header,
    capture: true,
    command: [ stringize, 'static const char *' + decl, '@INPUT@' ])
endforeach

srcinc = include_directories('.')

cpp_args = ['-DHAVE_CONFIG_H=1']

libglyphy = library('glyphy', glyphy_sources + glyphy_headers + glyphy_shader_sources,
  include_directories: [confinc],
  cpp_args: cpp_args,
  version: '0.0.0',
  install: true)

install_headers(glyphy_headers, subdir: meson.project_name())
