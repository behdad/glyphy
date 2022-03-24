project('glyphy', 'c', 'cpp',
  meson_version: '>= 0.55.0',
  version: '0.2.0',
  default_options: [
    'cpp_rtti=false',
    'cpp_std=c++11',
    'wrap_mode=nofallback'
  ],
)

pkgmod = import('pkgconfig')
cpp = meson.get_compiler('cpp')

is_msvc = cpp.get_id() == 'msvc'
is_msvc_like = is_msvc or cpp.get_argument_syntax() == 'msvc'

if is_msvc
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
else
  add_project_link_arguments(cpp.get_supported_link_arguments([
    '-Bsymbolic-functions'
  ]), language: 'c')

  add_project_arguments(cpp.get_supported_arguments([
    '-fno-exceptions',
    '-fno-rtti',
    '-fno-threadsafe-statics',
    '-fvisibility-inlines-hidden',
  ]), language: 'cpp')
endif

if is_msvc_like
  add_project_arguments(cpp.get_supported_arguments([
    '-D_USE_MATH_DEFINES', # For using _M_SQRT* etc with MSVC headers
  ]), language: ['c', 'cpp'])
endif

glyphy_conf = configuration_data()

freetype_dep = dependency(is_msvc_like ? 'freetype': 'freetype2', required: true)
harfbuzz_dep = dependency('harfbuzz', version: '>= 4.0.0', required: true)
gl_dep = dependency('gl', required: true)
glew_dep = dependency('glew', required: false)
glut_dep = dependency('glut', required: true)

prefix = get_option('prefix')
libdir = join_paths(prefix, get_option('libdir'))
includedir = join_paths(prefix, get_option('includedir'))
datadir = join_paths(prefix, get_option('datadir'))
pkgdatadir = join_paths(datadir, meson.project_name())

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
glyphy_conf.set_quoted('PKGDATADIR', pkgdatadir)

configure_file(output: 'config.h', configuration: glyphy_conf)
confinc = include_directories('.')

subdir('src')

if get_option('demo').enabled()
  subdir('demo')
endif

summary('Demo', get_option('demo').enabled(), section: 'Features')
summary('Freetype', freetype_dep.found(), section: 'Features')
summary('Harfbuzz', harfbuzz_dep.found(), section: 'Features')

summary('Compiler', cpp.get_id(), section: 'Toolchain')
summary('Linker', cpp.get_linker_id(), section: 'Toolchain')

summary('Debugging', get_option('debug'), section: 'Build')
summary('Optimization', get_option('optimization'), section: 'Build')

summary('prefix', prefix, section: 'Directories')
summary('includedir', includedir, section: 'Directories')
summary('libdir', libdir, section: 'Directories')
summary('datadir', datadir, section: 'Directories')

