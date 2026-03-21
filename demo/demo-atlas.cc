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

#include "demo-atlas.h"


struct demo_atlas_t {
  unsigned int refcount;

  GLuint tex_unit;
  GLuint tex_name;
  GLuint tex_w;
  GLuint tex_h;
  GLuint item_w;
  GLuint item_h_q;
  GLuint cursor_x;
  GLuint cursor_y;
};


demo_atlas_t *
demo_atlas_create (unsigned int w,
		   unsigned int h,
		   unsigned int item_w,
		   unsigned int item_h_quantum)
{
  TRACE();

  demo_atlas_t *at = (demo_atlas_t *) calloc (1, sizeof (demo_atlas_t));
  at->refcount = 1;

  glGetIntegerv (GL_ACTIVE_TEXTURE, (GLint *) &at->tex_unit);
  glGenTextures (1, &at->tex_name);
  at->tex_w = w;
  at->tex_h = h;
  at->item_w = item_w;
  at->item_h_q = item_h_quantum;
  at->cursor_x = 0;
  at->cursor_y = 0;

  demo_atlas_bind_texture (at);

  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  gl(TexImage2D) (GL_TEXTURE_2D, 0, GL_RGBA16I, at->tex_w, at->tex_h, 0, GL_RGBA_INTEGER, GL_SHORT, NULL);

  return at;
}

demo_atlas_t *
demo_atlas_reference (demo_atlas_t *at)
{
  if (at) at->refcount++;
  return at;
}

void
demo_atlas_destroy (demo_atlas_t *at)
{
  if (!at || --at->refcount)
    return;

  glDeleteTextures (1, &at->tex_name);
  free (at);
}

void
demo_atlas_bind_texture (demo_atlas_t *at)
{
  glActiveTexture (at->tex_unit);
  glBindTexture (GL_TEXTURE_2D, at->tex_name);
}

void
demo_atlas_set_uniforms (demo_atlas_t *at)
{
  GLuint program;
  glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *) &program);

  glUniform1i (glGetUniformLocation (program, "u_atlas"), at->tex_unit - GL_TEXTURE0);
}

void
demo_atlas_alloc (demo_atlas_t    *at,
		  glyphy_texel_t  *data,
		  unsigned int     len,
		  unsigned int    *px,
		  unsigned int    *py)
{
  GLuint w, h, x, y;

  w = at->item_w;
  h = (len + w - 1) / w;

  if (at->cursor_y + h > at->tex_h) {
    at->cursor_x += at->item_w;
    at->cursor_y = 0;
  }

  if (at->cursor_x + w <= at->tex_w &&
      at->cursor_y + h <= at->tex_h)
  {
    x = at->cursor_x;
    y = at->cursor_y;
    at->cursor_y += (h + at->item_h_q - 1) & ~(at->item_h_q - 1);
  } else
    die ("Ran out of atlas memory");

  demo_atlas_bind_texture (at);
  if (w * h == len)
    gl(TexSubImage2D) (GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA_INTEGER, GL_SHORT, data);
  else {
    gl(TexSubImage2D) (GL_TEXTURE_2D, 0, x, y, w, h - 1, GL_RGBA_INTEGER, GL_SHORT, data);
    gl(TexSubImage2D) (GL_TEXTURE_2D, 0, x, y + h - 1, len - (w * (h - 1)), 1, GL_RGBA_INTEGER, GL_SHORT,
		       data + w * (h - 1));
  }

  *px = x;
  *py = y;
}
