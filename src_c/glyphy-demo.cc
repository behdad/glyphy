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

#include "glyphy-demo-freetype.h"
#include "glyphy-demo-glut.h"
#include "glyphy-demo-shaders.h"


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



glyphy_bool_t
ft_outline_to_texture (FT_Outline *outline, unsigned int upem, void *buffer,
		       unsigned int *output_len)
{
  double tolerance = upem * TOLERANCE; // in font design units
  std::vector<glyphy_arc_endpoint_t> endpoints;
  double error;

  if (!ft_outline_to_arcs (outline, tolerance, endpoints, &error))
    return false;

  glyphy_rgba_t rgba[10000];
  double avg_fetch_achieved;
  unsigned int glyph_layout;
  glyphy_extents_t extents;
  double faraway = double (upem) / MIN_FONT_SIZE;

  if (!glyphy_arc_list_encode_rgba (&endpoints[0], endpoints.size (),
				    &rgba[0], ARRAY_LEN (rgba),
				    faraway,
				    4,
				    &avg_fetch_achieved,
				    output_len,
				    &glyph_layout,
				    &extents))
    return false;

  printf ("Used %d arc endpoints; Approx. err %g; Tolerance %g; Percentage %g. %s\n",
	  (int) endpoints.size (), error, tolerance, round (100 * error / tolerance),
	  error <= tolerance ? "PASS" : "FAIL");
  printf ("Average %g texture accesses\n", avg_fetch_achieved);

  memcpy (buffer, &rgba[0], *output_len * sizeof (rgba[0]));

  return true;
}

int
ft_face_to_texture (FT_Face face, FT_ULong unicode, void *buffer,
		    unsigned int *output_len)
{
  unsigned int upem = face->units_per_EM;
  unsigned int glyph_index = FT_Get_Char_Index (face, unicode);

  if (FT_Load_Glyph (face,
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

  return ft_outline_to_texture (&face->glyph->outline, upem, buffer, output_len);
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

GLint
create_texture (const char *font_path, const char UTF8)
{
  FT_Face face;
  FT_Library library;
  FT_Init_FreeType (&library);
  FT_New_Face (library, font_path, 0, &face);

  char buffer[50000];
  unsigned int output_len;

  ft_face_to_texture (face, UTF8, buffer, &output_len);

  int tex_w = SUB_TEX_W;
  int tex_h = (output_len + tex_w - 1) / tex_w;

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
  glUniform3i (glGetUniformLocation(program, "u_texSize"), TEX_W, TEX_H, SUB_TEX_W);
  glUniform1i (glGetUniformLocation(program, "u_tex"), 0);
  glActiveTexture (GL_TEXTURE0);

  return texture;
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
    GLfloat g;
    GLfloat h;
  };
  const glyph_attrib_t w_vertices[] = {{-1, -1, 0, 0},
				       {+1, -1, 1, 0},
				       {+1, +1, 1, 1},
				       {-1, +1, 0, 1}};

  GLuint a_pos_loc = glGetAttribLocation (program, "a_position");
  GLuint a_glyph_loc = glGetAttribLocation (program, "a_glyph");
  glVertexAttribPointer (a_pos_loc, 2, GL_FLOAT, GL_FALSE, sizeof (glyph_attrib_t),
			 (const char *) w_vertices + offsetof (glyph_attrib_t, x));
  glVertexAttribPointer (a_glyph_loc, 2, GL_FLOAT, GL_FALSE, sizeof (glyph_attrib_t),
			 (const char *) w_vertices + offsetof (glyph_attrib_t, g));
  glEnableVertexAttribArray (a_pos_loc);
  glEnableVertexAttribArray (a_glyph_loc);

  glut_main ();

  return 0;
}
