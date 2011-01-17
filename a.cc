#include <X11/Xlib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>

#include <GLES2/gl2.h>
#define GL_GLEXT_PROTOTYPES
#include <GLES2/gl2ext.h>
#include <EGL/egl.h>
#define EGL_EGLEXT_PROTOTYPES
#include <EGL/eglext.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// Creates a EGLContext backed by a window.
void CreateWindowContext(Display* dpy, int width, int height,
                         XID window, EGLSurface* surface,
                         EGLContext* context) {
  EGLDisplay edpy = eglGetDisplay(dpy);

  const EGLint attribs[] = {
    EGL_BUFFER_SIZE, 32,
    EGL_ALPHA_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_DEPTH_SIZE, 16,
    EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_NONE };

  EGLConfig econfig;
  EGLint num_configs;
  if (!eglChooseConfig(edpy, attribs, &econfig, 1, &num_configs)) {
    fprintf(stderr,"Couldn't find config.\n");
    *surface = *context = NULL;
    return;
  }
  if (!num_configs) {
    fprintf(stderr,"Didn't find config.\n");
    *surface = *context = NULL;
    return;
  }

  if (!(*surface = eglCreateWindowSurface(edpy, econfig, window, NULL))) {
    fprintf(stderr,"Couldn't create window surface.\n");
    *surface = *context = NULL;
    return;
  }

  EGLint ctx_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  *context = eglCreateContext(
      edpy, econfig, EGL_NO_CONTEXT, ctx_attribs);

  eglMakeCurrent(edpy, *surface, *surface, *context);
}

// Create a pixmap with the same depth as the given window.
XID CreatePixmap(Display* dpy, XID window, int width, int height) {
  XWindowAttributes gwa;
  XGetWindowAttributes(dpy, window, &gwa);
  return XCreatePixmap(dpy, window, width, height, gwa.depth);
}

// Creates a EGLContext backed by a pixmap.
void CreatePixmapContext(Display* dpy, int width, int height,
                         XID* pixmap, EGLSurface* surface,
                         EGLContext* context) {
  *pixmap = CreatePixmap(dpy, RootWindow(dpy, 0), width, height);
  EGLDisplay edpy = eglGetDisplay(dpy);

  const EGLint attribs[] = {
    EGL_BUFFER_SIZE, 32,
    EGL_ALPHA_SIZE, 8,
    EGL_RED_SIZE, 8,
    EGL_GREEN_SIZE, 8,
    EGL_BLUE_SIZE, 8,
    EGL_DEPTH_SIZE, 16,
    EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
    EGL_SURFACE_TYPE, EGL_PIXMAP_BIT,
    EGL_NONE };

  EGLConfig econfig;
  EGLint num_configs;
  if (!eglChooseConfig(edpy, attribs, &econfig, 1, &num_configs)) {
    fprintf(stderr,"Couldn't find config.\n");
    *surface = *context = NULL;
    return;
  }
  if (!num_configs) {
    fprintf(stderr,"Didn't find config.\n");
    *surface = *context = NULL;
    return;
  }

  *surface = eglCreateWindowSurface(edpy, econfig, *pixmap, NULL);
  if (!*surface) {
    fprintf(stderr,"Couldn't create pixmap surface.\n");
    *surface = *context = NULL;
    return;
  }

  EGLint ctx_attribs[] = {
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
  };

  *context = eglCreateContext(
      edpy, econfig, EGL_NO_CONTEXT, ctx_attribs);

  eglMakeCurrent(edpy, *surface, *surface, *context);
}

GLuint CompileShader(GLenum type, const GLchar* source) {
  GLuint shader = glCreateShader(type);
  if (!shader)
    return 0;

  glShaderSource(shader, 1, &source, 0);
  glCompileShader(shader);

  GLint compiled;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
  if (!compiled) {
    fprintf(stderr, "Compile error:\n");
    GLint info_len = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);

    if (info_len > 0) {
      char* info_log = (char*) malloc(sizeof(char) * info_len);
      glGetShaderInfoLog(shader, info_len, NULL, info_log);

      fprintf(stderr, "%s\n", info_log);
      free(info_log);
      return 0;
    }
  }
  return shader;
}

int
main(int argc, char** argv)
{
  GtkWidget *window;

  gtk_init (&argc, &argv);

  window = (GtkWidget *) gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (gtk_main_quit), NULL);

  Display* dpy = gdk_x11_display_get_xdisplay (gtk_widget_get_display (window));
  EGLDisplay edpy = eglGetDisplay(dpy);
  EGLint major, minor;
  eglInitialize(edpy, &major, &minor);

  if (!eglBindAPI(EGL_OPENGL_ES_API)) {
    fprintf(stderr, "failed to bind EGL_OPENGL_ES_API\n");
    return 0;
  }

  gtk_widget_show_all (window);

  int width = 400, height = 300;
    XID pixmap;
  {

    printf("Egl %d.%d\n", major, minor);

    EGLSurface egl_pixmap;
    EGLContext p_context;
    CreatePixmapContext(dpy, width, height,
                        &pixmap, &egl_pixmap, &p_context);

    {
      GLuint p_vert_shader = CompileShader(
          GL_VERTEX_SHADER,
          "attribute vec4 vPosition;"
          "uniform mat4 u_matViewProjection;"
          "void main()"
          "{"
          "   gl_Position = u_matViewProjection * vPosition;"
          "}");

      GLuint p_frag_shader = CompileShader(
          GL_FRAGMENT_SHADER,
          "precision mediump float;"
          "void main()"
          "{"
          "  gl_FragColor = vec4(1.0, 1.0, 0.0, 1.0);"
          "}");

      GLuint p_program = glCreateProgram();
      glAttachShader(p_program, p_vert_shader);
      glAttachShader(p_program, p_frag_shader);
      glLinkProgram(p_program);

      glUseProgram(p_program);
      GLfloat p_vertices[] = { +0.50, +0.00, +0.00,
                             -0.25, +0.43, +0.00,
                             -0.25, -0.43, +0.00 };
      glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, p_vertices);
      glEnableVertexAttribArray(0);

      GLint p_mat_loc = glGetUniformLocation(p_program, "u_matViewProjection");

      for(int i=0; i<1; i++) {
        // Render to texture
        eglMakeCurrent(edpy, egl_pixmap, egl_pixmap, p_context);

        glViewport(0, 0, width, height);
        glClearColor(0.25, 0.25, 0.25, 0.);
        glClear(GL_COLOR_BUFFER_BIT);

        double theta = M_PI / 360.0 * i;
        GLfloat mat[] = { +cos(theta), +sin(theta), 0., 0.,
                          -sin(theta), +cos(theta), 0., 0.,
                                   0.,          0., 1., 0.,
                                  0.,          0., 0., 1., };
        glUniformMatrix4fv(p_mat_loc, 1, GL_FALSE, mat);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        eglSwapBuffers(edpy, egl_pixmap);
      }
    }
  }
  {
      // Create an image wrapper for the pixmap and bind that to a texture
    XID xwindow;
    EGLSurface egl_window;
    EGLContext w_context;
    xwindow = GDK_WINDOW_XID (window->window);

    CreateWindowContext(dpy, width, height,
                        xwindow, &egl_window, &w_context);

    GLuint w_vert_shader = CompileShader(
        GL_VERTEX_SHADER,
        "attribute vec4 a_position;"
        "attribute vec2 a_texCoord;"
        "uniform mat4 u_matViewProjection;"
        "varying vec2 v_texCoord;"
        "void main()"
        "{"
        "  gl_Position = u_matViewProjection * a_position;"
        "  v_texCoord = a_texCoord;"
        "}");

    GLuint w_frag_shader = CompileShader(
        GL_FRAGMENT_SHADER,
        "uniform sampler2D tex;"
        "varying vec2 v_texCoord;"
        "void main()"
        "{"
        "  gl_FragColor = texture2D(tex, v_texCoord);"
        "}");

    GLuint w_program = glCreateProgram();
    glAttachShader(w_program, w_vert_shader);
    glAttachShader(w_program, w_frag_shader);
    glLinkProgram(w_program);

    // Texture from pixmap.
    EGLImageKHR i_pixmap = eglCreateImageKHR(
        edpy, w_context, EGL_NATIVE_PIXMAP_KHR, (void*) pixmap, NULL);
    GLuint pixmap_tex;
    glGenTextures(1, &pixmap_tex);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, pixmap_tex);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glEGLImageTargetTexture2DOES(GL_TEXTURE_2D, i_pixmap);

    // Bind to program.
    glUseProgram(w_program);
    GLint w_tex_loc = glGetUniformLocation(w_program, "tex");
    glUniform1i(w_tex_loc, 0);

    GLint w_mat_loc = glGetUniformLocation(w_program, "u_matViewProjection");

    GLfloat w_vertices[] = { -0.50, -0.50, +0.00,
                             +1.00, +0.00,
                             +0.50, -0.50, +0.00,
                             +0.00, +0.00,
                             +0.50, +0.50, +0.00,
                             +0.00, +1.00,
                             -0.50, +0.50, +0.00,
                             +1.00, +1.00 };

    GLuint w_a_pos_loc = glGetAttribLocation(w_program, "a_position");
    GLuint w_a_tex_loc = glGetAttribLocation(w_program, "a_texCoord");

    glVertexAttribPointer(w_a_pos_loc, 3, GL_FLOAT, 
                          GL_FALSE, 5 * sizeof(GLfloat), w_vertices);
    glVertexAttribPointer(w_a_tex_loc, 2, GL_FLOAT,
                          GL_FALSE, 5 * sizeof(GLfloat), &w_vertices[3]);

    glEnableVertexAttribArray(w_a_pos_loc);
    glEnableVertexAttribArray(w_a_tex_loc);

      eglMakeCurrent(edpy, egl_window, egl_window, w_context);
    for(int i=0; i<10000; i++) {
      double theta = M_PI / 360.0 * i;
      GLfloat mat[] = { +cos(theta), +sin(theta), 0., 0.,
                        -sin(theta), +cos(theta), 0., 0.,
                                 0.,          0., 1., 0.,
                                 0.,          0., 0., 1., };
      // Render to screen

      glViewport(0, 0, width, height);
      glClearColor(0., 0., 0., 0.);
      glClear(GL_COLOR_BUFFER_BIT);

      glUniformMatrix4fv(w_mat_loc, 1, GL_FALSE, mat);

      glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

      eglSwapBuffers(edpy, egl_window);
    }
  }
  //gtk_main ();
}
