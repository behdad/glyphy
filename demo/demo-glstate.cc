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

struct demo_glstate_t {
  unsigned int   refcount;

  GLuint program;
  demo_atlas_t *atlas;

  /* Uniforms */
  double u_debug;
  double u_contrast;
  double u_gamma_adjust;
  double u_outline;
  double u_outline_thickness;
  double u_boldness;
};

demo_glstate_t *
demo_glstate_create (void)
{
  TRACE();

  demo_glstate_t *st = (demo_glstate_t *) calloc (1, sizeof (demo_glstate_t));
  st->refcount = 1;

  st->program = demo_shader_create_program ();
  st->atlas = demo_atlas_create (2048, 1024, 64, 8);

  st->u_debug = false;
  st->u_contrast = 1.0;
  st->u_gamma_adjust = 1.0;
  st->u_outline = false;
  st->u_outline_thickness = 1.0;
  st->u_boldness = 0.;

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
set_uniform (GLuint program, const char *name, double *p, double value, double scale=1.0)
{
  *p = value;
  glUniform1f (glGetUniformLocation (program, name), value * scale);
  LOGI ("Setting %s to %g\n", name + 2, value);
}

#define SET_UNIFORM(name, value) set_uniform (st->program, #name, &st->name, value)
#define SET_UNIFORM_SCALE(name, value, scale) set_uniform (st->program, #name, &st->name, value, scale)

void
demo_glstate_setup (demo_glstate_t *st)
{
  glUseProgram (st->program);

  demo_atlas_set_uniforms (st->atlas);

  SET_UNIFORM (u_debug, st->u_debug);
  SET_UNIFORM (u_contrast, st->u_contrast);
  SET_UNIFORM (u_gamma_adjust, st->u_gamma_adjust);
  SET_UNIFORM (u_outline, st->u_outline);
  SET_UNIFORM (u_outline_thickness, st->u_outline_thickness);
  SET_UNIFORM (u_boldness, st->u_boldness);

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

demo_atlas_t *
demo_glstate_get_atlas (demo_glstate_t *st)
{
  return st->atlas;
}

void
demo_glstate_scale_gamma_adjust (demo_glstate_t *st, double factor)
{
  SET_UNIFORM (u_gamma_adjust, clamp (st->u_gamma_adjust * factor, .1, 10.));
}

void
demo_glstate_scale_contrast (demo_glstate_t *st, double factor)
{
  SET_UNIFORM (u_contrast, clamp (st->u_contrast * factor, .1, 10.));
}

void
demo_glstate_toggle_debug (demo_glstate_t *st)
{
  SET_UNIFORM (u_debug, 1 - st->u_debug);
}

void
demo_glstate_set_matrix (demo_glstate_t *st, float mat[16])
{
  glUniformMatrix4fv (glGetUniformLocation (st->program, "u_matViewProjection"), 1, GL_FALSE, mat);
}

void
demo_glstate_toggle_outline (demo_glstate_t *st)
{
  SET_UNIFORM (u_outline, 1 - st->u_outline);
}

void
demo_glstate_scale_outline_thickness (demo_glstate_t *st, double factor)
{
  SET_UNIFORM (u_outline_thickness, clamp (st->u_outline_thickness * factor, .5, 3.));
}

void
demo_glstate_adjust_boldness (demo_glstate_t *st, double adjustment)
{
  SET_UNIFORM_SCALE (u_boldness, clamp (st->u_boldness + adjustment, -ENLIGHTEN_MAX, EMBOLDEN_MAX), GRID_SIZE);
}
