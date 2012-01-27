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

#include <assert.h>

#include <vector>



#if 1
// Large font size profile
#define MIN_FONT_SIZE 64
#define TOLERANCE 5e-4
#else
// Small font size profile
#define MIN_FONT_SIZE 20
#define TOLERANCE 5e-3
#endif



static void
die (const char *msg)
{
  fprintf (stderr, "%s\n", msg);
  exit (1);
}



static glyphy_bool_t
accumulate_endpoint (glyphy_arc_endpoint_t              *endpoint,
		     std::vector<glyphy_arc_endpoint_t> *endpoints)
{
  endpoints->push_back (*endpoint);
  return true;
}

static void
glyphy_freetype_glyph_encode (FT_Face face, unsigned int glyph_index,
			      double tolerance_per_em,
			      void *buffer, unsigned int buffer_size,
			      unsigned int *output_size)
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

  double tolerance = face->units_per_EM * tolerance_per_em; /* in font design units */
  double faraway = double (face->units_per_EM) / MIN_FONT_SIZE;
  std::vector<glyphy_arc_endpoint_t> endpoints;

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
  unsigned int glyph_layout;
  glyphy_extents_t extents;

  if (!glyphy_arc_list_encode_rgba (&endpoints[0], endpoints.size (),
				    (glyphy_rgba_t *) buffer,
				    buffer_size / sizeof (glyphy_rgba_t),
				    faraway,
				    4,
				    &avg_fetch_achieved,
				    output_size,
				    &glyph_layout,
				    &extents))
    die ("Failed encoding arcs");

  *output_size *= sizeof (glyphy_rgba_t);

  printf ("Average %g texture accesses\n", avg_fetch_achieved);
}




#define TEX_W 512
#define TEX_H 512
#define SUB_TEX_W 64

#define gl(name) \
	for (GLint __ee, __ii = 0; \
	     __ii < 1; \
	     (__ii++, \
	      (__ee = glGetError()) && \
	      (fprintf (stderr, "gl" #name " failed with error %04X on line %d", __ee, __LINE__), abort (), 0))) \
	  gl##name

static GLint
create_texture (const char *font_path, unsigned int unicode)
{
  FT_Face face;
  FT_Library library;
  FT_Init_FreeType (&library);
  FT_New_Face (library, font_path, 0, &face);

  char buffer[50000];
  unsigned int output_size;

  glyphy_freetype_glyph_encode (face,
			        FT_Get_Char_Index (face, unicode),
				TOLERANCE,
			        buffer, sizeof (buffer),
			        &output_size);

  printf ("Used %'u bytes\n", output_size);

  int tex_w = SUB_TEX_W;
  int tex_h = (output_size + sizeof (glyphy_rgba_t) * (tex_w - 1)) / sizeof (glyphy_rgba_t) / tex_w;

  GLuint texture;
  glGenTextures (1, &texture);
  glBindTexture (GL_TEXTURE_2D, texture);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  /* Upload*/
  gl(TexImage2D) (GL_TEXTURE_2D, 0, GL_RGBA, TEX_W, TEX_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  gl(TexSubImage2D) (GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_RGBA, GL_UNSIGNED_BYTE, buffer);

  GLuint program;
  glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *) &program);
  glUniform4f (glGetUniformLocation(program, "u_atlas_info"), TEX_W, TEX_H, SUB_TEX_W, 0);
  glUniform1i (glGetUniformLocation(program, "u_atlas_tex"), 0);
  glActiveTexture (GL_TEXTURE0);

  return texture;
}

unsigned int
glyph_encode (unsigned int atlas_x,  /* 9 bits, should be multiple of 4 */
	      unsigned int atlas_y,  /* 9 bits, should be multiple of 4 */
	      unsigned int corner_x, /* 1 bit */
	      unsigned int corner_y, /* 1 bit */
	      unsigned int glyph_layout /* 16 bits */)
{
  assert (0 == (atlas_x & ~0x1FC));
  assert (0 == (atlas_y & ~0x1FC));
  assert (0 == (corner_x & ~1));
  assert (0 == (corner_y & ~1));
  assert (0 == (glyph_layout & ~0xFFFF));

  unsigned int x = (((atlas_x << 6) | (glyph_layout >> 8))   << 1) | corner_x;
  unsigned int y = (((atlas_y << 6) | (glyph_layout & 0xFF)) << 1) | corner_y;

  return (x << 16) | y;
}

int
main (int argc, char** argv)
{
  char *font_path = NULL;
  char utf8 = 0;
  if (argc >= 3) {
     font_path = argv[1];
     utf8 = argv[2][0];
     if (argc >= 4)
       animate = atoi (argv[3]);
  }
  else
    die ("Usage: grid PATH_TO_FONT_FILE CHARACTER_TO_DRAW ANIMATE?");

  glut_init (&argc, argv);

  GLuint program = create_program ();
  glUseProgram (program);

  GLuint texture = create_texture (font_path, utf8);

  struct glyph_attrib_t {
    GLfloat x;
    GLfloat y;

    GLfloat g16hi;
    GLfloat g16lo;
  };
  glyph_attrib_t w_vertices[10];
  unsigned int num_vertices = 0;
  unsigned int glyph_layout = 0x1010;

#define ENCODE_CORNER(_x, _y, _cx, _cy, _glyph_layout) \
  do { \
    unsigned int i = num_vertices; \
    unsigned int encoded = glyph_encode (0, 0, _cx, _cy, _glyph_layout); \
    w_vertices[i].x = _x; \
    w_vertices[i].y = _y; \
    w_vertices[i].g16hi = encoded >> 16; \
    w_vertices[i].g16lo = encoded & 0xFFFF; \
    num_vertices++; \
  } while (0)

  ENCODE_CORNER (-1, -1, 0, 0, glyph_layout);
  ENCODE_CORNER (+1, -1, 1, 0, glyph_layout);
  ENCODE_CORNER (+1, +1, 1, 1, glyph_layout);
  ENCODE_CORNER (-1, +1, 0, 1, glyph_layout);

  GLuint a_pos_loc = glGetAttribLocation (program, "a_position");
  GLuint a_glyph_loc = glGetAttribLocation (program, "a_glyph");
  glVertexAttribPointer (a_pos_loc, 2, GL_FLOAT, GL_FALSE, sizeof (glyph_attrib_t),
			 (const char *) w_vertices + offsetof (glyph_attrib_t, x));
  glVertexAttribPointer (a_glyph_loc, 2, GL_FLOAT, GL_FALSE, sizeof (glyph_attrib_t),
			 (const char *) w_vertices + offsetof (glyph_attrib_t, g16hi));
  glEnableVertexAttribArray (a_pos_loc);
  glEnableVertexAttribArray (a_glyph_loc);

  glut_main ();

  return 0;
}
