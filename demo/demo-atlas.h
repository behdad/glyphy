/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef DEMO_ATLAS_H
#define DEMO_ATLAS_H

#include "demo-common.h"


typedef struct demo_atlas_t demo_atlas_t;

demo_atlas_t *
demo_atlas_create (unsigned int capacity);

demo_atlas_t *
demo_atlas_reference (demo_atlas_t *at);

void
demo_atlas_destroy (demo_atlas_t *at);


/* Returns the 1D offset where the data was placed. */
unsigned int
demo_atlas_alloc (demo_atlas_t    *at,
		  glyphy_texel_t  *data,
		  unsigned int     len);

void
demo_atlas_bind_texture (demo_atlas_t *at);

void
demo_atlas_set_uniforms (demo_atlas_t *at);


#endif /* DEMO_ATLAS_H */
