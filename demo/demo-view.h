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

#ifndef DEMO_VIEW_H
#define DEMO_VIEW_H

#include "demo-common.h"
#include "demo-buffer.h"
#include "demo-glstate.h"

typedef struct demo_view_t demo_view_t;

demo_view_t *
demo_view_create (demo_glstate_t *st);

demo_view_t *
demo_view_reference (demo_view_t *vu);

void
demo_view_destroy (demo_view_t *vu);


void
demo_view_reset (demo_view_t *vu);

void
demo_view_reshape_func (demo_view_t *vu, int width, int height);

void
demo_view_keyboard_func (demo_view_t *vu, unsigned char key, int x, int y);

void
demo_view_special_func (demo_view_t *view, int key, int x, int y);

void
demo_view_mouse_func (demo_view_t *vu, int button, int state, int x, int y);

void
demo_view_motion_func (demo_view_t *vu, int x, int y);

void
demo_view_print_help (demo_view_t *vu);

void
demo_view_display (demo_view_t *vu, demo_buffer_t *buffer);

void
demo_view_setup (demo_view_t *vu);


#endif /* DEMO_VIEW_H */
