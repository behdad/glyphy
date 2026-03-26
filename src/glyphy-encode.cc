/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "glyphy.h"
#include "glyphy.hh"

#include <cstdint>
#include <cmath>
#include <limits>
#include <vector>
#include <algorithm>


/*
 * Encode quadratic Bezier curves into a blob for GPU rendering.
 *
 * Blob layout (single RGBA16I texture region):
 *
 *   [H-band headers (num_hbands texels)]
 *   [V-band headers (num_vbands texels)]
 *   [Curve index lists (variable)]
 *   [Curve data (2 texels per curve)]
 *
 * Band header texel:
 *   R = curve count
 *   G = offset to descending curve index list (from blob start)
 *   B = offset to ascending curve index list (from blob start)
 *   A = split value for symmetric optimization
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

static bool
quantize_fits_i16 (double v)
{
  double q = round (v * GLYPHY_UNITS_PER_EM_UNIT);
  return q >= std::numeric_limits<int16_t>::min () &&
         q <= std::numeric_limits<int16_t>::max ();
}

typedef struct
{
  double min_x;
  double max_x;
  double min_y;
  double max_y;
  bool is_horizontal;
  bool is_vertical;
  int hband_lo;
  int hband_hi;
  int vband_lo;
  int vband_hi;
} curve_info_t;

static curve_info_t
curve_info (const glyphy_curve_t *c)
{
  curve_info_t info;

  info.min_x = std::min (std::min (c->p1.x, c->p2.x), c->p3.x);
  info.max_x = std::max (std::max (c->p1.x, c->p2.x), c->p3.x);
  info.min_y = std::min (std::min (c->p1.y, c->p2.y), c->p3.y);
  info.max_y = std::max (std::max (c->p1.y, c->p2.y), c->p3.y);
  info.is_horizontal = c->p1.y == c->p2.y && c->p2.y == c->p3.y;
  info.is_vertical = c->p1.x == c->p2.x && c->p2.x == c->p3.x;
  info.hband_lo = 0;
  info.hband_hi = -1;
  info.vband_lo = 0;
  info.vband_hi = -1;

  return info;
}

/*
 * glyphy_t lifecycle and drawing
 */

glyphy_t *
glyphy_create (void)
{
  glyphy_t *g = new glyphy_t;
  glyphy_reset (g);
  return g;
}

void
glyphy_destroy (glyphy_t *g)
{
  delete g;
}

void
glyphy_reset (glyphy_t *g)
{
  g->start_point = g->current_point = {0, 0};
  g->need_moveto = true;
  g->num_curves = 0;
  g->success = true;
  g->curves.clear ();
}

static void
emit (glyphy_t *g, const glyphy_curve_t *curve)
{
  g->curves.push_back (*curve);
  g->num_curves++;
  g->current_point = curve->p3;
}

static void
emit_conic (glyphy_t *g,
            const glyphy_point_t *p2,
            const glyphy_point_t *p3)
{
  if (g->current_point.x == p3->x && g->current_point.y == p3->y)
    return;

  if (g->need_moveto) {
    g->start_point = g->current_point;
    g->need_moveto = false;
  }

  glyphy_curve_t curve = {g->current_point, *p2, *p3};
  emit (g, &curve);
}

void
glyphy_move_to (glyphy_t *g, const glyphy_point_t *p0)
{
  g->need_moveto = true;
  g->current_point = *p0;
}

void
glyphy_line_to (glyphy_t *g, const glyphy_point_t *p1)
{
  glyphy_point_t p0 = g->current_point;
  emit_conic (g, &p0, p1);
}

void
glyphy_conic_to (glyphy_t *g,
                 const glyphy_point_t *p1,
                 const glyphy_point_t *p2)
{
  emit_conic (g, p1, p2);
}

/* glyphy_cubic_to is in glyphy-cu2qu.cc */

void
glyphy_close_path (glyphy_t *g)
{
  if (!g->need_moveto &&
      (g->current_point.x != g->start_point.x ||
       g->current_point.y != g->start_point.y))
    glyphy_line_to (g, &g->start_point);
}

void
glyphy_get_current_point (glyphy_t *g, glyphy_point_t *point)
{
  *point = g->current_point;
}

unsigned int
glyphy_get_num_curves (glyphy_t *g)
{
  return g->num_curves;
}

glyphy_bool_t
glyphy_successful (glyphy_t *g)
{
  return g->success;
}


/*
 * Encode accumulated curves into blob
 */

glyphy_bool_t
glyphy_encode (glyphy_t         *g,
               glyphy_texel_t   *blob,
               unsigned int      blob_size,
               unsigned int     *output_len,
               glyphy_extents_t *extents)
{
  const glyphy_curve_t *curves = g->curves.data ();
  unsigned int num_curves = g->curves.size ();

  if (num_curves == 0) {
    glyphy_extents_clear (extents);
    *output_len = 0;
    return true;
  }

  std::vector<curve_info_t> curve_infos;
  curve_infos.resize (num_curves);
  glyphy_extents_clear (extents);
  for (unsigned int i = 0; i < num_curves; i++) {
    curve_infos[i] = curve_info (&curves[i]);

    if (i == 0) {
      extents->min_x = curve_infos[i].min_x;
      extents->max_x = curve_infos[i].max_x;
      extents->min_y = curve_infos[i].min_y;
      extents->max_y = curve_infos[i].max_y;
    } else {
      extents->min_x = std::min (extents->min_x, curve_infos[i].min_x);
      extents->max_x = std::max (extents->max_x, curve_infos[i].max_x);
      extents->min_y = std::min (extents->min_y, curve_infos[i].min_y);
      extents->max_y = std::max (extents->max_y, curve_infos[i].max_y);
    }
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

  std::vector<unsigned int> hband_curve_counts;
  std::vector<unsigned int> vband_curve_counts;
  hband_curve_counts.assign (num_hbands, 0);
  vband_curve_counts.assign (num_vbands, 0);

  for (unsigned int i = 0; i < num_curves; i++) {
    curve_info_t &info = curve_infos[i];

    if (!info.is_horizontal) {
      if (height > 0) {
        info.hband_lo = (int) floor ((info.min_y - extents->min_y) / hband_size);
        info.hband_hi = (int) floor ((info.max_y - extents->min_y) / hband_size);
        info.hband_lo = std::max (info.hband_lo, 0);
        info.hband_hi = std::min (info.hband_hi, (int) num_hbands - 1);
        for (int b = info.hband_lo; b <= info.hband_hi; b++)
          hband_curve_counts[b]++;
      } else {
        info.hband_lo = 0;
        info.hband_hi = 0;
        hband_curve_counts[0]++;
      }
    }

    if (!info.is_vertical) {
      if (width > 0) {
        info.vband_lo = (int) floor ((info.min_x - extents->min_x) / vband_size);
        info.vband_hi = (int) floor ((info.max_x - extents->min_x) / vband_size);
        info.vband_lo = std::max (info.vband_lo, 0);
        info.vband_hi = std::min (info.vband_hi, (int) num_vbands - 1);
        for (int b = info.vband_lo; b <= info.vband_hi; b++)
          vband_curve_counts[b]++;
      } else {
        info.vband_lo = 0;
        info.vband_hi = 0;
        vband_curve_counts[0]++;
      }
    }
  }

  std::vector<unsigned int> hband_offsets;
  std::vector<unsigned int> vband_offsets;
  hband_offsets.resize (num_hbands);
  vband_offsets.resize (num_vbands);
  unsigned int total_hband_indices = 0;
  unsigned int total_vband_indices = 0;

  for (unsigned int b = 0; b < num_hbands; b++) {
    hband_offsets[b] = total_hband_indices;
    total_hband_indices += hband_curve_counts[b];
  }

  for (unsigned int b = 0; b < num_vbands; b++) {
    vband_offsets[b] = total_vband_indices;
    total_vband_indices += vband_curve_counts[b];
  }

  /* Assign curves to bands */
  std::vector<unsigned int> hband_curves;
  std::vector<unsigned int> hband_curves_asc;
  std::vector<unsigned int> vband_curves;
  std::vector<unsigned int> vband_curves_asc;
  std::vector<unsigned int> hband_cursors;
  std::vector<unsigned int> vband_cursors;
  hband_curves.resize (total_hband_indices);
  hband_curves_asc.resize (total_hband_indices);
  vband_curves.resize (total_vband_indices);
  vband_curves_asc.resize (total_vband_indices);
  hband_cursors = hband_offsets;
  vband_cursors = vband_offsets;

  for (unsigned int i = 0; i < num_curves; i++) {
    const curve_info_t &info = curve_infos[i];

    /* Horizontal lines never intersect horizontal rays;
     * vertical lines never intersect vertical rays. */

    for (int b = info.hband_lo; b <= info.hband_hi; b++) {
      unsigned int index = hband_cursors[b]++;
      hband_curves[index] = i;
      hband_curves_asc[index] = i;
    }

    for (int b = info.vband_lo; b <= info.vband_hi; b++) {
      unsigned int index = vband_cursors[b]++;
      vband_curves[index] = i;
      vband_curves_asc[index] = i;
    }
  }

  /* Build two sort orders per band for symmetric optimization.
   * Descending max: for rightward/upward ray (current behavior).
   * Ascending min: for leftward/downward ray. */
  for (unsigned int b = 0; b < num_hbands; b++) {
    unsigned int off = hband_offsets[b];
    unsigned int count = hband_curve_counts[b];
    std::sort (hband_curves.begin () + off, hband_curves.begin () + off + count,
               [&] (unsigned int a, unsigned int b) {
                 return curve_infos[a].max_x > curve_infos[b].max_x;
               });
    std::sort (hband_curves_asc.begin () + off, hband_curves_asc.begin () + off + count,
               [&] (unsigned int a, unsigned int b) {
                 return curve_infos[a].min_x < curve_infos[b].min_x;
               });
  }

  for (unsigned int b = 0; b < num_vbands; b++) {
    unsigned int off = vband_offsets[b];
    unsigned int count = vband_curve_counts[b];
    std::sort (vband_curves.begin () + off, vband_curves.begin () + off + count,
               [&] (unsigned int a, unsigned int b) {
                 return curve_infos[a].max_y > curve_infos[b].max_y;
               });
    std::sort (vband_curves_asc.begin () + off, vband_curves_asc.begin () + off + count,
               [&] (unsigned int a, unsigned int b) {
                 return curve_infos[a].min_y < curve_infos[b].min_y;
               });
  }

  /* Compute sizes -- two index lists per band */
  unsigned int total_curve_indices = (total_hband_indices + total_vband_indices) * 2;

  unsigned int header_len = 2; /* blob header: extents + band counts */
  /* Compute curve data size with shared endpoints.
   * Adjacent curves in a contour share p3/p1: N+1 texels per contour.
   * Layout per contour: (p1,p2) (p3/p1,p2) ... (p3,0)
   * The shader reads curveLoc and curveLoc+1, same as before. */
  unsigned int num_contour_breaks = 0;
  for (unsigned int i = 0; i + 1 < num_curves; i++)
    if (curves[i].p3.x != curves[i + 1].p1.x ||
        curves[i].p3.y != curves[i + 1].p1.y)
      num_contour_breaks++;

  /* With sharing: num_curves + (num_contour_breaks + 1) texels
   * (one extra texel per contour for the final p3). */
  unsigned int curve_data_len = num_curves + num_contour_breaks + 1;

  unsigned int band_headers_len = num_hbands + num_vbands;
  unsigned int total_len = header_len + band_headers_len + total_curve_indices + curve_data_len;

  if (total_len > blob_size)
    return false;

  /* Offsets and counts are stored in signed 16-bit lanes in the atlas. */
  if (total_len - 1 > (unsigned int) std::numeric_limits<int16_t>::max ())
    return false;

  if (!quantize_fits_i16 (extents->min_x) ||
      !quantize_fits_i16 (extents->min_y) ||
      !quantize_fits_i16 (extents->max_x) ||
      !quantize_fits_i16 (extents->max_y))
    return false;

  unsigned int curve_data_offset = header_len + band_headers_len + total_curve_indices;

  /* Pack blob header */
  blob[0].r = quantize (extents->min_x);
  blob[0].g = quantize (extents->min_y);
  blob[0].b = quantize (extents->max_x);
  blob[0].a = quantize (extents->max_y);
  blob[1].r = (int16_t) num_hbands;
  blob[1].g = (int16_t) num_vbands;
  blob[1].b = 0;
  blob[1].a = 0;

  /* Pack curve data with shared endpoints.
   * Build curve_texel_offset[i] = texel offset for curve i's first texel. */
  std::vector<unsigned int> curve_texel_offset;
  curve_texel_offset.resize (num_curves);
  unsigned int texel = curve_data_offset;

  for (unsigned int i = 0; i < num_curves; i++) {
    bool contour_start = (i == 0 ||
                          curves[i - 1].p3.x != curves[i].p1.x ||
                          curves[i - 1].p3.y != curves[i].p1.y);

    if (contour_start) {
      curve_texel_offset[i] = texel;
      /* First curve in contour: write (p1, p2) */
      blob[texel].r = quantize (curves[i].p1.x);
      blob[texel].g = quantize (curves[i].p1.y);
      blob[texel].b = quantize (curves[i].p2.x);
      blob[texel].a = quantize (curves[i].p2.y);
      texel++;
    } else {
      /* Non-start curve: p12 is in the previous texel (p3_prev, p2) */
      curve_texel_offset[i] = texel - 1;
    }

    /* Write (p3, p2_next) or (p3, 0) if last in contour */
    bool has_next = (i + 1 < num_curves &&
                     curves[i].p3.x == curves[i + 1].p1.x &&
                     curves[i].p3.y == curves[i + 1].p1.y);

    blob[texel].r = quantize (curves[i].p3.x);
    blob[texel].g = quantize (curves[i].p3.y);
    if (has_next) {
      blob[texel].b = quantize (curves[i + 1].p2.x);
      blob[texel].a = quantize (curves[i + 1].p2.y);
    } else {
      blob[texel].b = 0;
      blob[texel].a = 0;
    }
    texel++;
  }

  /* Pack band headers and curve indices.
   * Band header: (count, desc_offset, asc_offset, split_value)
   * All offsets are relative to blob start. */
  unsigned int index_offset = header_len + band_headers_len;

  for (unsigned int b = 0; b < num_hbands; b++) {
    /* Per-band split: find x that minimizes max(left_count, right_count).
     * Curves with max_x >= split are processed by rightward ray.
     * Curves with min_x <= split are processed by leftward ray.
     * Descending sort is by max_x, so try split at each max_x boundary. */
    int16_t hband_split;
    {
      unsigned int off = hband_offsets[b];
      unsigned int n = hband_curve_counts[b];
      unsigned int best_worst = n;
      double best_split = (extents->min_x + extents->max_x) * 0.5;
      unsigned int left_count = n;
      for (unsigned int ci = 0; ci < n; ci++) {
        double split = curve_infos[hband_curves[off + ci]].max_x;
        unsigned int right_count = ci + 1; /* curves with max_x >= split */
        while (left_count &&
               curve_infos[hband_curves_asc[off + left_count - 1]].min_x > split)
          left_count--;
        unsigned int worst = std::max (right_count, left_count);
        if (worst < best_worst) {
          best_worst = worst;
          best_split = split;
        }
      }
      hband_split = quantize (best_split);
    }
    unsigned int hdr = header_len + b;
    unsigned int desc_off = index_offset;

    for (unsigned int ci = 0; ci < hband_curve_counts[b]; ci++) {
      blob[index_offset].r = (int16_t) curve_texel_offset[hband_curves[hband_offsets[b] + ci]];
      blob[index_offset].g = 0;
      blob[index_offset].b = 0;
      blob[index_offset].a = 0;
      index_offset++;
    }

    unsigned int asc_off = index_offset;

    for (unsigned int ci = 0; ci < hband_curve_counts[b]; ci++) {
      blob[index_offset].r = (int16_t) curve_texel_offset[hband_curves_asc[hband_offsets[b] + ci]];
      blob[index_offset].g = 0;
      blob[index_offset].b = 0;
      blob[index_offset].a = 0;
      index_offset++;
    }

    blob[hdr].r = (int16_t) hband_curve_counts[b];
    blob[hdr].g = (int16_t) desc_off;
    blob[hdr].b = (int16_t) asc_off;
    blob[hdr].a = hband_split;
  }

  for (unsigned int b = 0; b < num_vbands; b++) {
    int16_t vband_split;
    {
      unsigned int off = vband_offsets[b];
      unsigned int n = vband_curve_counts[b];
      unsigned int best_worst = n;
      double best_split = (extents->min_y + extents->max_y) * 0.5;
      unsigned int left_count = n;
      for (unsigned int ci = 0; ci < n; ci++) {
        double split = curve_infos[vband_curves[off + ci]].max_y;
        unsigned int right_count = ci + 1;
        while (left_count &&
               curve_infos[vband_curves_asc[off + left_count - 1]].min_y > split)
          left_count--;
        unsigned int worst = std::max (right_count, left_count);
        if (worst < best_worst) {
          best_worst = worst;
          best_split = split;
        }
      }
      vband_split = quantize (best_split);
    }

    unsigned int hdr = header_len + num_hbands + b;
    unsigned int desc_off = index_offset;

    for (unsigned int ci = 0; ci < vband_curve_counts[b]; ci++) {
      blob[index_offset].r = (int16_t) curve_texel_offset[vband_curves[vband_offsets[b] + ci]];
      blob[index_offset].g = 0;
      blob[index_offset].b = 0;
      blob[index_offset].a = 0;
      index_offset++;
    }

    unsigned int asc_off = index_offset;

    for (unsigned int ci = 0; ci < vband_curve_counts[b]; ci++) {
      blob[index_offset].r = (int16_t) curve_texel_offset[vband_curves_asc[vband_offsets[b] + ci]];
      blob[index_offset].g = 0;
      blob[index_offset].b = 0;
      blob[index_offset].a = 0;
      index_offset++;
    }

    blob[hdr].r = (int16_t) vband_curve_counts[b];
    blob[hdr].g = (int16_t) desc_off;
    blob[hdr].b = (int16_t) asc_off;
    blob[hdr].a = vband_split;
  }

  *output_len = total_len;

  return true;
}
