GLyphy is a GPU text renderer that renders glyph outlines directly,
using the [Slug algorithm](https://jcgt.org/published/0006/02/02/)
by Eric Lengyel for robust winding number calculation.

GLyphy works with quadratic Bezier curves (TrueType outlines) and
produces pixel-perfect rendering at any scale with proper antialiasing.

Note: GLyphy is deprecated in favor of the
[HarfBuzz](https://github.com/harfbuzz/harfbuzz) GPU module, which
supports shaders for all major platforms.

## Building

Requires: meson, OpenGL 3.3+, FreeType, HarfBuzz, GLUT, GLEW.

```sh
meson setup build
meson compile -C build
build/demo/glyphy-demo
```

## How it works

Each glyph's outline is encoded into a compact blob stored in a GPU
buffer texture. The fragment shader computes a winding number by
casting horizontal and vertical rays against the quadratic Bezier
curves, using Lengyel's equivalence class algorithm for numerical
robustness. Curves are organized into bands for efficient early exit.

## License

GLyphy is licensed under the HarfBuzz Old MIT license.
See COPYING for details.
