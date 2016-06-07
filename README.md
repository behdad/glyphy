GLyphy is a signed-distance-field (SDF) text renderer using OpenGL ES2 shading language.

The main difference between GLyphy and other SDF-based OpenGL renderers is that most other projects sample the SDF into a texture. This has all the usual problems that sampling has. Ie. it distorts the outline and is low quality.

GLyphy instead represents the SDF using actual vectors submitted to the GPU. This results in very high quality rendering, though at a much higher runtime cost.

See this video for insight to how GLyphy works:

http://vimeo.com/behdad/glyphy

Dicussions happen on:

https://groups.google.com/forum/#!forum/glyphy


----------------------------------------------------------------------

This is continuation and cleanup of unfinished:

https://github.com/behdad/glyphy/pull/12

1. demo/windows is self-contained (not depending on external libraries - freeglut and glew included)
2. demo/windows complies w/o warnings using msvc2012/2013/2015
3. demo builds for both x86 and x64 platforms
4. jabberwocky text changed to a bit more useful "usage" instructions

----------------------------------------------------------------------

On GNOME3 and possibly other systems, if the vsync extension is not working (ie. pressing v in the demo doesn't have any effect), try running with `vblank_mode=0` env var.

Compilation instructions on Mac OS X:

1. Install Xcode and command line tools (as of Xcode 4.3.x, from
   within Preferences -> Downloads).
2. Install MacPorts.
3. `sudo port install automake autoconf libtool pkgconfig freetype`
4. `./autogen.sh`
5. `make`

Compilation instructions on Windows:

( originally developed by: https://github.com/tml1024/glyphy )

1. Install msvc2012/2013/2015 (comunity edition should be OK) if needed
2. Extact sed.exe from https://sourceforge.net/projects/unxutils/ and make sure it is on the PATH.
3. Open any of glyphy/demo/windows/msvc201x/glyphy.sln
4. Build all configurations and set demo as StartUp project.
5. Run and enjoy.

![alt tag](https://github.com/leok7v/glyphy/blob/master/demo/windows/glyphy.png)

