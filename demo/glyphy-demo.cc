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

#include "demo-buffer.h"
#include "demo-font.h"
#include "demo-view.h"

static demo_glstate_t *st;
static demo_view_t *vu;
static demo_buffer_t *buffer;

#define WINDOW_W 700
#define WINDOW_H 700

static void
reshape_func (int width, int height)
{
  demo_view_reshape_func (vu, width, height);
}

static void
keyboard_func (unsigned char key, int x, int y)
{
  demo_view_keyboard_func (vu, key, x, y);
}

static void
special_func (int key, int x, int y)
{
  demo_view_special_func (vu, key, x, y);
}

static void
mouse_func (int button, int state, int x, int y)
{
  demo_view_mouse_func (vu, button, state, x, y);
}

static void
motion_func (int x, int y)
{
  demo_view_motion_func (vu, x, y);
}

static void
display_func (void)
{
  demo_view_display (vu, buffer);
}

int
main (int argc, char** argv)
{
  /* Setup glut */
  glutInit (&argc, argv);
  glutInitWindowSize (WINDOW_W, WINDOW_H);
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);
  int window = glutCreateWindow ("GLyphy Demo");
  glutReshapeFunc (reshape_func);
  glutDisplayFunc (display_func);
  glutKeyboardFunc (keyboard_func);
  glutSpecialFunc (special_func);
  glutMouseFunc (mouse_func);
  glutMotionFunc (motion_func);

  /* Setup glew */
  if (GLEW_OK != glewInit ())
    die ("Failed to initialize GL; something really broken");
  if (!glewIsSupported ("GL_VERSION_2_0"))
    die ("OpenGL 2.0 not supported");

  if ((argc > 1 && 0 == strcmp (argv[1], "--help")) || argc > 3) {
    fprintf (stderr, "Usage: %s [FONTFILE [TEXT]]\n", argv[0]);
    exit (1);
  }

  const char *font_path = NULL;
  if (argc >= 2)
    font_path = argv[1];
  else
    font_path = DEFAULT_FONT;

  const char *text = NULL;
  if (argc >= 3)
    text = argv[2];
  else {
#   include "jabberwocky.h"
    text = jabberwocky;
  }

  st = demo_glstate_create ();
  vu = demo_view_create (st);

  demo_view_print_help (vu);

  FT_Library ft_library;
  FT_Init_FreeType (&ft_library);
  FT_Face ft_face;
#ifdef EMSCRIPTEN
# include "DroidSans.c"
  FT_New_Memory_Face (ft_library, (const FT_Byte *) DroidSans, sizeof (DroidSans), 0/*face_index*/, &ft_face);
#else
  FT_New_Face (ft_library, font_path, 0/*face_index*/, &ft_face);
#endif
  if (!ft_face)
    die ("Failed to open font file");
  demo_font_t *font = demo_font_create (ft_face, demo_glstate_get_atlas (st));

  buffer = demo_buffer_create ();
  glyphy_point_t top_left = {0, 0};
  demo_buffer_move_to (buffer, top_left);
  demo_buffer_add_text (buffer, text, font, 1, top_left);

  demo_font_print_stats (font);

  demo_view_setup (vu);
  glutMainLoop ();

  demo_buffer_destroy (buffer);
  demo_font_destroy (font);

  FT_Done_Face (ft_face);
  FT_Done_FreeType (ft_library);

  demo_view_destroy (vu);
  demo_glstate_destroy (st);

  glutDestroyWindow (window);

  return 0;
}
