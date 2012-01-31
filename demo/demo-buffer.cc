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
 * Google Author(s): Behdad Esfahbod
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "demo-buffer.h"

struct demo_buffer_t {
  unsigned int   refcount;

  glyphy_point_t cursor;
  std::vector<glyph_vertex_t> *vertices;
  glyphy_extents_t extents;
};

demo_buffer_t *
demo_buffer_create (void)
{
  demo_buffer_t *buffer = (demo_buffer_t *) calloc (1, sizeof (demo_buffer_t));
  buffer->refcount = 1;

  buffer->vertices = new std::vector<glyph_vertex_t>;

  demo_buffer_clear (buffer);

  return buffer;
}

demo_buffer_t *
demo_buffer_reference (demo_buffer_t *buffer)
{
  if (buffer) buffer->refcount++;
  return buffer;
}

void
demo_buffer_destroy (demo_buffer_t *buffer)
{
  if (!buffer || --buffer->refcount)
    return;

  delete buffer->vertices;
  free (buffer);
}


void
demo_buffer_clear (demo_buffer_t *buffer)
{
  buffer->vertices->clear ();
  glyphy_extents_clear (&buffer->extents);
}

void
demo_buffer_extents (demo_buffer_t    *buffer,
		     glyphy_extents_t *extents)
{
  *extents = buffer->extents;
}

void
demo_buffer_move_to (demo_buffer_t  *buffer,
		     glyphy_point_t  p)
{
  buffer->cursor = p;
}

void
demo_buffer_current_point (demo_buffer_t  *buffer,
			   glyphy_point_t *p)
{
  *p = buffer->cursor;
}

void
demo_buffer_add_text (demo_buffer_t  *buffer,
		      const char     *utf8,
		      demo_font_t    *font,
		      double          font_size,
		      glyphy_point_t  top_left)
{
  FT_Face face = demo_font_get_face (font);
  buffer->cursor = top_left;
  buffer->cursor.y += font_size /* * font->ascent */;
  for (const char *p = utf8; *p; p++) {
    unsigned int unicode = *p;
    if (unicode == '\n') {
      buffer->cursor.y += font_size;
      buffer->cursor.x = top_left.x;
      continue;
    }
    unsigned int glyph_index = FT_Get_Char_Index (face, unicode);
    glyph_info_t gi;
    glyphy_extents_t extents;
    demo_font_lookup_glyph (font, glyph_index, &gi);
    demo_shader_add_glyph_vertices (buffer->cursor, font_size, &gi, buffer->vertices, &extents);
    glyphy_extents_extend (&buffer->extents, &extents);
    buffer->cursor.x += font_size * gi.advance;
  }
}

void
demo_buffer_draw (demo_buffer_t *buffer,
		  demo_state_t  *st)
{
  GLuint a_glyph_vertex_loc = glGetAttribLocation (st->program, "a_glyph_vertex");
  glEnableVertexAttribArray (a_glyph_vertex_loc);
  glVertexAttribPointer (a_glyph_vertex_loc, 4, GL_FLOAT, GL_FALSE, sizeof (glyph_vertex_t), (const char *) &(*buffer->vertices)[0]);
  glDrawArrays (GL_TRIANGLES, 0, buffer->vertices->size ());
}
