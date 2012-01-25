/*
 * Copyright 2012 Google, Inc. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
