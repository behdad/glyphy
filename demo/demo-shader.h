/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
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
  /* Corner direction for dilation (-1 or +1 per axis) */
  GLfloat cx;
  GLfloat cy;
  /* Texcoord-per-position ratio (constant across glyph) */
  GLfloat tpx;
  GLfloat tpy;
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
