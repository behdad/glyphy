/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "demo-shader.h"

#include "demo-vertex-glsl.h"
#include "demo-fragment-glsl.h"


void
demo_shader_add_glyph_vertices (const glyphy_point_t        &p,
				double                       font_size,
				glyph_info_t                *gi,
				std::vector<glyph_vertex_t> *vertices,
				glyphy_extents_t            *extents)
{
  if (gi->is_empty)
    return;

  /* Extents and texcoords are in font design units.
   * Screen position uses font_size / upem as the scale. */
  double scale = font_size / gi->upem;

  glyph_vertex_t v[4];

  for (int ci = 0; ci < 4; ci++) {
    int cx = (ci >> 1) & 1;
    int cy = ci & 1;

    double ex = (1 - cx) * gi->extents.min_x + cx * gi->extents.max_x;
    double ey = (1 - cy) * gi->extents.min_y + cy * gi->extents.max_y;

    v[ci].x = (float) (p.x + scale * ex);
    v[ci].y = (float) (p.y - scale * ey);
    v[ci].tx = (float) ex;
    v[ci].ty = (float) ey;
    v[ci].cx = cx ? 1.f : -1.f;
    v[ci].cy = cy ? 1.f : -1.f; /* top dilates up, bottom dilates down */
    v[ci].tpx = (float) (1.0 / scale);
    v[ci].tpy = (float) (-1.0 / scale);
    v[ci].atlas_offset = gi->atlas_offset;
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
link_program (GLuint vertex_shader,
	      GLuint fragment_shader)
{
  GLuint program;
  GLint linked;

  program = glCreateProgram ();
  glAttachShader (program, vertex_shader);
  glAttachShader (program, fragment_shader);
  glLinkProgram (program);
  glDeleteShader (vertex_shader);
  glDeleteShader (fragment_shader);

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
  GLuint vertex_shader, fragment_shader, program;
  const GLchar *vertex_shader_sources[] = {"#version 330\n",
					   glyphy_vertex_shader_source (),
					   demo_vertex_glsl};
  vertex_shader = compile_shader (GL_VERTEX_SHADER,
				  ARRAY_LEN (vertex_shader_sources),
				  vertex_shader_sources);
  const GLchar *fragment_shader_sources[] = {"#version 330\n",
					     glyphy_fragment_shader_source (),
					     demo_fragment_glsl};
  fragment_shader = compile_shader (GL_FRAGMENT_SHADER,
				    ARRAY_LEN (fragment_shader_sources),
				    fragment_shader_sources);

  program = link_program (vertex_shader, fragment_shader);
  return program;
}
