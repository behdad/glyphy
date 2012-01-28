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

#ifndef GLYPHY_DEMO_GLUT_H
#define GLYPHY_DEMO_GLUT_H

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <GL/glew.h>

#if defined(__APPLE__)
    #include <Glut/glut.h>
#else
    #include <GL/glut.h>
#endif

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/time.h>


static int num_frames = 0;
static int animate = 0;
static long fps_start_time = 0;
static long last_time = 0;
static double phase = 0;


/* return current time in milli-seconds */
static long
current_time (void)
{
   struct timeval tv;
   struct timezone tz;
   (void) gettimeofday(&tv, &tz);
   return (long) tv.tv_sec * 1000 + (long) tv.tv_usec / 1000;
}


void
glut_reshape_func (int width, int height)
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
    glutPostRedisplay ();
  }
}

static void
idle_step (void)
{
  if (animate) {
    glutIdleFunc (idle_step);
    num_frames++;
    glutPostRedisplay ();
  }
}

static bool has_fps_timer = false;

static void
print_fps (int ms)
{
  if (animate) {
    glutTimerFunc (ms, print_fps, ms);
    long t = current_time ();
    printf ("%gfps\n", num_frames * 1000. / (t - fps_start_time));
    num_frames = 0;
    fps_start_time = t;
  } else
    has_fps_timer = false;
}

static void
start_animation (void)
{
  num_frames = 0;
  fps_start_time = current_time ();
  //glutTimerFunc (1000/60, timed_step, 1000/60);
  glutIdleFunc (idle_step);
  if (!has_fps_timer) {
    has_fps_timer = true;
    glutTimerFunc (5000, print_fps, 5000);
  }
}

static void
toggle_animation (void)
{
  last_time = 0;
  animate = !animate;
  if (animate)
    start_animation ();
}

static void
set_uniform (const char *name, double *p, double value)
{
  GLuint program;
  glGetIntegerv (GL_CURRENT_PROGRAM, (GLint *) &program);
  *p = value;
  glUniform1f (glGetUniformLocation (program, name), value);
  printf ("Setting %s to %g\n", name, value);
  glutPostRedisplay ();
}
#define SET_UNIFORM(name, value) set_uniform (#name, &name, value)

/* Uniforms */
static double u_contrast;
static double u_gamma;
static double u_debug;

static void
glut_keyboard_func (unsigned char key, int x, int y)
{
  switch (key) {
    case '\033':
    case 'q':
      exit (0);
      break;

    case ' ':
      toggle_animation ();
      break;

    case 'd':
      SET_UNIFORM (u_debug, 1 - u_debug);
      break;

    case 'a':
      SET_UNIFORM (u_contrast, u_contrast / .9);
      break;
    case 'z':
      SET_UNIFORM (u_contrast, u_contrast * .9);
      break;

    case 'g':
      SET_UNIFORM (u_gamma, u_gamma / .9);
      break;
    case 'b':
      SET_UNIFORM (u_gamma, u_gamma * .9);
      break;
  }
}

static void
glut_display_func (void)
{
//  int viewport[4];
//  glGetIntegerv (GL_VIEWPORT, viewport);
//  GLuint width  = viewport[2];
//  GLuint height = viewport[3];

  double elapsed_time;
  long t = current_time ();
  if (animate) {
    if (last_time == 0)
      last_time = t;
    elapsed_time = t - last_time;
    last_time = t;
  }
  phase += elapsed_time;

  double theta = M_PI / 360.0 * phase * .05;
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

static void
glut_init (int *argc, char **argv)
{
  glutInit (argc, argv);
  glutInitWindowSize (712, 712);
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("GLyphy");
  glutReshapeFunc (glut_reshape_func);
  glutDisplayFunc (glut_display_func);
  glutKeyboardFunc (glut_keyboard_func);

  glewInit ();
//  if (!glewIsSupported ("GL_VERSION_2_0"))
//    abort ();// die ("OpenGL 2.0 not supported");
}

static void
glut_main (void)
{
  if (animate)
    start_animation ();

  SET_UNIFORM (u_contrast, 1);
  SET_UNIFORM (u_gamma, 2.2);
  SET_UNIFORM (u_debug, 0);

  glutMainLoop ();
}



#endif /* GLYPHY_DEMO_GLUT_H */
