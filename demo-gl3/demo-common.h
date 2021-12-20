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

/* Tailor config for various platforms. */

// first include GL3 header
#if defined(__APPLE__)
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
    #include <OpenGL/OpenGL.h>
#else
    #include <GL/gl3.h>
#endif

// next include GLUT header

#ifdef HAVE_GLUT
    #if defined(__APPLE__)
        #define __gl_h_
        #include <GLUT/glut.h>
        #undef __gl_h_
    #else
        #include <GL/glut.h>
    #endif
#endif

// include glew if we have it
// otherwise emulate needed functionality
#ifdef HAVE_GLEW
#  include <GL/glew.h>
#else
#  define GLEW_OK 0
   static inline int glewInit (void) { return GLEW_OK; }
   static inline int glewIsSupported (const char *s)
   { return 0 == strcmp ("GL_VERSION_2_0", s); }
#endif


#if 0

#ifdef EMSCRIPTEN
/* https://github.com/kripken/emscripten/issues/340 */
#  undef HAVE_GLEW
   /* WebGL shaders are ES2 */
#  define GL_ES_VERSION_2_0 1
#endif

#if defined(__ANDROID__)
#  define HAVE_GLES2 1
#  define HAVE_GLUT 1
#endif

#ifdef _WIN32
#  define HAVE_GL 1
#  define HAVE_GLEW 1
#  define HAVE_GLUT 1
#  define HAVE_FREETYPE2 1
#endif

/* Get Glew out of the way. */
#ifdef HAVE_GLEW
#  include <GL/glew.h>
#else
#  define GLEW_OK 0
   static inline int glewInit (void) { return GLEW_OK; }
   static inline int glewIsSupported (const char *s)
   { return 0 == strcmp ("GL_VERSION_2_0", s); }
#endif /* HAVE_GLEW */

/* WTF this block?! */
#if defined(HAVE_GLES2)
#  include <GLES2/gl2.h>
#elif defined(HAVE_GL)
#  ifndef HAVE_GLEW
#    define GL_GLEXT_PROTOTYPES 1
#    if defined(__APPLE__)
#      include <OpenGL/gl.h>
#    else
#      include <GL/gl.h>
#    endif
#  endif
#  if defined(__APPLE__)
#    include <OpenGL/OpenGL.h>
#  else
#    ifdef HAVE_GLEW
#      ifdef _WIN32
#	 include <GL/wglew.h>
#      else
#	include <GL/glxew.h>
#      endif
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
