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
 * Google Author(s): Behdad Esfahbod
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glyphy-demo.h"

#include <sys/time.h>

static int num_frames = 0;
static int animate = 0;
static long fps_start_time = 0;
static long last_time = 0;
static int phase = 0;
static bool has_fps_timer = false;



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
    phase++;
    glutPostRedisplay ();
  }
}

static void
idle_step (void)
{
  if (animate) {
    glutIdleFunc (idle_step);
    num_frames++;
    phase++;
    glutPostRedisplay ();
  }
}

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

void
glyphy_demo_animation_toggle (void)
{
  last_time = 0;
  animate = !animate;
  if (animate)
    start_animation ();
}

double
glyphy_demo_animation_get_phase (void)
{
  double elapsed_time = 0;
  long t = current_time ();
  return phase * 1000  / 60.;
}
