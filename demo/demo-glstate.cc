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

#include "demo-glstate.h"

struct demo_glstate_t {
  unsigned int   refcount;

  GLuint program;
  demo_atlas_t *atlas;
};

demo_glstate_t *
demo_glstate_create (void)
{
  TRACE();

  demo_glstate_t *st = (demo_glstate_t *) calloc (1, sizeof (demo_glstate_t));
  st->refcount = 1;

  st->program = demo_shader_create_program ();
  st->atlas = demo_atlas_create (1024 * 1024);

  return st;
}

demo_glstate_t *
demo_glstate_reference (demo_glstate_t *st)
{
  if (st) st->refcount++;
  return st;
}

void
demo_glstate_destroy (demo_glstate_t *st)
{
  if (!st || --st->refcount)
    return;

  demo_atlas_destroy (st->atlas);
  glDeleteProgram (st->program);

  free (st);
}

void
demo_glstate_setup (demo_glstate_t *st)
{
  glUseProgram (st->program);

  demo_atlas_set_uniforms (st->atlas);

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

demo_atlas_t *
demo_glstate_get_atlas (demo_glstate_t *st)
{
  return st->atlas;
}

void
demo_glstate_set_matrix (demo_glstate_t *st, float mat[16])
{
  glUniformMatrix4fv (glGetUniformLocation (st->program, "u_matViewProjection"), 1, GL_FALSE, mat);
}

