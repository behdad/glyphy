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
 * Google Author(s): Behdad Esfahbod, Maysum Panju, Wojciech Baranowski
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glyphy.h>
#include <glyphy-freetype.h>

#include "glyphy-demo-glut.h"
#include "glyphy-demo-shaders.h"
#include "glyphy-demo-font.h"

#include <assert.h>

#include <vector>

using namespace std;



#if 1
// Large font size profile
#define MIN_FONT_SIZE 64
#define TOLERANCE 5e-4
#else
// Small font size profile
#define MIN_FONT_SIZE 20
#define TOLERANCE 1e-3
#endif



static void
die (const char *msg)
{
  fprintf (stderr, "%s\n", msg);
  exit (1);
}



static glyphy_bool_t
accumulate_endpoint (glyphy_arc_endpoint_t         *endpoint,
		     vector<glyphy_arc_endpoint_t> *endpoints)
{
  endpoints->push_back (*endpoint);
  return true;
}

static void
encode_ft_glyph (FT_Face face, unsigned int glyph_index,
		 double tolerance_per_em,
		 glyphy_rgba_t *buffer, unsigned int buffer_len,
		 unsigned int *output_len,
		 unsigned int *glyph_layout,
		 glyphy_extents_t *extents)
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

  printf ("Average %g texture accesses\n", avg_fetch_achieved);
}

static FT_Face
open_ft_face (const char   *font_path,
	      unsigned int  face_index)
{
  static FT_Library library;
  if (!library)
    FT_Init_FreeType (&library);
  FT_Face face;
  FT_New_Face (library, font_path, face_index, &face);
  return face;
}

static void
upload_glyph (atlas_t *atlas,
	      FT_Face face,
	      unsigned int glyph_index,
	      glyph_info_t *glyph_info)
{
  glyphy_rgba_t buffer[10000];
  unsigned int output_len;

  encode_ft_glyph (face,
		   glyph_index,
		   TOLERANCE,
		   buffer, ARRAY_LEN (buffer),
		   &output_len,
		   &glyph_info->glyph_layout,
		   &glyph_info->extents);

  printf ("Used %'lu bytes\n", output_len * sizeof (glyphy_rgba_t));

  atlas_alloc (atlas, buffer, output_len, &glyph_info->atlas_x, &glyph_info->atlas_y);
}

static void
get_glyph_info (glyph_cache_t *glyph_cache,
		atlas_t *atlas,
		FT_Face face,
		unsigned int glyph_index,
		glyph_info_t *glyph_info)
{
  if (glyph_cache->find (glyph_index) == glyph_cache->end ()) {
    upload_glyph (atlas, face, glyph_index, glyph_info);
    (*glyph_cache)[glyph_index] = *glyph_info;
  } else
    *glyph_info = (*glyph_cache)[glyph_index];
}

int
main (int argc, char** argv)
{
  char *font_path = NULL;
  unsigned int unicode = 0;
  if (argc >= 3) {
     font_path = argv[1];
     unicode = argv[2][0];
     if (argc >= 4)
       animate = atoi (argv[3]);
  }
  else
    die ("Usage: grid PATH_TO_FONT_FILE CHARACTER_TO_DRAW ANIMATE?");

  glut_init (&argc, argv);

  FT_Face face = open_ft_face (font_path, 0);
  /* TODO handle error */

  GLuint program = create_program ();

  GLuint a_glyph_vertex_loc = glGetAttribLocation (program, "a_glyph_vertex");
  glEnableVertexAttribArray (a_glyph_vertex_loc);

  atlas_t atlas;
  atlas_init (&atlas, 512, 512, 32, 4);

  glyph_cache_t glyph_cache;

  vector<glyph_vertex_t> vertices;

  unsigned int glyph_index;
  glyph_info_t gi;

  glyph_index = FT_Get_Char_Index (face, unicode);
  get_glyph_info (&glyph_cache, &atlas, face, glyph_index, &gi);
  add_glyph_vertices (-200, 100, 300, &gi, &vertices);

  glyph_index = FT_Get_Char_Index (face, 'x');
  get_glyph_info (&glyph_cache, &atlas, face, glyph_index, &gi);
  add_glyph_vertices (0, 100, 300, &gi, &vertices);

  glVertexAttribPointer (a_glyph_vertex_loc, 4, GL_FLOAT, GL_FALSE, sizeof (glyph_vertex_t), (const char *) &vertices[0]);
  glUseProgram (program);
  atlas_use (&atlas, program);
  glut_main ();

  return 0;
}
