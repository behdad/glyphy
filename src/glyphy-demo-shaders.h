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



#define STRINGIZE1(Src) #Src
#define STRINGIZE(Src) STRINGIZE1(Src)
#define ARRAY_LEN(Array) (sizeof (Array) / sizeof (*Array))

#define GLSL_VERSION_STRING "#version 120\n"

GLuint
create_program (void)
{
  GLuint vshader, fshader, program;
  const GLchar *vshader_sources[] = {GLSL_VERSION_STRING,
				     STRINGIZE
  (
    uniform mat4 u_matViewProjection;
    attribute vec4 a_position;
    attribute vec2 a_glyph;
    varying vec4 v_glyph;

    int mod (const int a, const int b) { return a - (a / b) * b; }
    int div (const int a, const int b) { return a / b; }

    vec4 glyph_decode (vec2 v)
    {
      ivec2 g = ivec2 (int(v.x), int(v.y));
      return vec4 (mod (g.x, 2), mod (g.y, 2), div (g.x, 2), div(g.y, 2));
    }

    void main()
    {
      gl_Position = u_matViewProjection * a_position;
      v_glyph = glyph_decode (a_glyph);
    }
  )};
  vshader = compile_shader (GL_VERTEX_SHADER, ARRAY_LEN (vshader_sources), vshader_sources);
  const GLchar *fshader_sources[] = {GLSL_VERSION_STRING,
				     STRINGIZE
  (
    uniform sampler2D u_tex;
    uniform ivec3 u_texSize;
    varying vec4 v_glyph;

    int mod (const int a, const int b) { return a - (a / b) * b; }
    int div (const int a, const int b) { return a / b; }
    vec4 tex_1D (ivec2 offset, int i)
    {
      vec2 orig = offset;
      return texture2D (u_tex, vec2 ((orig.x + mod (i, u_texSize.z) + .5) / float (u_texSize.x),
				   (orig.y + div (i, u_texSize.z) + .5) / float (u_texSize.y)));
    }
  ),
  glyphy_common_shader_source (),
  glyphy_sdf_shader_source (),
  STRINGIZE
  (

    void main()
    {
      gl_FragColor = glyphy_fragment_color(v_glyph.xy, v_glyph);
    }
  )};
  fshader = compile_shader (GL_FRAGMENT_SHADER, ARRAY_LEN (fshader_sources), fshader_sources);

  program = link_program (vshader, fshader);
  return program;
}



#endif /* GLYPHY_DEMO_SHADERS_H */
