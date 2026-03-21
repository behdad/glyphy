/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */

#ifndef DEMO_SHADERS_H
#define DEMO_SHADERS_H

#include "demo-common.h"
#include "demo-font.h"


struct glyph_vertex_t {
  /* Object-space position */
  GLfloat x;
  GLfloat y;
  /* Em-space texture coordinates */
  GLfloat tx;
  GLfloat ty;
  /* Object-space normal for dilation (corner direction * scale) */
  GLfloat nx;
  GLfloat ny;
  /* Inverse Jacobian: maps object-space to em-space (constant across glyph) */
  GLfloat jx;
  GLfloat jy;
  /* Atlas offset (constant across glyph) */
  GLuint atlas_offset;
  GLuint _padding;
};

void
demo_shader_add_glyph_vertices (const glyphy_point_t        &p,
				double                       font_size,
				glyph_info_t                *gi,
				std::vector<glyph_vertex_t> *vertices,
				glyphy_extents_t            *extents);


GLuint
demo_shader_create_program (void);


#endif /* DEMO_SHADERS_H */
