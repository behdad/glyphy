/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
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
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glyphy.h"

#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>


/*
 * Encode quadratic Bezier curves into a blob for GPU rendering.
 *
 * Blob layout (single RGBA16UI texture region):
 *
 *   [H-band headers (num_hbands texels)]
 *   [V-band headers (num_vbands texels)]
 *   [Curve index lists (variable)]
 *   [Curve data (2 texels per curve)]
 *
 * Band header texel:
 *   R = curve count
 *   G = offset to curve index list (from blob start)
 *   B = reserved (split value for symmetric bands, future)
 *   A = reserved
 *
 * Curve index texel:
 *   R = offset to curve data (from blob start)
 *   G = reserved
 *   B = reserved
 *   A = reserved
 *
 * Curve data (2 consecutive texels):
 *   Texel 0: R=p1.x, G=p1.y, B=p2.x, A=p2.y  (int16, em-space * UNITS_PER_EM_UNIT)
 *   Texel 1: R=p3.x, G=p3.y, B=0, A=0
 *
 * All offsets are 1D from blob start. The shader converts to 2D atlas
 * coordinates using the atlas width, similar to Slug's CalcBandLoc.
 */


static int16_t
quantize (double v)
{
  return (int16_t) round (v * GLYPHY_UNITS_PER_EM_UNIT);
}

static double
curve_max_x (const glyphy_curve_t *c)
{
  return fmax (fmax (c->p1.x, c->p2.x), c->p3.x);
}

static double
curve_max_y (const glyphy_curve_t *c)
{
  return fmax (fmax (c->p1.y, c->p2.y), c->p3.y);
}

static void
curve_y_range (const glyphy_curve_t *c, double *min_y, double *max_y)
{
  *min_y = fmin (fmin (c->p1.y, c->p2.y), c->p3.y);
  *max_y = fmax (fmax (c->p1.y, c->p2.y), c->p3.y);
}

static void
curve_x_range (const glyphy_curve_t *c, double *min_x, double *max_x)
{
  *min_x = fmin (fmin (c->p1.x, c->p2.x), c->p3.x);
  *max_x = fmax (fmax (c->p1.x, c->p2.x), c->p3.x);
}

static bool
curve_is_horizontal (const glyphy_curve_t *c)
{
  return c->p1.y == c->p2.y && c->p2.y == c->p3.y;
}

static bool
curve_is_vertical (const glyphy_curve_t *c)
{
  return c->p1.x == c->p2.x && c->p2.x == c->p3.x;
}


glyphy_bool_t
glyphy_curve_list_encode_blob (const glyphy_curve_t *curves,
			       unsigned int          num_curves,
			       glyphy_texel_t       *blob,
			       unsigned int          blob_size,
			       unsigned int         *output_len,
			       glyphy_extents_t     *extents)
{
  glyphy_curve_list_extents (curves, num_curves, extents);

  if (num_curves == 0) {
    *output_len = 0;
    return true;
  }

  /* Choose number of bands (capped at 16 per Slug paper) */
  unsigned int num_hbands = std::min (num_curves, 16u);
  unsigned int num_vbands = std::min (num_curves, 16u);
  num_hbands = std::max (num_hbands, 1u);
  num_vbands = std::max (num_vbands, 1u);

  double height = extents->max_y - extents->min_y;
  double width = extents->max_x - extents->min_x;

  if (height <= 0) num_hbands = 1;
  if (width <= 0) num_vbands = 1;

  double hband_size = height / num_hbands;
  double vband_size = width / num_vbands;

  /* Assign curves to bands */
  std::vector<std::vector<unsigned int>> hband_curves (num_hbands);
  std::vector<std::vector<unsigned int>> vband_curves (num_vbands);

  for (unsigned int i = 0; i < num_curves; i++) {
    /* Horizontal lines never intersect horizontal rays;
     * vertical lines never intersect vertical rays. */

    if (!curve_is_horizontal (&curves[i])) {
      if (height > 0) {
	double min_y, max_y;
	curve_y_range (&curves[i], &min_y, &max_y);
	int band_lo = (int) floor ((min_y - extents->min_y) / hband_size);
	int band_hi = (int) floor ((max_y - extents->min_y) / hband_size);
	band_lo = std::max (band_lo, 0);
	band_hi = std::min (band_hi, (int) num_hbands - 1);
	for (int b = band_lo; b <= band_hi; b++)
	  hband_curves[b].push_back (i);
      } else {
	hband_curves[0].push_back (i);
      }
    }

    if (!curve_is_vertical (&curves[i])) {
      if (width > 0) {
	double min_x, max_x;
	curve_x_range (&curves[i], &min_x, &max_x);
	int band_lo = (int) floor ((min_x - extents->min_x) / vband_size);
	int band_hi = (int) floor ((max_x - extents->min_x) / vband_size);
	band_lo = std::max (band_lo, 0);
	band_hi = std::min (band_hi, (int) num_vbands - 1);
	for (int b = band_lo; b <= band_hi; b++)
	  vband_curves[b].push_back (i);
      } else {
	vband_curves[0].push_back (i);
      }
    }
  }

  /* Sort curves within bands for early exit.
   * H-bands: descending max-x (rightward ray exits early).
   * V-bands: descending max-y (upward ray exits early). */
  for (auto &band : hband_curves)
    std::sort (band.begin (), band.end (),
	       [&] (unsigned int a, unsigned int b) {
		 return curve_max_x (&curves[a]) > curve_max_x (&curves[b]);
	       });

  for (auto &band : vband_curves)
    std::sort (band.begin (), band.end (),
	       [&] (unsigned int a, unsigned int b) {
		 return curve_max_y (&curves[a]) > curve_max_y (&curves[b]);
	       });

  /* Compute sizes */
  unsigned int total_curve_indices = 0;
  for (auto &band : hband_curves) total_curve_indices += band.size ();
  for (auto &band : vband_curves) total_curve_indices += band.size ();

  unsigned int header_len = 2; /* blob header: extents + band counts */
  unsigned int band_headers_len = num_hbands + num_vbands;
  unsigned int curve_data_len = num_curves * 2;
  unsigned int total_len = header_len + band_headers_len + total_curve_indices + curve_data_len;

  if (total_len > blob_size)
    return false;

  unsigned int curve_data_offset = header_len + band_headers_len + total_curve_indices;

  /* Pack blob header:
   * Texel 0: (min_x, min_y, max_x, max_y) - quantized extents
   * Texel 1: (num_hbands, num_vbands, 0, 0) */
  blob[0].r = quantize (extents->min_x);
  blob[0].g = quantize (extents->min_y);
  blob[0].b = quantize (extents->max_x);
  blob[0].a = quantize (extents->max_y);
  blob[1].r = (int16_t) num_hbands;
  blob[1].g = (int16_t) num_vbands;
  blob[1].b = 0;
  blob[1].a = 0;

  /* Pack curve data.
   * Quantize p1 and p3 first, then compute p2 as the average of the
   * quantized p1 and p3 IF the curve is a line (p2 == midpoint(p1,p3)).
   * This ensures a = p1 - 2*p2 + p3 = 0 exactly in the shader,
   * preventing quantization from turning lines into slight curves. */
  for (unsigned int i = 0; i < num_curves; i++) {
    unsigned int off = curve_data_offset + i * 2;
    int16_t p1x = quantize (curves[i].p1.x);
    int16_t p1y = quantize (curves[i].p1.y);
    int16_t p3x = quantize (curves[i].p3.x);
    int16_t p3y = quantize (curves[i].p3.y);
    int16_t p2x, p2y;

    /* Check if p2 is midpoint of p1 and p3 (i.e., curve is a line) */
    double mid_x = (curves[i].p1.x + curves[i].p3.x) * 0.5;
    double mid_y = (curves[i].p1.y + curves[i].p3.y) * 0.5;
    if (curves[i].p2.x == mid_x && curves[i].p2.y == mid_y) {
      /* Line: compute p2 from quantized endpoints to preserve a=0 */
      p2x = (int16_t) ((p1x + p3x) / 2);
      p2y = (int16_t) ((p1y + p3y) / 2);
    } else {
      p2x = quantize (curves[i].p2.x);
      p2y = quantize (curves[i].p2.y);
    }

    blob[off].r = p1x;
    blob[off].g = p1y;
    blob[off].b = p2x;
    blob[off].a = p2y;

    blob[off + 1].r = p3x;
    blob[off + 1].g = p3y;
    blob[off + 1].b = 0;
    blob[off + 1].a = 0;
  }

  /* Pack band headers and curve indices.
   * All offsets are relative to blob start (including header). */
  unsigned int index_offset = header_len + band_headers_len;

  for (unsigned int b = 0; b < num_hbands; b++) {
    blob[header_len + b].r = (int16_t) hband_curves[b].size ();
    blob[header_len + b].g = (int16_t) index_offset;
    blob[header_len + b].b = 0;
    blob[header_len + b].a = 0;

    for (unsigned int ci = 0; ci < hband_curves[b].size (); ci++) {
      unsigned int curve_off = curve_data_offset + hband_curves[b][ci] * 2;
      blob[index_offset].r = (int16_t) curve_off;
      blob[index_offset].g = 0;
      blob[index_offset].b = 0;
      blob[index_offset].a = 0;
      index_offset++;
    }
  }

  for (unsigned int b = 0; b < num_vbands; b++) {
    blob[header_len + num_hbands + b].r = (int16_t) vband_curves[b].size ();
    blob[header_len + num_hbands + b].g = (int16_t) index_offset;
    blob[header_len + num_hbands + b].b = 0;
    blob[header_len + num_hbands + b].a = 0;

    for (unsigned int ci = 0; ci < vband_curves[b].size (); ci++) {
      unsigned int curve_off = curve_data_offset + vband_curves[b][ci] * 2;
      blob[index_offset].r = (int16_t) curve_off;
      blob[index_offset].g = 0;
      blob[index_offset].b = 0;
      blob[index_offset].a = 0;
      index_offset++;
    }
  }

  *output_len = total_len;

  return true;
}
