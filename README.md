GLyphy is a signed-distance-field (SDF) text renderer using OpenGL ES2 shading language.

The main difference between GLyphy and other SDF-based OpenGL renderers is that most other projects sample the SDF into a texture. This has all the usual problems that sampling has. Ie. it distorts the outline and is low quality.

GLyphy instead represents the SDF using actual vectors submitted to the GPU. This results in very high quality rendering, though at a much higher runtime cost.

See this video for insight to how GLyphy works:

http://vimeo.com/behdad/glyphy

Dicussions happen on:

https://groups.google.com/forum/#!forum/glyphy

----------------------------------------------------------------------

On GNOME3 and possibly other systems, if the vsync extension is not working (ie. pressing `v` in the demo doesn't have any effect), try running with `vblank_mode=0` env var.

### Compilation instructions on Mac OS X

1. Install Xcode and command line tools (as of Xcode 4.3.x, from
   within `Preferences` -> `Downloads`).
2. Install [MacPorts](https://www.macports.org/install.php).
3. `sudo port install automake autoconf libtool pkgconfig freetype`
4. `./autogen.sh`
5. `make`

### Compilation instructions on Windows

See [appveyor.yml](https://github.com/behdad/glyphy/blob/master/appveyor.yml), basically first get [vcpkg](https://github.com/Microsoft/vcpkg) and install `glew`, `freetype` and `freeglut` on it, then open win32\glyphy.sln
with Visual Studio.

### Compilation instructions for emscripten

Assuming you have installed emscripten and have its tools on path,

1. `NOCONFIGURE=1 ./autogen.sh`
2. `CPPFLAGS='-s USE_FREETYPE=1' LDFLAGS='-s USE_FREETYPE=1' emconfigure ./configure`
3. `make EXEEXT=.html GL_LIBS= GLUT_LIBS=`
4. The result will be located on `demo/.libs/glyphy-demo.html` (not `demo/glyphy-demo.html`)
