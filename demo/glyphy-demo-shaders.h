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
 * Google Author(s): Behdad Esfahbod, Maysum Panju
 */

#ifndef GLYPHY_DEMO_SHADERS_H
#define GLYPHY_DEMO_SHADERS_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glyphy-demo-gl.h"
#include "glyphy-demo-font.h"

#include "glyphy-demo-atlas-glsl.h"
#include "glyphy-demo-vshader-glsl.h"
#include "glyphy-demo-fshader-glsl.h"

#include <assert.h>


unsigned int
glyph_encode (unsigned int atlas_x,  /* 7 bits */
	      unsigned int atlas_y,  /* 7 bits */
	      unsigned int corner_x, /* 1 bit */
	      unsigned int corner_y, /* 1 bit */
	      unsigned int glyph_layout /* 16 bits */)
{
  assert (0 == (atlas_x & ~0x7F));
  assert (0 == (atlas_y & ~0x7F));
  assert (0 == (corner_x & ~1));
  assert (0 == (corner_y & ~1));
  assert (0 == (glyph_layout & ~0xFFFF));

  unsigned int x = (((atlas_x << 8) | (glyph_layout >> 8))   << 1) | corner_x;
  unsigned int y = (((atlas_y << 8) | (glyph_layout & 0xFF)) << 1) | corner_y;

  return (x << 16) | y;
}

struct glyph_vertex_t {
  /* Position */
  GLfloat x;
  GLfloat y;
  /* Glyph info */
  GLfloat g16hi;
  GLfloat g16lo;
};

static void
glyph_vertex_encode (double x, double y,
		     unsigned int corner_x, unsigned int corner_y,
		     const glyph_info_t *gi,
		     glyph_vertex_t *v)
{
  unsigned int encoded = glyph_encode (gi->atlas_x, gi->atlas_y,
				       corner_x, corner_y,
				       gi->glyph_layout);
  v->x = x;
  v->y = y;
  v->g16hi = encoded >> 16;
  v->g16lo = encoded & 0xFFFF;
}

static void
add_glyph_vertices (const glyphy_point_t   &p,
		    double                  font_size,
		    glyph_info_t           *gi,
		    vector<glyph_vertex_t> *vertices)
{
  glyph_vertex_t v;

#define ENCODE_CORNER(_cx, _cy) \
  do { \
    double _vx = p.x + font_size * ((1-_cx) * gi->extents.min_x + _cx * gi->extents.max_x); \
    double _vy = p.y - font_size * ((1-_cy) * gi->extents.min_y + _cy * gi->extents.max_y); \
    glyph_vertex_encode (_vx, _vy, _cx, _cy, gi, &v); \
    vertices->push_back (v); \
  } while (0)

  ENCODE_CORNER (0, 0);
  ENCODE_CORNER (1, 0);
  ENCODE_CORNER (1, 1);

  ENCODE_CORNER (1, 1);
  ENCODE_CORNER (0, 1);
  ENCODE_CORNER (0, 0);
#undef ENCODE_CORNER
}




#define GLSL_VERSION_STRING "#version 120\n"

GLuint
create_program (void)
{
  GLuint vshader, fshader, program;
  const GLchar *vshader_sources[] = {GLSL_VERSION_STRING,
				     glyphy_demo_vshader_glsl};
  vshader = compile_shader (GL_VERTEX_SHADER, ARRAY_LEN (vshader_sources), vshader_sources);
  const GLchar *fshader_sources[] = {GLSL_VERSION_STRING,
				     glyphy_demo_atlas_glsl,
				     glyphy_common_shader_source (),
				     glyphy_sdf_shader_source (),
				     glyphy_demo_fshader_glsl};
  fshader = compile_shader (GL_FRAGMENT_SHADER, ARRAY_LEN (fshader_sources), fshader_sources);

  program = link_program (vshader, fshader);
  return program;
}



#endif /* GLYPHY_DEMO_SHADERS_H */
