/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glyphy.h"

#include "glyphy-fragment-glsl.h"
#include "glyphy-vertex-glsl.h"

const char * glyphy_fragment_shader_source (void) { return glyphy_fragment_glsl; }
const char * glyphy_vertex_shader_source (void) { return glyphy_vertex_glsl; }
