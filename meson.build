project('glyphy', 'c', 'cpp',
  meson_version: '>= 0.55.0',
  version: '1.0',
  default_options: [
    'cpp_rtti=false',
    'cpp_std=c++11',
    'wrap_mode=nofallback'
  ],
)

pkgmod = import('pkgconfig')
cpp = meson.get_compiler('cpp')

if cpp.get_id() == 'msvc'
  # Ignore several spurious warnings for things HarfBuzz does very commonly.
  # If a warning is completely useless and spammy, use '/wdXXXX' to suppress it
  # If a warning is harmless but hard to fix, use '/woXXXX' so it's shown once
  # NOTE: Only add warnings here if you are sure they're spurious
  msvc_args = [
    '/wd4018', # implicit signed/unsigned conversion
    '/wd4146', # unary minus on unsigned (beware INT_MIN)
    '/wd4244', # lossy type conversion (e.g. double -> int)
    '/wd4305', # truncating type conversion (e.g. double -> float)
    cpp.get_supported_arguments(['/utf-8']), # set the input encoding to utf-8
  ]
  add_project_arguments(msvc_args, language: ['c', 'cpp'])
  # Disable SAFESEH with MSVC for libs that use external deps that are built with MinGW
  # noseh_link_args = ['/SAFESEH:NO']
  # disable exception handling
  add_project_arguments(['/EHs-', '/EHc-'], language: 'cpp')
endif

add_project_link_arguments(cpp.get_supported_link_arguments([
  '-Bsymbolic-functions'
]), language: 'c')

add_project_arguments(cpp.get_supported_arguments([
  '-fno-exceptions',
  '-fno-rtti',
  '-fno-threadsafe-statics',
  '-fvisibility-inlines-hidden',
]), language: 'cpp')

glyphy_conf = configuration_data()

freetype_dep = dependency('freetype2', required: true)
harfbuzz_dep = dependency('harfbuzz', version: '>= 4.0.0', required: true)
gl_dep = dependency('gl', required: true)
glew_dep = dependency('glew', required: false)
glut_dep = dependency('glut', required: true)

pkg_data_dir = join_paths(get_option ('prefix'), 'share', meson.project_name())

if freetype_dep.found()
  glyphy_conf.set('HAVE_FREETYPE2', 1)
endif
if harfbuzz_dep.found()
  glyphy_conf.set('HAVE_HARFBUZZ', 1)
endif
if gl_dep.found()
  glyphy_conf.set('HAVE_GL', 1)
endif
if glew_dep.found()
  glyphy_conf.set('HAVE_GLEW', 1)
endif
if glut_dep.found()
  glyphy_conf.set('HAVE_GLUT',  1)
endif
glyphy_conf.set_quoted('PKGDATADIR', pkg_data_dir)

configure_file(output: 'config.h', configuration: glyphy_conf)
confinc = include_directories('.')

subdir('src')
subdir('demo')
