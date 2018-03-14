#include <stdio.h>
#include <vector>
#include <iostream>

#include "ft2build.h"

#include <glyphy-freetype.h>

#include "default-font.h"

static FT_Int32 GLYPH_LOAD_FLAGS = \
  FT_LOAD_NO_BITMAP |
  FT_LOAD_NO_HINTING |
  FT_LOAD_NO_AUTOHINT |
  FT_LOAD_NO_SCALE |
  FT_LOAD_LINEAR_DESIGN |
  FT_LOAD_IGNORE_TRANSFORM;

static glyphy_bool_t
accumulate_endpoint (glyphy_arc_endpoint_t *endpoint,
                     std::vector<glyphy_arc_endpoint_t> *endpoints)
{
  endpoints->push_back (*endpoint);
  return true;
}


int main() {
  FT_Library ft_library;
  FT_Init_FreeType (&ft_library);
  FT_Face ft_face = NULL;
  FT_New_Face (ft_library, ".", 0, &ft_face);
  FT_New_Memory_Face (ft_library,
                      (const FT_Byte *) default_font,
                      sizeof (default_font),
                      0,
                      &ft_face);
  if (!ft_face) {
    printf("FT_New_Memory_Face failed\n");
  }
  FT_ULong latin_capital_G = 0x47;
  unsigned int glyph_index = FT_Get_Char_Index (ft_face, latin_capital_G);
  if (FT_Err_Ok != FT_Load_Glyph (ft_face, glyph_index, GLYPH_LOAD_FLAGS)) {
    printf("FT_Load_Glyph failed\n");
  }
  double default_tolerance = (1.0 / 2048);
  FT_UShort upem = ft_face->units_per_EM;
  double tolerance = default_tolerance * upem;
  std::vector<glyphy_arc_endpoint_t> endpoints;

  glyphy_arc_accumulator_t* acc = glyphy_arc_accumulator_create();
  glyphy_arc_accumulator_reset (acc);
  glyphy_arc_accumulator_set_tolerance (acc, tolerance);
  glyphy_arc_accumulator_set_callback (
    acc,
    (glyphy_arc_endpoint_accumulator_callback_t) accumulate_endpoint,
    &endpoints
  );

  FT_Error decomposition_result = glyphy_freetype(outline_decompose) (&ft_face->glyph->outline, acc);

  if (FT_Err_Ok != decomposition_result)
    printf("FT_Outline_Decompose failed\n");

  glyphy_outline_winding_from_even_odd (&endpoints[0], endpoints.size (), false);

  glyphy_rgba_t buffer[4096 * 16];
  signed int buffer_len = sizeof (buffer) / sizeof (buffer[0]);
  #define SCALE  (1. * (1 << 0))
  #define MIN_FONT_SIZE 10
  #define M_SQRT2 1.4142135623730951
  double faraway = upem / (MIN_FONT_SIZE * M_SQRT2);
  double avg_fetch_achieved;
	unsigned int output_len;
	unsigned int nominal_width;
	unsigned int nominal_height;
	glyphy_extents_t extents;

  if (!glyphy_arc_list_encode_blob (
    endpoints.size () ? &endpoints[0] : NULL, endpoints.size (),
    buffer,
    buffer_len,
    faraway / SCALE,
    4, // UNUSED
    &avg_fetch_achieved,
    &output_len,
    &nominal_width,
    &nominal_height,
    &extents))
    printf("glyphy_arc_list_encode_blob failed\n");

  glyphy_extents_scale (&extents, 1. / upem, 1. / upem);
  glyphy_extents_scale (&extents, SCALE, SCALE);

  // TODO:
  // create GL program, attach simple pass-through vertex shader and glyphy sdf
  // shader programs, bind buffer send glyphy data to buffer

  /* from demo-buffer.cc+166
  GLint program;
  glGetIntegerv (GL_CURRENT_PROGRAM, &program);
  GLuint a_glyph_vertex_loc = glGetAttribLocation (program, "a_glyph_vertex");
  glBindBuffer (GL_ARRAY_BUFFER, buffer->buf_name);
  if (buffer->dirty) {
    glBufferData (GL_ARRAY_BUFFER,  sizeof (glyph_vertex_t) * buffer->vertices->size (), (const char *) &(*buffer->vertices)[0], GL_STATIC_DRAW);
    buffer->dirty = false;
  }
  glEnableVertexAttribArray (a_glyph_vertex_loc);
  glVertexAttribPointer (a_glyph_vertex_loc, 4, GL_FLOAT, GL_FALSE, sizeof (glyph_vertex_t), 0);
  glDrawArrays (GL_TRIANGLES, 0, buffer->vertices->size ());
  glDisableVertexAttribArray (a_glyph_vertex_loc);
  */

  return 0;
}
