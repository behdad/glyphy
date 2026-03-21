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
  /* Band transform (constant across glyph) */
  GLfloat band_scale_x;
  GLfloat band_scale_y;
  GLfloat band_offset_x;
  GLfloat band_offset_y;
  /* Glyph data (constant across glyph) */
  GLint atlas_x;
  GLint atlas_y;
  GLint num_hbands;
  GLint num_vbands;
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
