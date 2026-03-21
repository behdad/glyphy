/*
 * Copyright 2012 Google, Inc. All Rights Reserved.
 *
 * Google Author(s): Behdad Esfahbod, Maysum Panju
 */

#ifndef DEMO_COMMON_H
#define DEMO_COMMON_H

#include <config.h>

#include <glyphy.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include <algorithm>
#include <vector>

#ifdef _WIN32
#  define HAVE_GL 1
#  define HAVE_GLEW 1
#  define HAVE_GLFW 1
#  define HAVE_FREETYPE2 1
#endif

#include <GL/glew.h>

#ifdef __APPLE__
#  define GL_SILENCE_DEPRECATION
#endif

#if defined(HAVE_GL)
#  if defined(__APPLE__)
#    include <OpenGL/OpenGL.h>
#  endif
#endif /* HAVE_GL */

#include <GLFW/glfw3.h>


#define LOGI(...) ((void) fprintf (stdout, __VA_ARGS__))
#define LOGW(...) ((void) fprintf (stderr, __VA_ARGS__))
#define LOGE(...) ((void) fprintf (stderr, __VA_ARGS__), abort ())



#define STRINGIZE1(Src) #Src
#define STRINGIZE(Src) STRINGIZE1(Src)

#define ARRAY_LEN(Array) (sizeof (Array) / sizeof (*Array))




#define gl(name) \
	for (GLint __ee, __ii = 0; \
	     __ii < 1; \
	     (__ii++, \
	      (__ee = glGetError()) && \
	      (fprintf (stderr, "gl" #name " failed with error %04X on line %d\n", __ee, __LINE__), abort (), 0))) \
	  gl##name


static inline void
die (const char *msg)
{
  fprintf (stderr, "%s\n", msg);
  exit (1);
}

template <typename T>
T clamp (T v, T m, T M)
{
  return v < m ? m : v > M ? M : v;
}

#endif /* DEMO_COMMON_H */
