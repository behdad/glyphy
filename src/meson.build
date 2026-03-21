glyphy_sources = [
  'glyphy-curves.cc',
  'glyphy-encode.cc',
  'glyphy-extents.cc',
  'glyphy-shaders.cc',
]

glyphy_headers = [
  'glyphy.h',
  'glyphy-freetype.h',
  'glyphy-harfbuzz.h',
]

glyphy_shaders = [
  [ 'glyphy-fragment.glsl', 'glyphy-fragment-glsl.h', 'glyphy_fragment_glsl' ],
  [ 'glyphy-vertex.glsl', 'glyphy-vertex-glsl.h', 'glyphy_vertex_glsl' ],
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

cpp_args = ['-DHAVE_CONFIG_H=1', '-DBUILDING_GLYPHY=1']

if get_option('default_library') == 'static'
  cpp_args += ['-DGLYPHY_STATIC']
endif

libglyphy = library('glyphy', glyphy_sources + glyphy_headers + glyphy_shader_sources,
  include_directories: [confinc],
  cpp_args: cpp_args,
  version: '0.0.0',
  install: true)

libglyphy_dep = declare_dependency(
  link_with: libglyphy,
  include_directories: [confinc])

meson.override_dependency('glyphy', libglyphy_dep)

libglyphy_pc = pkgmod.generate(libglyphy,
  description: 'High-quality GPU glyph rendering library',
  subdirs: meson.project_name())

install_headers(glyphy_headers, subdir: meson.project_name())
