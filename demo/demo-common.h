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

#ifndef DEMO_COMMON_H
#define DEMO_COMMON_H

#include <glyphy.h>

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>
#include <assert.h>

#include <algorithm>
#include <vector>


#ifdef EMSCRIPTEN
/* https://github.com/kripken/emscripten/issues/340 */
#undef HAVE_GLEW
/* WebGL shaders are ES2 */
#define GL_ES_VERSION_2_0
#endif

#if defined(HAVE_GL) && defined(HAVE_GLUT)

#ifdef HAVE_GLEW
#  include <GL/glew.h>
#else
#define GLEW_OK 0
   static inline int glewInit (void) { return GLEW_OK; }
   static inline int glewIsSupported (const char *s) { return 0 == strcmp ("GL_VERSION_2_0", s); }
#  define GL_GLEXT_PROTOTYPES 1
#  if defined(__APPLE__)
#    include <OpenGL/gl.h>
#  else
#    include <GL/gl.h>
#  endif
#endif

#if defined(__APPLE__)
#  include <GLUT/glut.h>
#  include <OpenGL/OpenGL.h>
#else
#  ifdef HAVE_GLEW
#    if defined(_WIN32)
#      include <GL/wglew.h>
#    else
#      include <GL/glxew.h>
#    endif
#  endif
#  include <GL/glut.h>
#endif

#endif




/* Logging. */
#ifdef __ANDROID__
#  include <android/log.h>
#  define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "glyphy-demo", __VA_ARGS__))
#  define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "glyphy-demo", __VA_ARGS__))
#  define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "glyphy-demo", __VA_ARGS__))
#else /* !__ANDROID__ */
#  define LOGI(...) ((void) fprintf (stdout, __VA_ARGS__))
#  define LOGW(...) ((void) fprintf (stderr, __VA_ARGS__))
#  define LOGE(...) ((void) fprintf (stderr, __VA_ARGS__), abort ())
#endif



#define STRINGIZE1(Src) #Src
#define STRINGIZE(Src) STRINGIZE1(Src)

#define ARRAY_LEN(Array) (sizeof (Array) / sizeof (*Array))


#if 0
#if 0
// Large font size profile
#define MIN_FONT_SIZE 64
#define TOLERANCE 5e-4
#else
// Small font size profile
#define MIN_FONT_SIZE 20
#define TOLERANCE 5e-4
#endif
#endif

#define MIN_FONT_SIZE 10
#define TOLERANCE (1./2048)


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
