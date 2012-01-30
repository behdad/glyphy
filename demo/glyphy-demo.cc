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

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <vector>

using namespace std;


#define STRINGIZE1(Src) #Src
#define STRINGIZE(Src) STRINGIZE1(Src)
#define ARRAY_LEN(Array) (sizeof (Array) / sizeof (*Array))


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

#include "glyphy-demo-glut.h"
#include "glyphy-demo-shaders.h"
#include "glyphy-demo-font.h"




static FT_Face
open_ft_face (const char   *font_path,
	      unsigned int  face_index)
{
  static FT_Library library;
  if (!library)
    FT_Init_FreeType (&library);
  FT_Face face;
  FT_New_Face (library, font_path, face_index, &face);
  if (!face)
    die ("Failed to open font file");
  return face;
}

int
main (int argc, char** argv)
{
  const char *font_path = NULL;
  const char *text = NULL;

  if (argc >= 3) {
     font_path = argv[1];
     text = argv[2];
     if (argc >= 4)
       animate = atoi (argv[3]);
  }
  else {
    fprintf (stderr, "Usage: %s FONT_FILE TEXT ANIMATE?\n", argv[0]);
    exit (1);
  }

  glut_init (&argc, argv);

  GLuint program = create_program ();

  atlas_t atlas;
  atlas_init (&atlas, 512, 512, 32, 4);

  FT_Face face = open_ft_face (font_path, 0);

  glyphy_demo_font_t *font = glyphy_demo_font_create (face, &atlas);

#define FONT_SIZE 300

  vertices.clear ();
  glyphy_point_t cursor = {-200, 100};
  for (const char *p = text; *p; p++) {
    unsigned int unicode = *p;
    unsigned int glyph_index = FT_Get_Char_Index (face, unicode);
    glyph_info_t gi;
    glyphy_demo_font_lookup_glyph (font, glyph_index, &gi);
    add_glyph_vertices (cursor, FONT_SIZE, &gi, &vertices);
    cursor.x += FONT_SIZE * gi.advance;
  }

  glUseProgram (program);
  atlas_use (&atlas, program);
  glut_main ();

  return 0;
}
