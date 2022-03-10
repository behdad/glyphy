demo_sources = [
  'demo-atlas.h',
  'demo-atlas.cc',
  'demo-buffer.h',
  'demo-buffer.cc',
  'demo-common.h',
  'demo-font.h',
  'demo-font.cc',
  'demo-glstate.h',
  'demo-glstate.cc',
  'demo-shader.h',
  'demo-shader.cc',
  'demo-view.h',
  'demo-view.cc',
  'glyphy-demo.cc',
  'matrix4x4.h',
  'matrix4x4.c',
  'trackball.h',
  'trackball.c',
]

demo_shaders = [
  [ 'demo-atlas.glsl', 'demo-atlas-glsl.h', 'demo_atlas_glsl' ],
  [ 'demo-fshader.glsl', 'demo-fshader-glsl.h', 'demo_fshader_glsl' ],
  [ 'demo-vshader.glsl', 'demo-vshader-glsl.h', 'demo_vshader_glsl' ],
  [ 'default-text.txt', 'default-text.h', 'default_text' ],
] 

demo_shader_sources = []

foreach p: demo_shaders
  glsl = p.get(0)
  header = p.get(1)
  decl = p.get(2)
  demo_shader_sources += custom_target('@0@ shader'.format(glsl),
    input: glsl,
    output: header,
    capture: true,
    command: [ stringize, 'static const char *' + decl, '@INPUT@' ])
endforeach

hexify = find_program('hexify')

demo_shader_sources += custom_target('default font',
  input: 'default-font.ttf',
  output: 'default-font.h',
  capture: true,
  command: [ hexify, 'static const char default_font[]', '@INPUT@' ])

glyphy_demo = executable('glyphy-demo', demo_sources + demo_shader_sources,
  cpp_args: [],
  include_directories: [confinc, srcinc],
  dependencies: [freetype_dep, harfbuzz_dep, gl_dep, glew_dep, glut_dep],
  link_with: [libglyphy],
  install: true)
