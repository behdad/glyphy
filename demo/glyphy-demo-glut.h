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

#include "demo-shader.h"
#include "demo-state.h"

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


#define WINDOW_SIZE 700



/* return current time in milli-seconds */
static long
current_time (void)
{
   struct timeval tv;
   struct timezone tz;
   (void) gettimeofday(&tv, &tz);
   return (long) tv.tv_sec * 1000 + (long) tv.tv_usec / 1000;
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
  glutPostRedisplay ();
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
      toggle_animation ();
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
  }
}


static void
display_func (void)
{
  int viewport[4];
  glGetIntegerv (GL_VIEWPORT, viewport);
  GLuint width  = viewport[2];
  GLuint height = viewport[3];

  double elapsed_time = 0;
  long t = current_time ();
  if (animate) {
    if (last_time == 0)
      last_time = t;
    elapsed_time = t - last_time;
    last_time = t;
  }
  phase += elapsed_time;

  double theta = M_PI / 360.0 * phase * .05;
  GLfloat mat[] = { +cos(theta)*2/width, -sin(theta)*2/height, 0., 0.,
		    -sin(theta)*2/width, -cos(theta)*2/height, 0., 0.,
			     0.,          0., 0., 0.,
			     0.,          0., 0., 1., };

  glClearColor (1, 1, 1, 1);
  glClear (GL_COLOR_BUFFER_BIT);

  glUniformMatrix4fv (glGetUniformLocation (st.program, "u_matViewProjection"), 1, GL_FALSE, mat);

  demo_buffer_draw (buffer, &st);
  glutSwapBuffers ();
}

static void
glut_init (int *argc, char **argv)
{
  glutInit (argc, argv);
  glutInitWindowSize (WINDOW_SIZE, WINDOW_SIZE);
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
  glutCreateWindow("GLyphy Demo");
  glutReshapeFunc (reshape_func);
  glutDisplayFunc (display_func);
  glutKeyboardFunc (keyboard_func);

  glewInit ();
//  if (!glewIsSupported ("GL_VERSION_2_0"))
//    abort ();// die ("OpenGL 2.0 not supported");
}



#endif /* GLYPHY_DEMO_GLUT_H */
