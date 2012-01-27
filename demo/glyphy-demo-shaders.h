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

#include "glyphy-demo-atlas-glsl.h"
#include "glyphy-demo-vshader-glsl.h"
#include "glyphy-demo-fshader-glsl.h"


#define STRINGIZE1(Src) #Src
#define STRINGIZE(Src) STRINGIZE1(Src)
#define ARRAY_LEN(Array) (sizeof (Array) / sizeof (*Array))

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
