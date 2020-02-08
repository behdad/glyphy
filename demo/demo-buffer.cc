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
  glyphy_extents_t ink_extents;
  glyphy_extents_t logical_extents;
  bool dirty;
  GLuint buf_name;
};

demo_buffer_t *
demo_buffer_create (void)
{
  demo_buffer_t *buffer = (demo_buffer_t *) calloc (1, sizeof (demo_buffer_t));
  buffer->refcount = 1;

  buffer->vertices = new std::vector<glyph_vertex_t>;
  glGenBuffers (1, &buffer->buf_name);

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

  glDeleteBuffers (1, &buffer->buf_name);
  delete buffer->vertices;
  free (buffer);
}


void
demo_buffer_clear (demo_buffer_t *buffer)
{
  buffer->vertices->clear ();
  glyphy_extents_clear (&buffer->ink_extents);
  glyphy_extents_clear (&buffer->logical_extents);
  buffer->dirty = true;
}

void
demo_buffer_extents (demo_buffer_t    *buffer,
		     glyphy_extents_t *ink_extents,
		     glyphy_extents_t *logical_extents)
{
  if (ink_extents)
    *ink_extents = buffer->ink_extents;
  if (logical_extents)
    *logical_extents = buffer->logical_extents;
}

void
demo_buffer_move_to (demo_buffer_t        *buffer,
		     const glyphy_point_t *p)
{
  buffer->cursor = *p;
}

void
demo_buffer_current_point (demo_buffer_t  *buffer,
			   glyphy_point_t *p)
{
  *p = buffer->cursor;
}

void
demo_buffer_add_text (demo_buffer_t        *buffer,
		      const char           *utf8,
		      demo_font_t          *font,
		      double                font_size)
{
  hb_face_t *face = demo_font_get_face (font);
  hb_font_t *hb_font = hb_font_create (face);
  glyphy_point_t top_left = buffer->cursor;
  buffer->cursor.y += font_size /* * font->ascent */;

  hb_buffer_t *hb_buffer = hb_buffer_create ();
  hb_buffer_add_utf8 (hb_buffer, utf8, -1, 0, -1);
  hb_buffer_guess_segment_properties (hb_buffer);
  hb_shape (hb_font, hb_buffer, NULL, 0);
  unsigned length;
  hb_glyph_info_t *infos = hb_buffer_get_glyph_infos (hb_buffer, &length);
  hb_glyph_position_t *positions = hb_buffer_get_glyph_positions (hb_buffer, &length);

  for (unsigned i = 0; i < length; ++i)
  {
//     if (unicode == '\n') {
//       buffer->cursor.y += font_size;
//       buffer->cursor.x = top_left.x;
//       continue;
//     }

    unsigned int glyph_index = infos[i].codepoint;
    glyph_info_t gi;
    demo_font_lookup_glyph (font, glyph_index, &gi);

    /* Update ink extents */
    glyphy_extents_t ink_extents;
    demo_shader_add_glyph_vertices (buffer->cursor, font_size, &gi, buffer->vertices, &ink_extents);
    glyphy_extents_extend (&buffer->ink_extents, &ink_extents);

    /* Update logical extents */
    glyphy_point_t corner;
    corner.x = buffer->cursor.x;
    corner.y = buffer->cursor.y - font_size;
    glyphy_extents_add (&buffer->logical_extents, &corner);
    corner.x = buffer->cursor.x + font_size * gi.advance;
    corner.y = buffer->cursor.y;
    glyphy_extents_add (&buffer->logical_extents, &corner);

    buffer->cursor.x += font_size * gi.advance;
  }

  hb_buffer_destroy (hb_buffer);
  hb_font_destroy (hb_font);

  buffer->dirty = true;
}

void
demo_buffer_draw (demo_buffer_t *buffer)
{
  GLint program;
  glGetIntegerv (GL_CURRENT_PROGRAM, &program);
  GLuint a_glyph_vertex_loc = glGetAttribLocation (program, "a_glyph_vertex");
  glBindBuffer (GL_ARRAY_BUFFER, buffer->buf_name);
  if (buffer->dirty) {
    glBufferData (GL_ARRAY_BUFFER,  sizeof (glyph_vertex_t) * buffer->vertices->size (), (const char *) &(*buffer->vertices)[0], GL_STATIC_DRAW);
    buffer->dirty = false;
  }
  glEnableVertexAttribArray (a_glyph_vertex_loc);
  glVertexAttribPointer (a_glyph_vertex_loc, 4, GL_FLOAT, GL_FALSE, sizeof (glyph_vertex_t), 0);
  glDrawArrays (GL_TRIANGLES, 0, buffer->vertices->size ());
  glDisableVertexAttribArray (a_glyph_vertex_loc);
}
