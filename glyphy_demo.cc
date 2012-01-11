#include "glyphy.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include "freetype-helper.hh"

using namespace GLyphy;

static void
die (const char *msg)
{
  fprintf (stderr, "%s\n", msg);
  exit (1);
}

GLuint
compile_shader (GLenum type, const GLchar* source)
{
  GLuint shader;
  GLint compiled;

  if (!(shader = glCreateShader(type)))
    return shader;

  glShaderSource (shader, 1, &source, 0);
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

#define COMPILE_SHADER1(Type,Src) compile_shader (Type, "#version 120\n" #Src)
#define COMPILE_SHADER(Type,Src) COMPILE_SHADER1(Type,Src)
#define gl(name) \
	for (GLint __ee, __ii = 0; \
	     __ii < 1; \
	     (__ii++, \
	      (__ee = glGetError()) && \
	      (fprintf (stderr, "gl" #name " failed with error %04X on line %d", __ee, __LINE__), abort (), 0))) \
	  gl##name

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

#define GEN_STRING1(Src) #Src
#define GEN_STRING(Src) GEN_STRING1(Src)
GLuint
create_program (void)
{
  GLuint vshader, fshader, program;
  vshader = COMPILE_SHADER (GL_VERTEX_SHADER,
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
  );
  std::string fShaderCode = std::string("#version 120\n") + GEN_STRING(
    uniform sampler2D u_tex;
    uniform ivec3 u_texSize;
    varying vec4 v_glyph;

    int mod (const int a, const int b) { return a - (a / b) * b; }
    int div (const int a, const int b) { return a / b; }
    vec4 tex_1D (const sampler2D tex, ivec2 offset, int i)
    {
      vec2 orig = offset;
      return texture2D (tex, vec2 ((orig.x + mod (i, u_texSize.z) + .5) / float (u_texSize.x),
				   (orig.y + div (i, u_texSize.z) + .5) / float (u_texSize.y)));
    }
  );
  std::ifstream fshader_file("fragment_shader.glsl");
  std::stringstream buff;
  buff << fshader_file.rdbuf();
  fShaderCode += buff.str();
  fShaderCode += GEN_STRING(
    void main()
    {
      gl_FragColor = fragment_color(v_glyph.xy);
    }
  );
  fshader = compile_shader(GL_FRAGMENT_SHADER,
			    fShaderCode.c_str());

  program = link_program (vshader, fshader);
  return program;
}


static int step_timer;
static int num_frames;
static bool animate = false;

void display( void )
{
  int viewport[4];
  glGetIntegerv (GL_VIEWPORT, viewport);
  GLuint width  = viewport[2];
  GLuint height = viewport[3];

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

#define TEX_W 512
#define TEX_H 512
#define SUB_TEX_W 64

GLint
create_texture (const char *font_path, const char UTF8)
{
  FT_Face face;
  FT_Library library;
  FT_Init_FreeType (&library);   
  FT_New_Face ( library, font_path, 0, &face );

  unsigned int upem = face->units_per_EM;
  unsigned int glyph_index = FT_Get_Char_Index (face, (FT_ULong) UTF8);

  FT_Outline * outline = face_to_outline(face, glyph_index);

  int tex_w = SUB_TEX_W, tex_h;
  void *buffer;

  generate_texture(upem, &face->glyph->outline, tex_w, &tex_h, &buffer);

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
