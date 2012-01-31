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

#include "demo-state.h"

void
demo_state_init (demo_state_t *st)
{
  st->program = demo_shader_create_program ();
  st->atlas = demo_atlas_create (512, 512, 32, 4);

  st->u_debug = 0;
  st->u_contrast = 1.0;
  st->u_gamma = 2.2;
}

void
demo_state_fini (demo_state_t *st)
{
  demo_atlas_destroy (st->atlas);
  glDeleteProgram (st->program);
}


static void
set_uniform (demo_state_t *st,
	     const char *name,
	     double *p,
	     double value)
{
  *p = value;
  glUniform1f (glGetUniformLocation (st->program, name), value);
}

#define SET_UNIFORM(name, value) set_uniform (st, #name, &st->name, value)

void
demo_state_setup (demo_state_t *st)
{
  glUseProgram (st->program);
  demo_atlas_set_uniforms (st->atlas);
  SET_UNIFORM (u_debug, 0);
  SET_UNIFORM (u_contrast, 1.0);
  SET_UNIFORM (u_gamma, 2.2);
}
