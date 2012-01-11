/*
 * Copyright © 2011  Google, Inc.
 *
 * Permission is hereby granted, without written agreement and without
 * license or royalty fees, to use, copy, modify, and distribute this
 * software and its documentation for any purpose, provided that the
 * above copyright notice and the following two paragraphs appear in
 * all copies of this software.
 *
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES
 * ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN
 * IF THE COPYRIGHT HOLDER HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * THE COPYRIGHT HOLDER SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING,
 * BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE COPYRIGHT HOLDER HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * Google Author(s): Behdad Esfahbod, Maysum Panju
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <cairo-ft.h>

#include "geometry.hh"
#include "cairo-helper.hh"
#include "freetype-helper.hh"
#include "sample-curves.hh"
#include "bezier-arc-approximation.hh"


using namespace GLyphy;
using namespace Geometry;
using namespace CairoHelper;
using namespace FreeTypeHelper;
using namespace SampleCurves;
using namespace BezierArcApproximation;

typedef Vector<Coord> vector_t;
typedef Point<Coord> point_t;
typedef Line<Coord> line_t;
typedef Circle<Coord, Scalar> circle_t;
typedef Arc<Coord, Scalar> arc_t;
typedef Bezier<Coord> bezier_t;


static void
demo_curve (cairo_t *cr, const bezier_t &b)
{
  cairo_save (cr);
  cairo_set_line_width (cr, 5);

  cairo_curve (cr, b);
  cairo_set_viewport (cr);
  cairo_new_path (cr);

  cairo_set_source_rgb (cr, 1, 0, 0);
  cairo_demo_curve (cr, b);

  typedef MaxDeviationApproximatorExact MaxDev;
  typedef BezierArcErrorApproximatorBehdad<MaxDev> BezierArcError;
  typedef BezierArcApproximatorMidpointTwoPart<BezierArcError> BezierArcApproximator;
  typedef BezierArcsApproximatorSpringSystem<BezierArcApproximator> SpringSystem;

  double tolerance = .002;
  double e;
  std::vector<Arc<Coord, Scalar> > &arcs = SpringSystem::approximate_bezier_with_arcs (b, tolerance, &e);

  printf ("Num arcs %d; Approximation error %g; Tolerance %g; Percentage %g; %s\n",
	  (int) arcs.size (), e, tolerance, round (100 * e / tolerance), e <= tolerance ? "PASS" : "FAIL");

  cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, 1.0);
  cairo_set_line_width (cr, cairo_get_line_width (cr) / 2);
  cairo_demo_arcs (cr, arcs);

  delete &arcs;

  cairo_restore (cr);
}

static void
demo_text (cairo_t *cr, const char *family, const char *utf8)
{
  typedef MaxDeviationApproximatorExact MaxDev;
//  typedef BezierArcErrorApproximatorViaBezier<BezierBezierErrorApproximatorSimpleMagnitudeDecomposed<MaxDev> > BezierArcError;
  typedef BezierArcErrorApproximatorBehdad<MaxDev> BezierArcError;
  typedef BezierArcApproximatorMidpointTwoPart<BezierArcError> BezierArcApproximator;
  typedef BezierArcsApproximatorSpringSystem<BezierArcApproximator> SpringSystem;
  typedef ArcApproximatorOutlineSink<SpringSystem> ArcApproximatorOutlineSink;

  double e;

  class ArcAccumulator
  {
    public:

    static bool callback (const arc_t &arc, void *closure)
    {
       ArcAccumulator *acc = static_cast<ArcAccumulator *> (closure);
       acc->arcs.push_back (arc);
       return true;
    }

    std::vector<arc_t> arcs;
  } acc;

  cairo_save (cr);
  cairo_scale (cr, 1, -1);
  cairo_select_font_face (cr, family, CAIRO_FONT_SLANT_NORMAL, CAIRO_FONT_WEIGHT_NORMAL);
  FT_Face face = cairo_ft_scaled_font_lock_face (cairo_get_scaled_font (cr));
  unsigned int upem = face->units_per_EM;
  double tolerance = upem * 3e-4; /* in font design units */
  if (FT_Load_Glyph (face,
		     FT_Get_Char_Index (face, (FT_ULong) *utf8),
		     FT_LOAD_NO_BITMAP |
		     FT_LOAD_NO_HINTING |
		     FT_LOAD_NO_AUTOHINT |
		     FT_LOAD_NO_SCALE |
		     FT_LOAD_LINEAR_DESIGN |
		     FT_LOAD_IGNORE_TRANSFORM))
    abort ();
  assert (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE);

  CairoOutlineSink cairo_sink (cr);
  FreeTypeOutlineSource<CairoOutlineSink>::decompose_outline (&face->glyph->outline, cairo_sink);
  cairo_path_t *path = cairo_copy_path (cr);
  cairo_path_print_stats (path);
  cairo_set_viewport (cr);

  cairo_new_path (cr);
  cairo_append_path (cr, path);
  {
    cairo_save (cr);
    cairo_set_tolerance (cr, tolerance);
    cairo_path_t *path2 = cairo_copy_path_flat (cr);
    cairo_path_print_stats (path2);
    cairo_path_destroy (path2);
    cairo_restore (cr);
  }
  cairo_new_path (cr);
  cairo_append_path (cr, path);
  cairo_path_destroy (path);
  cairo_set_line_width (cr, 5);
  cairo_set_source_rgb (cr, 1, 0, 0);
  cairo_fancy_stroke_preserve (cr);
  cairo_fill (cr);

  ArcApproximatorOutlineSink outline_arc_approximator (acc.callback,
						       static_cast<void *> (&acc),
						       tolerance);
  FreeTypeOutlineSource<ArcApproximatorOutlineSink>::decompose_outline (&face->glyph->outline,
									outline_arc_approximator);

  e = outline_arc_approximator.error;

  printf ("Num arcs %d; Approximation error %g; Tolerance %g; Percentage %g; %s\n",
	  (int) acc.arcs.size (), e, tolerance, round (100 * e / tolerance), e <= tolerance ? "PASS" : "FAIL");

  cairo_set_source_rgba (cr, 0.0, 0.0, 1.0, .5);
  cairo_arcs (cr, acc.arcs);
  cairo_fill (cr);

  cairo_set_source_rgba (cr, 0.0, 1.0, 0.0, .3);
  cairo_set_line_width (cr, 4);
  cairo_demo_arcs (cr, acc.arcs);

  cairo_ft_scaled_font_unlock_face (cairo_get_scaled_font (cr));
  cairo_restore (cr);
}

int main (int argc, char **argv)
{
  cairo_t *cr;
  char *filename;
  cairo_status_t status;
  cairo_surface_t *surface;

  if (argc != 2) {
    fprintf (stderr, "Usage: arc OUTPUT_FILENAME\n");
    return 1;
  }

  filename = argv[1];

  surface = cairo_image_surface_create (CAIRO_FORMAT_ARGB32, 1400, 800);
  cr = cairo_create (surface);

  cairo_set_source_rgb (cr, 1.0, 1.0, 1.0);
  cairo_paint (cr);

//  demo_curve (cr, sample_curve_skewed ());
//  demo_curve (cr, sample_curve_riskus_simple ());
//  demo_curve (cr, sample_curve_riskus_complicated ());
//  demo_curve (cr, sample_curve_s ());
//  demo_curve (cr, sample_curve_serpentine_c_symmetric ());
//  demo_curve (cr, sample_curve_serpentine_s_symmetric ());
//  demo_curve (cr, sample_curve_serpentine_quadratic ());
//  demo_curve (cr, sample_curve_cusp_symmetric ());
//  demo_curve (cr, sample_curve_loop_gamma_symmetric ());
//  demo_curve (cr, sample_curve_loop_gamma_small_symmetric ());
//  demo_curve (cr, sample_curve_loop_o_symmetric ());

  demo_text (cr, "Neuton", "g");
//  demo_text (cr, "Times New Roman", "g");
//  demo_text (cr, "Times New Roman", "@");
//  demo_text (cr, "DejaVu Sans", "@");

  printf("Done.\n");
  cairo_destroy (cr);

  status = cairo_surface_write_to_png (surface, filename);
  cairo_surface_destroy (surface);

  if (status != CAIRO_STATUS_SUCCESS) {
    fprintf (stderr, "Could not save png to '%s'\n", filename);
    return 1;
  }

  return 0;
}
