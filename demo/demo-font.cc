/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "demo-font.h"

#include <glyphy-harfbuzz.h>

#include <map>
#include <vector>

typedef std::map<unsigned int, glyph_info_t> glyph_cache_t;

struct demo_font_t {
  unsigned int   refcount;

  hb_face_t     *face;
  hb_font_t     *font;
  glyph_cache_t *glyph_cache;
  demo_atlas_t  *atlas;
  glyphy_curve_accumulator_t *acc;

  unsigned int num_glyphs;
  unsigned int sum_curves;
  unsigned int sum_bytes;
};

demo_font_t *
demo_font_create (hb_face_t    *face,
		  demo_atlas_t *atlas)
{
  demo_font_t *font = (demo_font_t *) calloc (1, sizeof (demo_font_t));
  font->refcount = 1;

  font->face = hb_face_reference (face);
  font->font = hb_font_create (face);
  font->glyph_cache = new glyph_cache_t ();
  font->atlas = demo_atlas_reference (atlas);
  font->acc = glyphy_curve_accumulator_create ();

  return font;
}

demo_font_t *
demo_font_reference (demo_font_t *font)
{
  if (font) font->refcount++;
  return font;
}

void
demo_font_destroy (demo_font_t *font)
{
  if (!font || --font->refcount)
    return;

  glyphy_curve_accumulator_destroy (font->acc);
  demo_atlas_destroy (font->atlas);
  delete font->glyph_cache;
  hb_font_destroy (font->font);
  hb_face_destroy (font->face);
  free (font);
}


hb_face_t *
demo_font_get_face (demo_font_t *font)
{
  return font->face;
}

hb_font_t *
demo_font_get_font (demo_font_t *font)
{
  return font->font;
}

demo_atlas_t *
demo_font_get_atlas (demo_font_t *font)
{
  return font->atlas;
}


static glyphy_bool_t
accumulate_curve (const glyphy_curve_t           *curve,
		  std::vector<glyphy_curve_t>    *curves)
{
  curves->push_back (*curve);
  return true;
}

static void
encode_glyph (demo_font_t      *font,
	      unsigned int      glyph_index,
	      glyphy_texel_t   *buffer,
	      unsigned int      buffer_len,
	      unsigned int     *output_len,
	      glyphy_extents_t *extents,
	      double           *advance)
{
  std::vector<glyphy_curve_t> curves;

  glyphy_curve_accumulator_reset (font->acc);
  glyphy_curve_accumulator_set_callback (font->acc,
					 (glyphy_curve_accumulator_callback_t) accumulate_curve,
					 &curves);

  glyphy_harfbuzz(font_get_glyph_shape) (font->font, glyph_index, font->acc);
  if (!glyphy_curve_accumulator_successful (font->acc))
    die ("Failed accumulating curves");

  if (!glyphy_curve_list_encode_blob (curves.size () ? &curves[0] : NULL, curves.size (),
				      buffer, buffer_len,
				      output_len,
				      extents))
    die ("Failed encoding blob");

  unsigned int upem = hb_face_get_upem (font->face);
  glyphy_extents_scale (extents, 1. / upem, 1. / upem);

  *advance = hb_font_get_glyph_h_advance (font->font, glyph_index) / (double) upem;

  font->num_glyphs++;
  font->sum_curves += glyphy_curve_accumulator_get_num_curves (font->acc);
  font->sum_bytes += (*output_len * sizeof (glyphy_texel_t));
}

static void
_demo_font_upload_glyph (demo_font_t *font,
			 unsigned int glyph_index,
			 glyph_info_t *glyph_info)
{
  glyphy_texel_t buffer[16384];
  unsigned int output_len;

  encode_glyph (font,
		glyph_index,
		buffer, ARRAY_LEN (buffer),
		&output_len,
		&glyph_info->extents,
		&glyph_info->advance);

  glyph_info->upem = hb_face_get_upem (font->face);
  glyph_info->is_empty = glyphy_extents_is_empty (&glyph_info->extents);
  if (!glyph_info->is_empty)
    glyph_info->atlas_offset = demo_atlas_alloc (font->atlas, buffer, output_len);
}

void
demo_font_lookup_glyph (demo_font_t  *font,
			unsigned int  glyph_index,
			glyph_info_t *glyph_info)
{
  if (font->glyph_cache->find (glyph_index) == font->glyph_cache->end ()) {
    _demo_font_upload_glyph (font, glyph_index, glyph_info);
    (*font->glyph_cache)[glyph_index] = *glyph_info;
  } else
    *glyph_info = (*font->glyph_cache)[glyph_index];
}

void
demo_font_print_stats (demo_font_t *font)
{
  LOGI ("%3d glyphs; avg curves%6.2f; avg %5.2fkb per glyph\n",
	font->num_glyphs,
	(double) font->sum_curves / font->num_glyphs,
	font->sum_bytes / 1024. / font->num_glyphs);
}
