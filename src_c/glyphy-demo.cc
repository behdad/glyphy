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
 * Google Author(s): Behdad Esfahbod, Maysum Panju, Wojciech Baranowski
 */

#include <glyphy.h>

#include "glyphy-geometry.hh"
#include "glyphy-arcs-bezier.hh"

#include <assert.h>

#include <GL/glew.h>
#if defined(__APPLE__)
    #include <Glut/glut.h>
#else
    #include <GL/glut.h>
#endif

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H


#if 1
// Large font size profile
#define MIN_FONT_SIZE 64
#define TOLERANCE 5e-4
#else
// Small font size profile
#define MIN_FONT_SIZE 20
#define TOLERANCE 5e-3
#endif


using namespace GLyphy::Geometry;
using namespace GLyphy::ArcsBezier;


typedef Arc arc_t;

static void
die (const char *msg)
{
  fprintf (stderr, "%s\n", msg);
  exit (1);
}

GLuint
compile_shader (GLenum type, GLsizei count, const GLchar** sources)
{
  GLuint shader;
  GLint compiled;

  if (!(shader = glCreateShader(type)))
    return shader;

  glShaderSource (shader, count, sources, 0);
  glCompileShader (shader);

  glGetShaderiv (shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    GLint info_len = 0;
    fprintf (stderr, "Shader failed to compile\n");
    glGetShaderiv (shader, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 0) {
      char *info_log = (char*) malloc (info_len);
      glGetShaderInfoLog (shader, info_len, NULL, info_log);

      fprintf (stderr, "%s\n", info_log);
      free (info_log);
    }

    abort ();
  }

  return shader;
}

GLuint
link_program (GLuint vshader, GLuint fshader)
{
  GLuint program;
  GLint linked;

  program = glCreateProgram();
  glAttachShader(program, vshader);
  glAttachShader(program, fshader);
  glLinkProgram(program);

  glGetProgramiv (program, GL_LINK_STATUS, &linked);
  if (!linked) {
    GLint info_len = 0;
    fprintf (stderr, "Program failed to link\n");
    glGetProgramiv (program, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 0) {
      char *info_log = (char*) malloc (info_len);
      glGetProgramInfoLog (program, info_len, NULL, info_log);

      fprintf (stderr, "%s\n", info_log);
      free (info_log);
    }

    abort ();
  }

  return program;
}

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


static int step_timer;
static int num_frames;
static bool animate = false;

void display( void )
{
//  int viewport[4];
//  glGetIntegerv (GL_VIEWPORT, viewport);
//  GLuint width  = viewport[2];
//  GLuint height = viewport[3];

  double theta = M_PI / 360.0 * step_timer / 3.;
  GLfloat mat[] = { +cos(theta), +sin(theta), 0., 0.,
		    -sin(theta), +cos(theta), 0., 0.,
			     0.,          0., 1., 0.,
			     0.,          0., 0., 1., };

  glClearColor (0, 0, 0, 1);
  glClear (GL_COLOR_BUFFER_BIT);

  GLuint program;
  glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *) &program);
  glUniformMatrix4fv (glGetUniformLocation (program, "u_matViewProjection"), 1, GL_FALSE, mat);

  glDrawArrays (GL_TRIANGLE_FAN, 0, 4);

  glutSwapBuffers ();
}


template <typename OutlineSink>
class FreeTypeOutlineSource
{
  public:

  static bool decompose_outline (FT_Outline *outline, OutlineSink &d)
  {
    static const FT_Outline_Funcs outline_funcs = {
        (FT_Outline_MoveToFunc) move_to,
        (FT_Outline_LineToFunc) line_to,
        (FT_Outline_ConicToFunc) conic_to,
        (FT_Outline_CubicToFunc) cubic_to,
        0, /* shift */
        0, /* delta */
    };

    return !FT_Outline_Decompose (const_cast <FT_Outline *> (outline), &outline_funcs, &d);
  }

  static const Point point (FT_Vector *to)
  {
    return Point (to->x, to->y);
  }

  static FT_Error err (bool success)
  {
    return success ? FT_Err_Ok : FT_Err_Out_Of_Memory;
  }

  static int
  move_to (FT_Vector *to, OutlineSink *d)
  {
    return err (d->move_to (point (to)));
  }

  static int
  line_to (FT_Vector *to, OutlineSink *d)
  {
    return err (d->line_to (point (to)));
  }

  static int
  conic_to (FT_Vector *control, FT_Vector *to, OutlineSink *d)
  {
    return err (d->conic_to (point (control), point (to)));
  }

  static int
  cubic_to (FT_Vector *control1, FT_Vector *control2,
	    FT_Vector *to, OutlineSink *d)
  {
    return err (d->cubic_to (point (control1), point (control2), point (to)));
  }
};

void
ft_outline_to_arcs (FT_Outline *outline,
		    double tolerance,
		    std::vector<arc_t> &arcs,
		    double &error)
{
  ArcAccumulator acc(arcs);
  ArcApproximatorOutlineSinkDefault outline_arc_approximator (acc.callback,
							      static_cast<void *> (&acc),
							      tolerance);
  FreeTypeOutlineSource<ArcApproximatorOutlineSinkDefault>::decompose_outline (outline,
									       outline_arc_approximator);
  error = outline_arc_approximator.error;
}

namespace GLyphy {
extern int
arcs_to_texture (std::vector<arc_t> &arcs,
		 double min_font_size,
		 int width, int *height,
		 void **buffer);
}

int
ft_outline_to_texture (FT_Outline *outline, unsigned int upem, int width,
		       int *height, void **buffer)
{
  int res = 0;
  double tolerance = upem * TOLERANCE; // in font design units
  std::vector<arc_t> arcs;
  double error;
  ft_outline_to_arcs (outline, tolerance, arcs, error);
  res = GLyphy::arcs_to_texture(arcs, MIN_FONT_SIZE, width, height, buffer);
  printf ("Used %d arcs; Approx. err %g; Tolerance %g; Percentage %g. %s\n",
	  (int) arcs.size (), error, tolerance, round (100 * error / tolerance),
	  error <= tolerance ? "PASS" : "FAIL");
  return res;
}

int
ft_face_to_texture (FT_Face face, FT_ULong unicode, int width, int *height,
		    void **buffer)
{
  unsigned int upem = face->units_per_EM;
  unsigned int glyph_index = FT_Get_Char_Index (face, unicode);

  if (FT_Load_Glyph (face,
		     glyph_index,
		     FT_LOAD_NO_BITMAP |
		     FT_LOAD_NO_HINTING |
		     FT_LOAD_NO_AUTOHINT |
		     FT_LOAD_NO_SCALE |
		     FT_LOAD_LINEAR_DESIGN |
		     FT_LOAD_IGNORE_TRANSFORM))
    die ("Failed loading FreeType glyph");

  if (face->glyph->format != FT_GLYPH_FORMAT_OUTLINE)
    die ("FreeType loaded glyph format is not outline");

  return ft_outline_to_texture (&face->glyph->outline, upem, width, height, buffer);
}




#define TEX_W 512
#define TEX_H 512
#define SUB_TEX_W 64

#define gl(name) \
	for (GLint __ee, __ii = 0; \
	     __ii < 1; \
	     (__ii++, \
	      (__ee = glGetError()) && \
	      (fprintf (stderr, "gl" #name " failed with error %04X on line %d", __ee, __LINE__), abort (), 0))) \
	  gl##name

GLint
create_texture (const char *font_path, const char UTF8)
{
  FT_Face face;
  FT_Library library;
  FT_Init_FreeType (&library);
  FT_New_Face (library, font_path, 0, &face);

  int tex_w = SUB_TEX_W, tex_h;
  void *buffer;

  ft_face_to_texture (face, UTF8, tex_w, &tex_h, &buffer);

  GLuint texture;
  glGenTextures (1, &texture);
  glBindTexture (GL_TEXTURE_2D, texture);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
  glTexParameterf (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

  /* Upload*/
  gl(TexImage2D) (GL_TEXTURE_2D, 0, GL_RGBA, TEX_W, TEX_H, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
  gl(TexSubImage2D) (GL_TEXTURE_2D, 0, 0, 0, tex_w, tex_h, GL_RGBA, GL_UNSIGNED_BYTE, buffer);
  free(buffer);

  GLuint program;
  glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *) &program);
  glUniform3i (glGetUniformLocation(program, "u_texSize"), TEX_W, TEX_H, SUB_TEX_W);
  glUniform1i (glGetUniformLocation(program, "u_tex"), 0);
  glActiveTexture (GL_TEXTURE0);

  return texture;
}

void
reshape (int width, int height)
{
  glViewport (0, 0, width, height);
  glMatrixMode (GL_PROJECTION);
  glLoadIdentity ();
  glOrtho (0, width, 0, height, -1, 1);
  glMatrixMode (GL_MODELVIEW);
  glutPostRedisplay ();
}

static void
timed_step (int ms)
{
  if (animate) {
    glutTimerFunc (ms, timed_step, ms);
    num_frames++;
    step_timer++;
    glutPostRedisplay ();
  }
}

static void
idle_step (void)
{
  if (animate) {
    glutIdleFunc (idle_step);
    num_frames++;
    step_timer++;
    glutPostRedisplay ();
  }
}

static void
print_fps (int ms)
{
  if (animate) {
    glutTimerFunc (ms, print_fps, ms);
    printf ("%gfps\n", num_frames / 5.);
    num_frames = 0;
  }
}

void
start_animation (void)
{
  num_frames = 0;
  //glutTimerFunc (40, timed_step, 40);
  glutIdleFunc (idle_step);
  glutTimerFunc (5000, print_fps, 5000);
}

void keyboard( unsigned char key, int x, int y )
{
  switch (key) {
    case '\033':
    case 'q':
      exit (0);
      break;

    case '\040':
      animate = !animate;
      if (animate)
        start_animation ();
      break;
  }
}

int
main (int argc, char** argv)
{
  char *font_path;
  char utf8;
  if (argc >= 3) {
     font_path = argv[1];
     utf8 = argv[2][0];
     if (argc >= 4)
       animate = atoi (argv[3]);
  }
  else
    die ("Usage: grid PATH_TO_FONT_FILE CHARACTER_TO_DRAW ANIMATE?");

  glutInit (&argc, argv);
  glutInitWindowSize (712, 712);
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("GLyphy");
  glutReshapeFunc (reshape);
  glutDisplayFunc (display);
  glutKeyboardFunc (keyboard);

  glewInit ();
  if (!glewIsSupported ("GL_VERSION_2_0"))
    die ("OpenGL 2.0 not supported");


  GLuint program = create_program ();
  glUseProgram (program);

  GLuint texture = create_texture (font_path, utf8);

  struct glyph_attrib_t {
    GLfloat x;
    GLfloat y;
    GLfloat g;
    GLfloat h;
  };
  const glyph_attrib_t w_vertices[] = {{-1, -1, 0, 0},
				       {+1, -1, 1, 0},
				       {+1, +1, 1, 1},
				       {-1, +1, 0, 1}};

  GLuint a_pos_loc = glGetAttribLocation (program, "a_position");
  GLuint a_glyph_loc = glGetAttribLocation (program, "a_glyph");
  glVertexAttribPointer (a_pos_loc, 2, GL_FLOAT, GL_FALSE, sizeof (glyph_attrib_t),
			 (const char *) w_vertices + offsetof (glyph_attrib_t, x));
  glVertexAttribPointer (a_glyph_loc, 2, GL_FLOAT, GL_FALSE, sizeof (glyph_attrib_t),
			 (const char *) w_vertices + offsetof (glyph_attrib_t, g));
  glEnableVertexAttribArray (a_pos_loc);
  glEnableVertexAttribArray (a_glyph_loc);

  if (animate)
    start_animation ();

  glutMainLoop ();

  return 0;
}
