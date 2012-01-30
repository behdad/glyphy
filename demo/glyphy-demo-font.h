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

#ifndef GLYPHY_DEMO_FONT_H
#define GLYPHY_DEMO_FONT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glyphy-freetype.h>

#include "glyphy-demo-atlas.h"

#include <ext/hash_map>

using namespace __gnu_cxx; /* This is ridiculous */


typedef struct {
  glyphy_extents_t extents;
  double           advance;
  unsigned int     glyph_layout;
  unsigned int     atlas_x;
  unsigned int     atlas_y;
} glyph_info_t;

typedef hash_map<unsigned int, glyph_info_t> glyph_cache_t;


typedef struct {
  FT_Face        face;
  glyph_cache_t *glyph_cache;
  atlas_t       *atlas;
} glyphy_demo_font_t;

glyphy_demo_font_t *
glyphy_demo_font_create (FT_Face   face,
			 atlas_t  *atlas)
{
  glyphy_demo_font_t *font = (glyphy_demo_font_t *) malloc (sizeof (glyphy_demo_font_t));
  font->face = face;
  font->glyph_cache = new glyph_cache_t ();
  font->atlas = atlas;
  return font;
}


static glyphy_bool_t
accumulate_endpoint (glyphy_arc_endpoint_t         *endpoint,
		     vector<glyphy_arc_endpoint_t> *endpoints)
{
  endpoints->push_back (*endpoint);
  return true;
}

static void
encode_ft_glyph (FT_Face           face,
		 unsigned int      glyph_index,
		 double            tolerance_per_em,
		 glyphy_rgba_t    *buffer,
		 unsigned int      buffer_len,
		 unsigned int     *output_len,
		 unsigned int     *glyph_layout,
		 glyphy_extents_t *extents,
		 double           *advance)
{
  if (FT_Err_Ok != FT_Load_Glyph (face,
				  glyph_index,
				  FT_LOAD_NO_BITMAP |
				  FT_LOAD_NO_HINTING |
				  FT_LOAD_NO_AUTOHINT |
				  FT_LOAD_NO_SCALE |
				  FT_LOAD_LINEAR_DESIGN |
				  FT_LOAD_IGNORE_TRANSFORM))
    die ("Failed loading FreeType glyph");

  if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
    die ("FreeType loaded glyph format is not outline");

  unsigned int upem = face->units_per_EM;
  double tolerance = upem * tolerance_per_em; /* in font design units */
  double faraway = double (upem) / MIN_FONT_SIZE;
  vector<glyphy_arc_endpoint_t> endpoints;

  glyphy_arc_accumulator_t acc;
  glyphy_arc_accumulator_init (&acc, tolerance,
			       (glyphy_arc_endpoint_accumulator_callback_t) accumulate_endpoint,
			       &endpoints);

  if (FT_Err_Ok != glyphy_freetype(outline_decompose) (&face->glyph->outline, &acc))
    die ("Failed converting glyph outline to arcs");

  printf ("Used %u arc endpoints; Approx. err %g; Tolerance %g; Percentage %g. %s\n",
	  (unsigned int) acc.num_endpoints,
	  acc.max_error, tolerance,
	  round (100 * acc.max_error / acc.tolerance),
	  acc.max_error <= acc.tolerance ? "PASS" : "FAIL");

  double avg_fetch_achieved;

  if (!glyphy_arc_list_encode_rgba (&endpoints[0], endpoints.size (),
				    buffer,
				    buffer_len,
				    faraway,
				    4,
				    &avg_fetch_achieved,
				    output_len,
				    glyph_layout,
				    extents))
    die ("Failed encoding arcs");

  extents->min_x /= upem;
  extents->min_y /= upem;
  extents->max_x /= upem;
  extents->max_y /= upem;

  *advance = face->glyph->metrics.horiAdvance / (double) upem;

  printf ("Average %g texture accesses\n", avg_fetch_achieved);
}

static void
_glyphy_demo_font_upload_glyph (glyphy_demo_font_t *font,
				unsigned int glyph_index,
				glyph_info_t *glyph_info)
{
  glyphy_rgba_t buffer[4096];
  unsigned int output_len;

  encode_ft_glyph (font->face,
		   glyph_index,
		   TOLERANCE,
		   buffer, ARRAY_LEN (buffer),
		   &output_len,
		   &glyph_info->glyph_layout,
		   &glyph_info->extents,
		   &glyph_info->advance);

  printf ("Used %'lu bytes\n", output_len * sizeof (glyphy_rgba_t));

  atlas_alloc (font->atlas, buffer, output_len,
	       &glyph_info->atlas_x, &glyph_info->atlas_y);
}

void
glyphy_demo_font_lookup_glyph (glyphy_demo_font_t *font,
			       unsigned int glyph_index,
			       glyph_info_t *glyph_info)
{
  if (font->glyph_cache->find (glyph_index) == font->glyph_cache->end ()) {
    _glyphy_demo_font_upload_glyph (font, glyph_index, glyph_info);
    (*font->glyph_cache)[glyph_index] = *glyph_info;
  } else
    *glyph_info = (*font->glyph_cache)[glyph_index];
}

#endif /* GLYPHY_DEMO_FONT_H */
