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

#ifndef GLYPHY_DEMO_ATLAS_H
#define GLYPHY_DEMO_ATLAS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glyphy-demo-gl.h"


typedef struct {
  GLuint tex_unit;
  GLuint tex_name;
  GLuint tex_w;
  GLuint tex_h;
  GLuint item_w;
  GLuint item_h_q; /* height quantum */
  GLuint cursor_x;
  GLuint cursor_y;
} atlas_t;

static inline void
atlas_bind (atlas_t *at)
{
  glActiveTexture (at->tex_unit);
  glBindTexture (GL_TEXTURE_2D, at->tex_name);
}

static void
atlas_init (atlas_t *at,
	    unsigned int w,
	    unsigned int h,
	    unsigned int item_w,
	    unsigned int item_h_quantum)
{
  glGetIntegerv (GL_ACTIVE_TEXTURE, (GLint *) &at->tex_unit);
  glGenTextures (1, &at->tex_name);
  at->tex_w = w;
  at->tex_h = h;
  at->item_w = item_w;
  at->item_h_q = item_h_quantum;
  at->cursor_x = 0;
  at->cursor_y = 0;

  atlas_bind (at);

  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  gl(TexImage2D) (GL_TEXTURE_2D, 0, GL_RGBA, at->tex_w, at->tex_h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
}

static void
atlas_use (atlas_t *at, GLuint program)
{
  glUniform4i (glGetUniformLocation (program, "u_atlas_info"),
	       at->tex_w, at->tex_h, at->item_w, at->item_h_q);
  glUniform1i (glGetUniformLocation (program, "u_atlas_tex"), at->tex_unit - GL_TEXTURE0);
}

static void
atlas_alloc (atlas_t *at,
	     glyphy_rgba_t *data,
	     unsigned int len,
	     unsigned int *px,
	     unsigned int *py)
{
  GLuint w, h, x, y;

  w = at->item_w;
  h = (len + w - 1) / w;

  if (at->cursor_y + h > at->tex_h) {
    /* Go to next column */
    at->cursor_x += at->item_w;
    at->cursor_y = 0;
  }

  if (at->cursor_x + w <= at->tex_w &&
      at->cursor_y + h <= at->tex_h)
  {
    x = at->cursor_x;
    y = at->cursor_y;
    at->cursor_y += (h + at->item_h_q - 1) & ~(at->item_h_q - 1);
  } else {
    fprintf (stderr, "Ran out of atlas memory\n");
    abort ();
  }

  atlas_bind (at);
  if (w * h == len)
    gl(TexSubImage2D) (GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, data);
  else {
    gl(TexSubImage2D) (GL_TEXTURE_2D, 0, x, y, w, h - 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
    /* Upload the last row separately */
    gl(TexSubImage2D) (GL_TEXTURE_2D, 0, x, y + h - 1, len - (w * (h - 1)), 1, GL_RGBA, GL_UNSIGNED_BYTE,
		       data + w * (h - 1));
  }

  *px = x / at->item_w;
  *py = y / at->item_h_q;
  return;
}

#endif /* GLYPHY_DEMO_ATLAS_H */