/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "demo-atlas.h"


struct demo_atlas_t {
  unsigned int refcount;

  GLuint tex_unit;
  GLuint tex_name;
  GLuint buf_name;
  GLuint capacity;
  GLuint cursor;
};

static void
demo_atlas_bind_texture (demo_atlas_t *at);


demo_atlas_t *
demo_atlas_create (unsigned int capacity)
{
  demo_atlas_t *at = (demo_atlas_t *) calloc (1, sizeof (demo_atlas_t));
  at->refcount = 1;

  glGetIntegerv (GL_ACTIVE_TEXTURE, (GLint *) &at->tex_unit);
  glGenBuffers (1, &at->buf_name);
  glGenTextures (1, &at->tex_name);
  at->capacity = capacity;
  at->cursor = 0;

  glBindBuffer (GL_TEXTURE_BUFFER, at->buf_name);
  glBufferData (GL_TEXTURE_BUFFER, capacity * sizeof (glyphy_texel_t), NULL, GL_STATIC_DRAW);

  demo_atlas_bind_texture (at);
  gl(TexBuffer) (GL_TEXTURE_BUFFER, GL_RGBA16I, at->buf_name);

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
  glDeleteBuffers (1, &at->buf_name);
  free (at);
}

static void
demo_atlas_bind_texture (demo_atlas_t *at)
{
  glActiveTexture (at->tex_unit);
  glBindTexture (GL_TEXTURE_BUFFER, at->tex_name);
}

void
demo_atlas_set_uniforms (demo_atlas_t *at)
{
  GLuint program;
  glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *) &program);

  glUniform1i (glGetUniformLocation (program, "u_atlas"), at->tex_unit - GL_TEXTURE0);
}

unsigned int
demo_atlas_alloc (demo_atlas_t    *at,
                  glyphy_texel_t  *data,
                  unsigned int     len)
{
  if (at->cursor + len > at->capacity)
    die ("Ran out of atlas memory");

  unsigned int offset = at->cursor;
  at->cursor += len;

  glBindBuffer (GL_TEXTURE_BUFFER, at->buf_name);
  glBufferSubData (GL_TEXTURE_BUFFER,
                   offset * sizeof (glyphy_texel_t),
                   len * sizeof (glyphy_texel_t),
                   data);

  return offset;
}

unsigned int
demo_atlas_get_used (demo_atlas_t *at)
{
  return at->cursor;
}
