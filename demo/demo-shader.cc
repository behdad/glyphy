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

#include "demo-shader.h"

#include "demo-vshader-glsl.h"
#include "demo-fshader-glsl.h"


void
demo_shader_add_glyph_vertices (const glyphy_point_t        &p,
				double                       font_size,
				glyph_info_t                *gi,
				std::vector<glyph_vertex_t> *vertices,
				glyphy_extents_t            *extents)
{
  if (gi->is_empty)
    return;

  /* gi->extents are in normalized em-space (0..1).
   * The blob stores curves in font design units.
   * We need texcoords in font design units to match,
   * so multiply back by upem.  Band transform also in font units. */
  double upem = gi->upem;
  double min_x = gi->extents.min_x * upem;
  double max_x = gi->extents.max_x * upem;
  double min_y = gi->extents.min_y * upem;
  double max_y = gi->extents.max_y * upem;
  double width = max_x - min_x;
  double height = max_y - min_y;

  float band_scale_x = (width > 0)  ? (float) (gi->num_vbands / width) : 0.f;
  float band_scale_y = (height > 0) ? (float) (gi->num_hbands / height) : 0.f;
  float band_offset_x = (float) (-min_x * band_scale_x);
  float band_offset_y = (float) (-min_y * band_scale_y);

  glyph_vertex_t v[4];

  for (int ci = 0; ci < 4; ci++) {
    int cx = (ci >> 1) & 1;
    int cy = ci & 1;

    double vx = p.x + font_size * ((1 - cx) * gi->extents.min_x + cx * gi->extents.max_x);
    double vy = p.y - font_size * ((1 - cy) * gi->extents.min_y + cy * gi->extents.max_y);

    double tx = (1 - cx) * min_x + cx * max_x;
    double ty = (1 - cy) * min_y + cy * max_y;

    v[ci].x = (float) vx;
    v[ci].y = (float) vy;
    v[ci].tx = (float) tx;
    v[ci].ty = (float) ty;
    v[ci].band_scale_x = band_scale_x;
    v[ci].band_scale_y = band_scale_y;
    v[ci].band_offset_x = band_offset_x;
    v[ci].band_offset_y = band_offset_y;
    v[ci].atlas_x = gi->atlas_x;
    v[ci].atlas_y = gi->atlas_y;
    v[ci].num_hbands = gi->num_hbands;
    v[ci].num_vbands = gi->num_vbands;
  }

  /* Two triangles */
  vertices->push_back (v[0]);
  vertices->push_back (v[1]);
  vertices->push_back (v[2]);

  vertices->push_back (v[1]);
  vertices->push_back (v[2]);
  vertices->push_back (v[3]);

  if (extents) {
    glyphy_extents_clear (extents);
    for (int i = 0; i < 4; i++) {
      glyphy_point_t pt = {v[i].x, v[i].y};
      glyphy_extents_add (extents, &pt);
    }
  }
}


static GLuint
compile_shader (GLenum         type,
		GLsizei        count,
		const GLchar** sources)
{
  TRACE();

  GLuint shader;
  GLint compiled;

  if (!(shader = glCreateShader (type)))
    return shader;

  glShaderSource (shader, count, sources, 0);
  glCompileShader (shader);

  glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint info_len = 0;
    LOGW ("%s shader failed to compile\n",
	     type == GL_VERTEX_SHADER ? "Vertex" : "Fragment");
    glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 0) {
      char *info_log = (char*) malloc (info_len);
      glGetShaderInfoLog (shader, info_len, NULL, info_log);

      LOGW ("%s\n", info_log);
      free (info_log);
    }

    abort ();
  }

  return shader;
}

static GLuint
link_program (GLuint vshader,
	      GLuint fshader)
{
  TRACE();

  GLuint program;
  GLint linked;

  program = glCreateProgram ();
  glAttachShader (program, vshader);
  glAttachShader (program, fshader);
  glLinkProgram (program);
  glDeleteShader (vshader);
  glDeleteShader (fshader);

  glGetProgramiv (program, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint info_len = 0;
    LOGW ("Program failed to link\n");
    glGetProgramiv (program, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 0) {
      char *info_log = (char*) malloc (info_len);
      glGetProgramInfoLog (program, info_len, NULL, info_log);

      LOGW ("%s\n", info_log);
      free (info_log);
    }

    abort ();
  }

  return program;
}

GLuint
demo_shader_create_program (void)
{
  TRACE();

  GLuint vshader, fshader, program;
  const GLchar *vshader_sources[] = {demo_vshader_glsl};
  vshader = compile_shader (GL_VERTEX_SHADER, ARRAY_LEN (vshader_sources), vshader_sources);
  const GLchar *fshader_sources[] = {"#version 330\n",
				     glyphy_slug_shader_source (),
				     demo_fshader_glsl};
  fshader = compile_shader (GL_FRAGMENT_SHADER, ARRAY_LEN (fshader_sources), fshader_sources);

  program = link_program (vshader, fshader);
  return program;
}
