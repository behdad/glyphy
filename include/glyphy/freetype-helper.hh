/*
 * Copyright © 2011  Google, Inc.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Behdad Esfahbod, Maysum Panju
 */

#include <glyphy/geometry.hh>

#include <algorithm>
#include <vector>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#ifndef FREETYPE_HELPER_HH
#define FREETYPE_HELPER_HH

namespace GLyphy {

namespace FreeTypeHelper {

using namespace Geometry;

template <typename OutlineSink>
class FreeTypeOutlineSource
{
  public:

  static bool decompose_outline (FT_Outline *outline, OutlineSink &d)
  {
    static const FT_Outline_Funcs outline_funcs = {
        (FT_Outline_MoveToFunc) move_to,
        (FT_Outline_LineToFunc) line_to,
        (FT_Outline_ConicToFunc) conic_to,
        (FT_Outline_CubicToFunc) cubic_to,
        0, /* shift */
        0, /* delta */
    };

    return !FT_Outline_Decompose (const_cast <FT_Outline *> (outline), &outline_funcs, &d);
  }

  static const Point<Coord> point (FT_Vector *to)
  {
    return Point<Coord> (to->x, to->y);
  }

  static FT_Error err (bool success)
  {
    return success ? FT_Err_Ok : FT_Err_Out_Of_Memory;
  }

  static int
  move_to (FT_Vector *to, OutlineSink *d)
  {
    return err (d->move_to (point (to)));
  }

  static int
  line_to (FT_Vector *to, OutlineSink *d)
  {
    return err (d->line_to (point (to)));
  }

  static int
  conic_to (FT_Vector *control, FT_Vector *to, OutlineSink *d)
  {
    return err (d->conic_to (point (control), point (to)));
  }

  static int
  cubic_to (FT_Vector *control1, FT_Vector *control2,
	    FT_Vector *to, OutlineSink *d)
  {
    return err (d->cubic_to (point (control1), point (control2), point (to)));
  }
};

FT_Outline *
ft_face_to_outline (FT_Face face, unsigned int glyph_index);

void
ft_outline_to_arcs (FT_Outline *outline, double tolerance,
		    std::vector<arc_t> &arcs, double &error);

int
ft_outline_to_texture (FT_Outline *outline, unsigned int upem, int width,
		       int *height, void **buffer);

int
ft_face_to_texture (FT_Face face, FT_ULong uni, int width, int *height,
		    void **buffer);

} /* namespace FreeTypeHelper */

} /* namespace GLyphy */
#endif
