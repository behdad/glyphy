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

#include "demo-glstate.h"

demo_glstate_t *
demo_glstate_create (void)
{
  demo_glstate_t *st = (demo_glstate_t *) calloc (1, sizeof (demo_glstate_t));
  st->refcount = 1;

  st->program = demo_shader_create_program ();
  st->atlas = demo_atlas_create (2048, 1024, 64, 8);

  st->u_debug = false;
  st->u_smoothfunc = 1;
  st->u_contrast = 1.0;
  st->u_gamma_adjust = 1.0;

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


static void
set_uniform (GLuint program, const char *name, double *p, double value)
{
  *p = value;
  glUniform1f (glGetUniformLocation (program, name), value);
  printf ("Setting %s to %g\n", name, value);
}

#define SET_UNIFORM(name, value) set_uniform (st->program, #name, &st->name, value)

void
demo_glstate_setup (demo_glstate_t *st)
{
  glUseProgram (st->program);
  demo_atlas_set_uniforms (st->atlas);
  SET_UNIFORM (u_debug, st->u_debug);
  SET_UNIFORM (u_smoothfunc, st->u_smoothfunc);
  SET_UNIFORM (u_contrast, st->u_contrast);
  SET_UNIFORM (u_gamma_adjust, st->u_gamma_adjust);
}

demo_atlas_t *
demo_glstate_get_atlas (demo_glstate_t *st)
{
  return st->atlas;
}
