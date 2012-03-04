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


typedef struct {
  GLuint program;
  demo_atlas_t *atlas;

  /* Uniforms */
  double u_debug;
  double u_smoothfunc;
  double u_contrast;
  double u_gamma_adjust;

} demo_state_t;

static void
demo_state_init (demo_state_t *st)
{
  st->program = demo_shader_create_program ();
  st->atlas = demo_atlas_create (2048, 1024, 64, 8);

  st->u_debug = false;
  st->u_smoothfunc = 1;
  st->u_contrast = 1.0;
  st->u_gamma_adjust = 1.0;
}

static void
demo_state_fini (demo_state_t *st)
{
  demo_atlas_destroy (st->atlas);
  glDeleteProgram (st->program);
}

static void
set_uniform (GLuint program, const char *name, double *p, double value)
{
  *p = value;
  glUniform1f (glGetUniformLocation (program, name), value);
  printf ("Setting %s to %g\n", name, value);
}
#define SET_UNIFORM(name, value) set_uniform (st->program, #name, &st->name, value)

static void
demo_state_setup (demo_state_t *st)
{
  glUseProgram (st->program);
  demo_atlas_set_uniforms (st->atlas);
  SET_UNIFORM (u_debug, st->u_debug);
  SET_UNIFORM (u_smoothfunc, st->u_smoothfunc);
  SET_UNIFORM (u_contrast, st->u_contrast);
  SET_UNIFORM (u_gamma_adjust, st->u_gamma_adjust);
}

static demo_state_t st[1];
static demo_buffer_t *buffer;
/* Viewer settings */
static double content_scale;
static double view_scale = 1.0;
static glyphy_point_t translate = {0, 0};
static double phase_offset = 0;
static GLint vsync = 0;
static glyphy_bool_t srgb = false;

#define WINDOW_W 700
#define WINDOW_H 700

void
v_sync_set (glyphy_bool_t sync)
{
  vsync = sync ? 1 : 0;
  printf ("Setting vsync %s.\n", vsync ? "on" : "off");
#if defined(__APPLE__)
  CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &sync);
#elif defined(_WIN32)
  if (glewIsSupported ("WGL_EXT_swap_control"))
    wglSwapIntervalEXT (vsync);
  else
    printf ("WGL_EXT_swal_control not supported; failed to set vsync\n");
#else
  if (glxewIsSupported ("GLX_SGI_swap_control"))
    glXSwapIntervalSGI (vsync);
  else
    printf ("GLX_SGI_swap_control not supported; failed to set vsync\n");
#endif
}

void
v_srgb_set (glyphy_bool_t _srgb)
{
  srgb = _srgb;
  printf ("Setting sRGB framebuffer %s.\n", srgb ? "on" : "off");
  if (glewIsSupported ("GL_ARB_framebuffer_sRGB") || glewIsSupported ("GL_EXT_framebuffer_sRGB")) {
    if (srgb)
      glEnable (GL_FRAMEBUFFER_SRGB);
    else
      glDisable (GL_FRAMEBUFFER_SRGB);
  } else
    printf ("No sRGB framebuffer extension found; failed to set sRGB framebuffer\n");

}



void
reshape_func (int width, int height)
{
  glViewport (0, 0, width, height);
  glutPostRedisplay ();
}


#define STEP 1.05
static void
keyboard_func (unsigned char key, int x, int y)
{
  switch (key)
  {
    case '\033':
    case 'q':
      exit (0);
      break;

    case ' ':
      glyphy_demo_animation_toggle ();
      break;
    case 'v':
      v_sync_set (!vsync);
      break;

    case 'f':
      glutFullScreen ();
      break;

    case 'd':
      SET_UNIFORM (u_debug, 1 - st->u_debug);
      break;

    case 's':
      SET_UNIFORM (u_smoothfunc, ((int) st->u_smoothfunc + 1) % 3);
      break;
    case 'a':
      SET_UNIFORM (u_contrast, st->u_contrast * STEP);
      break;
    case 'z':
      SET_UNIFORM (u_contrast, st->u_contrast / STEP);
      break;
    case 'g':
      SET_UNIFORM (u_gamma_adjust, st->u_gamma_adjust * STEP);
      break;
    case 'b':
      SET_UNIFORM (u_gamma_adjust, st->u_gamma_adjust / STEP);
      break;
    case 'c':
      v_srgb_set (!srgb);
      break;

    case '=':
      view_scale *= STEP;
      printf ("Setting scale to %g; font size is %.1f now.\n", view_scale, view_scale * content_scale);
      break;
    case '-':
      view_scale /= STEP;
      printf ("Setting scale to %g; font size is %.2f now.\n", view_scale, view_scale * content_scale);
      break;

    case 'k': 
      translate.y -= .1;
      break;
    case 'j':
      translate.y += .1;
      break;
    case 'h':
      translate.x += .1;
      break;
    case 'l':
      translate.x -= .1;
      break;

    case 'r':
      view_scale = 1.;
      translate.x = translate.y = 0.;
      phase_offset = glyphy_demo_animation_get_phase ();
      glutReshapeWindow (WINDOW_W, WINDOW_H);
      break;

    default:
      return;
  }

  glutPostRedisplay ();
}

static void
special_func (int key, int x, int y)
{
  switch (key)
  {
    case GLUT_KEY_UP:
      translate.y -= .1;
      break;
    case GLUT_KEY_DOWN:
      translate.y += .1;
      break;
    case GLUT_KEY_LEFT:
      translate.x += .1;
      break;
    case GLUT_KEY_RIGHT:
      translate.x -= .1;
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

  double phase = glyphy_demo_animation_get_phase () - phase_offset;

  glMatrixMode (GL_MODELVIEW);
  glLoadIdentity ();
  // Translate translate
  glTranslated (translate.x, translate.y, 0);
  // Screen coordinates
  glScaled (2. / width, -2. / height, 1);
  // View scale
  glScaled (view_scale, view_scale, 1);
  // Animation rotate
  glRotated (phase / 1000. * 360 / 10/*seconds*/, 0, 0, 1);
  // Buffer best-fit
  glyphy_extents_t extents;
  demo_buffer_extents (buffer, &extents);
  content_scale = .9 * std::min (width / (extents.max_x - extents.min_x),
				 height / (extents.max_y - extents.min_y));
  glScaled (content_scale, content_scale, 1);
  // Center buffer
  glTranslated (-(extents.max_x + extents.min_x) / 2.,
		-(extents.max_y + extents.min_y) / 2., 0);


  GLfloat mat[16];
  glGetFloatv (GL_MODELVIEW_MATRIX, mat);
  glUniformMatrix4fv (glGetUniformLocation (st->program, "u_matViewProjection"), 1, GL_FALSE, mat);

  glClearColor (1, 1, 1, 1);
  glClear (GL_COLOR_BUFFER_BIT);

  demo_buffer_draw (buffer);
  glutSwapBuffers ();
}

int
main (int argc, char** argv)
{
  const char *font_path = NULL;
  const char *text = NULL;

  glutInit (&argc, argv);
  glutInitWindowSize (WINDOW_W, WINDOW_H);
  glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB);
  int window = glutCreateWindow ("GLyphy Demo");
  glutReshapeFunc (reshape_func);
  glutDisplayFunc (display_func);
  glutKeyboardFunc (keyboard_func);
  glutSpecialFunc (special_func);

  if (GLEW_OK != glewInit ())
    die ("Failed to initialize GL; something really broken");
  if (!glewIsSupported ("GL_VERSION_2_0"))
    die ("OpenGL 2.0 not supported");

  glEnable (GL_BLEND);
  glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (argc != 3) {
    fprintf (stderr, "Usage: %s FONT_FILE TEXT\n", argv[0]);
    exit (1);
  }
  font_path = argv[1];
  text = argv[2];

  demo_state_init (st);

  FT_Library ft_library;
  FT_Init_FreeType (&ft_library);
  FT_Face ft_face;
  FT_New_Face (ft_library, font_path, 0/*face_index*/, &ft_face);
  if (!ft_face)
    die ("Failed to open font file");

  demo_font_t *font = demo_font_create (ft_face, st->atlas);

  glyphy_point_t top_left = {0, 0};
  buffer = demo_buffer_create ();
  demo_buffer_move_to (buffer, top_left);
  demo_buffer_add_text (buffer, text, font, 1, top_left);
  demo_font_print_stats (font);

  v_sync_set (true);
  v_srgb_set (true);

  demo_state_setup (st);
  glutMainLoop ();

  demo_buffer_destroy (buffer);
  demo_font_destroy (font);

  FT_Done_Face (ft_face);
  FT_Done_FreeType (ft_library);

  demo_state_fini (st);

  glutDestroyWindow (window);

  return 0;
}
