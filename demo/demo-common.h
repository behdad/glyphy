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
#  define HAVE_GLUT 1
#  define HAVE_FREETYPE2 1
#endif

#include <GL/glew.h>

#ifdef __APPLE__
#  define GL_SILENCE_DEPRECATION
#endif

#if defined(HAVE_GL)
#  if defined(__APPLE__)
#    include <OpenGL/OpenGL.h>
#  else
#    ifdef _WIN32
#      include <GL/wglew.h>
#    else
#      include <GL/glxew.h>
#    endif
#  endif
#endif /* HAVE_GL */

/* Finally, Glut. */
#ifdef HAVE_GLUT
#  if defined(__APPLE__)
#    include <GLUT/glut.h>
#  else
#    include <GL/glut.h>
#  endif
#endif




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


#if defined(_MSC_VER)
#define DEMO_FUNC __FUNCSIG__
#else
#define DEMO_FUNC __func__
#endif

struct auto_trace_t
{
  auto_trace_t (const char *func_) : func (func_)
  { printf ("Enter: %s\n", func); }

  ~auto_trace_t (void)
  { printf ("Leave: %s\n", func); }

  private:
  const char * const func;
};

#define TRACE() auto_trace_t trace(DEMO_FUNC)

#endif /* DEMO_COMMON_H */
