/*
 * Copyright 2012 Google, Inc. All Rights Reserved.
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
