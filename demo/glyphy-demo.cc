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

#ifndef _WIN32
#include <libgen.h>
#include <unistd.h>
#endif
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


#include "demo-buffer.h"
#include "demo-font.h"
#include "demo-view.h"

static demo_glstate_t *st;
static demo_view_t *vu;
static demo_buffer_t *buffer;

#define WINDOW_W 700
#define WINDOW_H 700

#ifdef _WIN32

static int isroot(const char *path)
{
  return ((strlen(path) == 1 && path[0] == '/') ||
	  (strlen(path) == 3 && isalpha(path[0]) && path[1] == ':' && (path[2] == '/' || path[2] == '\\')));
}

static char *basename(char *path)
{
  if (path == NULL || *path == '\0')
    return ".";

  while ((path[strlen(path)-1] == '/' ||
	  path[strlen(path)-1] == '\\') &&
	 !isroot(path))
    path[strlen(path)-1] = '\0';

  if (isroot(path))
    return path;

  char *slash = strrchr(path, '/');
  char *backslash = strrchr(path, '\\');

  if (slash != NULL && (backslash == NULL || backslash < slash))
    return slash + 1;
  else if (backslash != NULL && (slash == NULL || slash < backslash))
    return backslash + 1;
  else
    return path;
}

static int opterr = 1;
static int optind = 1;
static int optopt;
static char *optarg;

static int getopt(int argc, char *argv[], char *opts)
{
    static int sp = 1;
    int c;
    char *cp;

    if (sp == 1) {
        if (optind >= argc ||
            argv[optind][0] != '-' || argv[optind][1] == '\0')
            return EOF;
        else if (!strcmp(argv[optind], "--")) {
            optind++;
            return EOF;
        }
    }
    optopt = c = argv[optind][sp];
    if (c == ':' || !(cp = strchr(opts, c))) {
        fprintf(stderr, ": illegal option -- %c\n", c);
        if (argv[optind][++sp] == '\0') {
            optind++;
            sp = 1;
        }
        return '?';
    }
    if (*++cp == ':') {
        if (argv[optind][sp+1] != '\0')
            optarg = &argv[optind++][sp+1];
        else if(++optind >= argc) {
            fprintf(stderr, ": option requires an argument -- %c\n", c);
            sp = 1;
            return '?';
        } else
            optarg = argv[optind++];
        sp = 1;
    } else {
        if (argv[optind][++sp] == '\0') {
            sp = 1;
            optind++;
        }
        optarg = NULL;
    }

    return c;
}

#endif

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

static void
show_usage(const char *path)
{

  char *name, *p = strdup(path);

  name = basename(p);

  printf("Usage:\n"
	 "  %s [fontfile [text]]\n"
	 "or:\n"
	 "  %s [-h] [-f fontfile] [-t text]\n"
	 "\n"
	 "  -h             show this help message and exit;\n"
	 "  -t text        the text string to be rendered;     \n"
	 "  -f fontfile    the font file (e.g. /Library/Fonts/Microsoft/Verdana.ttf)\n"
	 "\n", name, name);

  free(p);
}

int
main (int argc, char** argv)
{
  /* Process received parameters */
#   include "default-text.h"
  const char *text = NULL;
  const char *font_path = NULL;
  char arg;
  while ((arg = getopt(argc, argv, "t:f:h")) != -1) {
    switch (arg) {
    case 't':
      text = optarg;
      break;
    case 'f':
      font_path = optarg;
      break;
    case 'h':
      show_usage(argv[0]);
      return 0;
    default:
      return 1;
    }
  }
  if (!font_path)
  {
    if (optind < argc)
      font_path = argv[optind++];
  }
  if (!text)
  {
    if (optind < argc)
      text = argv[optind++];
    else
      text = default_text;
  }
  if (!text || optind < argc)
  {
    show_usage(argv[0]);
    return 1;
  }

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

  st = demo_glstate_create ();
  vu = demo_view_create (st);
  demo_view_print_help (vu);

  FT_Library ft_library;
  FT_Init_FreeType (&ft_library);
  FT_Face ft_face = NULL;
  if (font_path)
  {
    FT_New_Face (ft_library, font_path, 0/*face_index*/, &ft_face);
  }
  else
  {
#ifdef _WIN32
    FT_New_Face(ft_library, "C:\\Windows\\Fonts\\calibri.ttf", 0/*face_index*/, &ft_face);
#else
    #include "default-font.h"
    FT_New_Memory_Face (ft_library, (const FT_Byte *) default_font, sizeof (default_font), 0/*face_index*/, &ft_face);
#endif
  }
  if (!ft_face)
    die ("Failed to open font file");
  demo_font_t *font = demo_font_create (ft_face, demo_glstate_get_atlas (st));

  buffer = demo_buffer_create ();
  glyphy_point_t top_left = {0, 0};
  demo_buffer_move_to (buffer, &top_left);
  demo_buffer_add_text (buffer, text, font, 1);

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
