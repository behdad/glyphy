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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glyphy-demo.h"
#include "demo-buffer.h"
#include "demo-font.h"
#include "demo-state.h"


static FT_Face
open_ft_face (const char   *font_path,
	      unsigned int  face_index)
{
  static FT_Library library;
  if (!library)
    FT_Init_FreeType (&library);
  FT_Face face;
  FT_New_Face (library, font_path, face_index, &face);
  if (!face)
    die ("Failed to open font file");
  return face;
}


demo_state_t st;
demo_buffer_t *buffer;
double scale = 1.0;



void
reshape_func (int width, int height)
{
  glViewport (0, 0, width, height);
  glutPostRedisplay ();
}

static void
set_uniform (const char *name, double *p, double value)
{
  *p = value;
  glUniform1f (glGetUniformLocation (st.program, name), value);
  printf ("Setting %s to %g\n", name, value);
}
#define SET_UNIFORM(name, value) set_uniform (#name, &st.name, value)


static void
keyboard_func (unsigned char key, int x, int y)
{
  switch (key) {
    case '\033':
    case 'q':
      exit (0);
      break;

    case ' ':
      glyphy_demo_animation_toggle ();
      break;

    case 'f':
      glutFullScreen ();
      break;

    case 'd':
      SET_UNIFORM (u_debug, 1 - st.u_debug);
      break;

    case 'a':
      SET_UNIFORM (u_contrast, st.u_contrast / .9);
      break;
    case 'z':
      SET_UNIFORM (u_contrast, st.u_contrast * .9);
      break;

    case 'g':
      SET_UNIFORM (u_gamma, st.u_gamma / .9);
      break;
    case 'b':
      SET_UNIFORM (u_gamma, st.u_gamma * .9);
      break;

    case '=':
      scale /= .9;
      break;
    case '-':
      scale *= .9;
      break;

    default:
      return;
  }

  glutPostRedisplay ();
}


static void
display_func (void)
{
  int viewport[4];
  glGetIntegerv (GL_VIEWPORT, viewport);
  GLuint width  = viewport[2];
  GLuint height = viewport[3];

  double phase = glyphy_demo_animation_get_phase ();

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  // Screen coordinates
  glScaled (2. / width, -2. / height, 1);
  // Global scale
  glScaled (scale, scale, 1);
  // Animation rotate
  glRotated (phase / 1000. * 360 / 10/*seconds*/, 0, 0, 1);
  // Buffer best-fit
  glyphy_extents_t extents;
  demo_buffer_extents (buffer, &extents);
  double s = .9 * std::min (width / (extents.max_x - extents.min_x),
			    height / (extents.max_y - extents.min_y));
  glScaled (s, s, 1);
  // Center buffer
  glTranslated (-(extents.max_x + extents.min_x) / 2.,
		-(extents.max_y + extents.min_y) / 2., 1);


  GLfloat mat[16];
  glGetFloatv (GL_MODELVIEW_MATRIX, mat);
  glUniformMatrix4fv (glGetUniformLocation (st.program, "u_matViewProjection"), 1, GL_FALSE, mat);

  glClearColor (1, 1, 1, 1);
  glClear (GL_COLOR_BUFFER_BIT);

  demo_buffer_draw (buffer, &st);
  glutSwapBuffers ();
}

int
main (int argc, char** argv)
{
  const char *font_path = NULL;
  const char *text = NULL;

  glutInit (&argc, argv);
  glutInitWindowSize (700, 700);
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("GLyphy Demo");
  glutReshapeFunc (reshape_func);
  glutDisplayFunc (display_func);
  glutKeyboardFunc (keyboard_func);

  glewInit ();
//  if (!glewIsSupported ("GL_VERSION_2_0"))
//    abort ();// die ("OpenGL 2.0 not supported");

  if (argc != 3) {
    fprintf (stderr, "Usage: %s FONT_FILE TEXT\n", argv[0]);
    exit (1);
  }
   font_path = argv[1];
   text = argv[2];

  demo_state_init (&st);
  FT_Face face = open_ft_face (font_path, 0);
  demo_font_t *font = demo_font_create (face, st.atlas);

  glyphy_point_t top_left = {0, 0};
  buffer = demo_buffer_create ();
  demo_buffer_move_to (buffer, top_left);
  demo_buffer_add_text (buffer, text, font, 1, top_left);

  demo_state_setup (&st);
  glutMainLoop ();

  demo_buffer_destroy (buffer);
  demo_font_destroy (font);
  demo_state_fini (&st);

  return 0;
}
