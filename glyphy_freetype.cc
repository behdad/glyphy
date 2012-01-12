#include "freetype-helper.hh"

#include "glyphy.h"
#include "geometry.hh"
#include "bezier-arc-approximation.hh"

namespace GLyphy {

using namespace Geometry;
using namespace BezierArcApproximation;

namespace FreeTypeHelper {

FT_Outline *
ft_face_to_outline (FT_Face face, unsigned int glyph_index)
{
  if (FT_Load_Glyph (face,
		     glyph_index,
		     FT_LOAD_NO_BITMAP |
		     FT_LOAD_NO_HINTING |
		     FT_LOAD_NO_AUTOHINT |
		     FT_LOAD_NO_SCALE |
		     FT_LOAD_LINEAR_DESIGN |
		     FT_LOAD_IGNORE_TRANSFORM))
    abort ();

  assert (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE);
  return &face->glyph->outline;
}

void
ft_outline_to_arcs (FT_Outline *outline,
		    double tolerance,
		    std::vector<arc_t> &arcs,
		    double &error)
{
  ArcAccumulator acc(arcs);
  TArcApproximatorOutlineSink outline_arc_approximator (acc.callback,
						       static_cast<void *> (&acc),
						       tolerance);
  FreeTypeOutlineSource<TArcApproximatorOutlineSink>::decompose_outline (outline,
									outline_arc_approximator);
  error = outline_arc_approximator.error;
}

#define TOLERANCE 5e-3

int
ft_outline_to_texture (FT_Outline *outline, unsigned int upem, int width,
		       int *height, void **buffer)
{
  int res = 0;
  double tolerance = upem * TOLERANCE; // in font design units
  std::vector<arc_t> arcs;
  double error;
  ft_outline_to_arcs (outline, tolerance, arcs, error);
  res = arcs_to_texture(arcs, width, height, buffer);
  printf ("Used %d arcs; Approx. err %g; Tolerance %g; Percentage %g. %s\n",
	  (int) arcs.size (), error, tolerance, round (100 * error / tolerance),
	  error <= tolerance ? "PASS" : "FAIL");
  return res;
}

int
ft_face_to_texture (FT_Face face, FT_ULong uni, int width, int *height,
		    void **buffer)
{
  unsigned int upem = face->units_per_EM;
  unsigned int glyph_index = FT_Get_Char_Index (face, uni);
  FT_Outline *outline = ft_face_to_outline(face, glyph_index);
  return ft_outline_to_texture (outline, upem, width, height, buffer);
}

} /* namespace FreeTypeHelper */

} /* namespace GLyphy */
