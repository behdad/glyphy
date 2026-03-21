GLyphy is a GPU text renderer that renders glyph outlines directly,
using the [Slug algorithm](https://jcgt.org/published/0006/02/02/)
by Eric Lengyel for robust winding number calculation.

GLyphy works with quadratic Bezier curves (TrueType outlines) and
produces pixel-perfect rendering at any scale with proper antialiasing.

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

The vertex shader performs dynamic dilation to ensure edge pixels
are always shaded for antialiasing.

Unlike Slug's reference vertex path, GLyphy computes dilation from
the projective Jacobian of NDC with respect to glyph-plane position
while using its existing quad vertex format, so the half-pixel
expansion remains perspective-aware.

## Notes

On GNOME3 and possibly other systems, if the vsync extension is not
working (pressing `v` in the demo has no effect), try running with
`vblank_mode=0`.

## License

GLyphy is licensed under the Apache License, Version 2.0.
