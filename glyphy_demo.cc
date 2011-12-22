#include "glyphy.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

static void
die (const char *msg)
{
  fprintf (stderr, "%s\n", msg);
  exit (1);
}

static int step_timer;
static int num_frames;

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

void keyboard( unsigned char key, int x, int y )
{
  switch (key) {
    case '\033': exit (0);
  }
}

static void
timed_step (int ms)
{
  glutTimerFunc (ms, timed_step, ms);
  num_frames++;
  step_timer++;
  glutPostRedisplay ();
}

static void
idle_step (void)
{
  glutIdleFunc (idle_step);
  num_frames++;
  step_timer++;
  glutPostRedisplay ();
}

static void
print_fps (int ms)
{
  glutTimerFunc (ms, print_fps, ms);
  printf ("%gfps\n", num_frames / 5.);
  num_frames = 0;
}

int
main (int argc, char** argv)
{
  char *font_path;
  char utf8;
  bool animate = false;
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
  };
  const glyph_attrib_t w_vertices[] = {{-1, -1, 0},
				       {+1, -1, 1},
				       {+1, +1, 3},
				       {-1, +1, 2}};

  GLuint a_pos_loc = glGetAttribLocation (program, "a_position");
  GLuint a_glyph_loc = glGetAttribLocation (program, "a_glyph");
  glVertexAttribPointer (a_pos_loc, 2, GL_FLOAT, GL_FALSE, sizeof (glyph_attrib_t),
			 (const char *) w_vertices + offsetof (glyph_attrib_t, x));
  glVertexAttribPointer (a_glyph_loc, 1, GL_FLOAT, GL_FALSE, sizeof (glyph_attrib_t),
			 (const char *) w_vertices + offsetof (glyph_attrib_t, g));
  glEnableVertexAttribArray (a_pos_loc);
  glEnableVertexAttribArray (a_glyph_loc);

  if (animate) {
    //glutTimerFunc (40, timed_step, 40);
    glutIdleFunc (idle_step);
    glutTimerFunc (5000, print_fps, 5000);
  }

  glutMainLoop ();

  return 0;
}
