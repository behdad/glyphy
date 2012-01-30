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

#ifndef DEMO_FONT_H
#define DEMO_FONT_H

#include "demo-common.h"
#include "demo-atlas.h"

#include <ft2build.h>
#include FT_FREETYPE_H


typedef struct {
  glyphy_extents_t extents;
  double           advance;
  unsigned int     glyph_layout;
  unsigned int     atlas_x;
  unsigned int     atlas_y;
} glyph_info_t;


typedef struct demo_font_t demo_font_t;

demo_font_t *
demo_font_create (FT_Face       face,
		  demo_atlas_t *atlas);

demo_font_t *
demo_font_reference (demo_font_t *font);

void
demo_font_destroy (demo_font_t *font);


void
demo_font_lookup_glyph (demo_font_t  *font,
			unsigned int  glyph_index,
			glyph_info_t *glyph_info);


#endif /* DEMO_FONT_H */
