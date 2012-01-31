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

#include "demo-common.h"
#include "demo-font.h"
#include "demo-shader.h"

#include "glyphy-demo-glut.h"

demo_state_t st;


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

  if (argc != 3) {
    fprintf (stderr, "Usage: %s FONT_FILE TEXT\n", argv[0]);
    exit (1);
  }
   font_path = argv[1];
   text = argv[2];

  glut_init (&argc, argv);

  demo_state_init (&st);
  FT_Face face = open_ft_face (font_path, 0);
  demo_font_t *font = demo_font_create (face, st.atlas);

#define FONT_SIZE 100

  glyphy_point_t top_left = {-200, -200};

  st.vertices->clear ();
  glyphy_point_t cursor = top_left;
  cursor.y += FONT_SIZE /* * font->ascent */;
  for (const char *p = text; *p; p++) {
    unsigned int unicode = *p;
    if (unicode == '\n') {
      cursor.y += FONT_SIZE;
      cursor.x = top_left.x;
      continue;
    }
    unsigned int glyph_index = FT_Get_Char_Index (face, unicode);
    glyph_info_t gi;
    demo_font_lookup_glyph (font, glyph_index, &gi);
    demo_shader_add_glyph_vertices (cursor, FONT_SIZE, &gi, st.vertices);
    cursor.x += FONT_SIZE * gi.advance;
  }

  demo_state_setup (&st);
  glutMainLoop ();

  demo_font_destroy (font);
  demo_state_fini (&st);

  return 0;
}
