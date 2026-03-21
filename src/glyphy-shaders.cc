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

#include "glyphy.h"

#include "glyphy-fragment-glsl.h"
#include "glyphy-vertex-glsl.h"

const char * glyphy_fragment_shader_source (void) { return glyphy_fragment_glsl; }
const char * glyphy_vertex_shader_source (void) { return glyphy_vertex_glsl; }
