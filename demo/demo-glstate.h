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

#ifndef DEMO_GLSTATE_H
#define DEMO_GLSTATE_H

#include "demo-common.h"
#include "demo-buffer.h"

#include "demo-atlas.h"
#include "demo-shader.h"

typedef struct demo_glstate_t demo_glstate_t;

demo_glstate_t *
demo_glstate_create (void);

demo_glstate_t *
demo_glstate_reference (demo_glstate_t *st);

void
demo_glstate_destroy (demo_glstate_t *st);


void
demo_glstate_setup (demo_glstate_t *st);

demo_atlas_t *
demo_glstate_get_atlas (demo_glstate_t *st);

void
demo_glstate_scale_gamma_adjust (demo_glstate_t *st, double factor);

void
demo_glstate_scale_contrast (demo_glstate_t *st, double factor);

void
demo_glstate_toggle_debug (demo_glstate_t *st);

void
demo_glstate_next_smoothfunc (demo_glstate_t *st);

void
demo_glstate_set_matrix (demo_glstate_t *st, float mat[16]);

void
demo_glstate_toggle_outline (demo_glstate_t *st);

void
demo_glstate_scale_outline_thickness (demo_glstate_t *st, double factor);


#endif /* DEMO_GLSTATE_H */
