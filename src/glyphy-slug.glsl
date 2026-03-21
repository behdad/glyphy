/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 * Based on the Slug algorithm by Eric Lengyel (MIT license).
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */


/* Requires GLSL 3.30 / ES 3.00 */


#ifndef GLYPHY_ATLAS_WIDTH
#define GLYPHY_ATLAS_WIDTH 4096
#endif

#ifndef GLYPHY_UNITS_PER_EM_UNIT
#define GLYPHY_UNITS_PER_EM_UNIT 2
#endif

#define GLYPHY_INV_UNITS float(1.0 / float(GLYPHY_UNITS_PER_EM_UNIT))

#define GLYPHY_LOG_ATLAS_WIDTH 12  /* log2(4096) */


uniform isampler2D u_atlas;


ivec2 glyphy_calc_blob_loc (ivec2 glyphLoc, int offset)
{
  ivec2 loc = ivec2 (glyphLoc.x + offset, glyphLoc.y);
  loc.y += loc.x >> GLYPHY_LOG_ATLAS_WIDTH;
  loc.x &= (1 << GLYPHY_LOG_ATLAS_WIDTH) - 1;
  return loc;
}

uint glyphy_calc_root_code (float y1, float y2, float y3)
{
  /* Extract sign bits of the three y coordinates.
   * This classifies the curve into one of 8 equivalence classes
   * that determine which roots contribute to the winding number. */

  uint i1 = floatBitsToUint (y1) >> 31U;
  uint i2 = floatBitsToUint (y2) >> 30U;
  uint i3 = floatBitsToUint (y3) >> 29U;

  uint shift = (i2 & 2U) | (i1 & ~2U);
  shift = (i3 & 4U) | (shift & ~4U);

  /* Eligibility returned in bits 0 and 8. */
  return (0x2E74U >> shift) & 0x0101U;
}

vec2 glyphy_solve_horiz_poly (vec4 p12, vec2 p3)
{
  /* Solve for t where curve crosses y = 0.
   * Quadratic: a*t^2 - 2*b*t + c = 0
   * Discriminant clamped to zero for robustness. */

  vec2 a = p12.xy - p12.zw * 2.0 + p3;
  vec2 b = p12.xy - p12.zw;
  float ra = 1.0 / a.y;
  float rb = 0.5 / b.y;

  float d = sqrt (max (b.y * b.y - a.y * p12.y, 0.0));
  float t1 = (b.y - d) * ra;
  float t2 = (b.y + d) * ra;

  /* Nearly linear case. */
  if (abs (a.y) < 1.0 / 65536.0)
    t1 = t2 = p12.y * rb;

  /* Return x coordinates at the roots. */
  return vec2 ((a.x * t1 - b.x * 2.0) * t1 + p12.x,
	       (a.x * t2 - b.x * 2.0) * t2 + p12.x);
}

vec2 glyphy_solve_vert_poly (vec4 p12, vec2 p3)
{
  /* Solve for t where curve crosses x = 0. */

  vec2 a = p12.xy - p12.zw * 2.0 + p3;
  vec2 b = p12.xy - p12.zw;
  float ra = 1.0 / a.x;
  float rb = 0.5 / b.x;

  float d = sqrt (max (b.x * b.x - a.x * p12.x, 0.0));
  float t1 = (b.x - d) * ra;
  float t2 = (b.x + d) * ra;

  if (abs (a.x) < 1.0 / 65536.0)
    t1 = t2 = p12.x * rb;

  return vec2 ((a.y * t1 - b.y * 2.0) * t1 + p12.y,
	       (a.y * t2 - b.y * 2.0) * t2 + p12.y);
}

float glyphy_calc_coverage (float xcov, float ycov, float xwgt, float ywgt)
{
  /* Combine horizontal and vertical ray coverages. */

  float coverage = max (abs (xcov * xwgt + ycov * ywgt) /
			max (xwgt + ywgt, 1.0 / 65536.0),
			min (abs (xcov), abs (ycov)));

  /* Nonzero fill rule. */
  return clamp (coverage, 0.0, 1.0);
}

float glyphy_slug_render (vec2 renderCoord, vec4 bandTransform,
			  ivec2 glyphLoc, int numHBands, int numVBands)
{
  vec2 emsPerPixel = fwidth (renderCoord);
  vec2 pixelsPerEm = 1.0 / emsPerPixel;

  /* Map em-space coordinates to band indices. */
  ivec2 bandIndex = clamp (ivec2 (renderCoord * bandTransform.xy + bandTransform.zw),
			   ivec2 (0, 0),
			   ivec2 (numVBands - 1, numHBands - 1));

  float xcov = 0.0;
  float xwgt = 0.0;

  /* Fetch H-band header. H-bands are at offsets [0, numHBands-1]. */
  ivec4 hbandData = texelFetch (u_atlas,
				glyphy_calc_blob_loc (glyphLoc, bandIndex.y), 0);
  int hCurveCount = hbandData.r;
  int hDataOffset = hbandData.g;

  /* Loop over curves in the horizontal band. */
  for (int ci = 0; ci < hCurveCount; ci++)
  {
    /* Fetch curve offset from index list. */
    ivec4 indexData = texelFetch (u_atlas,
				 glyphy_calc_blob_loc (glyphLoc, hDataOffset + ci), 0);
    int curveOffset = indexData.r;

    /* Fetch control points and convert from quantized int16 to em-space. */
    ivec4 raw12 = texelFetch (u_atlas,
			      glyphy_calc_blob_loc (glyphLoc, curveOffset), 0);
    ivec4 raw3 = texelFetch (u_atlas,
			     glyphy_calc_blob_loc (glyphLoc, curveOffset + 1), 0);

    vec4 p12 = vec4 (raw12) * GLYPHY_INV_UNITS - vec4 (renderCoord, renderCoord);
    vec2 p3 = vec2 (raw3.rg) * GLYPHY_INV_UNITS - renderCoord;

    /* Early exit: if max x of all control points is left of pixel. */
    if (max (max (p12.x, p12.z), p3.x) * pixelsPerEm.x < -0.5)
      break;

    uint code = glyphy_calc_root_code (p12.y, p12.w, p3.y);
    if (code != 0U)
    {
      vec2 r = glyphy_solve_horiz_poly (p12, p3) * pixelsPerEm.x;

      if ((code & 1U) != 0U)
      {
	xcov += clamp (r.x + 0.5, 0.0, 1.0);
	xwgt = max (xwgt, clamp (1.0 - abs (r.x) * 2.0, 0.0, 1.0));
      }

      if (code > 1U)
      {
	xcov -= clamp (r.y + 0.5, 0.0, 1.0);
	xwgt = max (xwgt, clamp (1.0 - abs (r.y) * 2.0, 0.0, 1.0));
      }
    }
  }

  float ycov = 0.0;
  float ywgt = 0.0;

  /* Fetch V-band header. V-bands start at offset numHBands. */
  ivec4 vbandData = texelFetch (u_atlas,
				glyphy_calc_blob_loc (glyphLoc, numHBands + bandIndex.x), 0);
  int vCurveCount = vbandData.r;
  int vDataOffset = vbandData.g;

  /* Loop over curves in the vertical band. */
  for (int ci = 0; ci < vCurveCount; ci++)
  {
    ivec4 indexData = texelFetch (u_atlas,
				 glyphy_calc_blob_loc (glyphLoc, vDataOffset + ci), 0);
    int curveOffset = indexData.r;

    ivec4 raw12 = texelFetch (u_atlas,
			      glyphy_calc_blob_loc (glyphLoc, curveOffset), 0);
    ivec4 raw3 = texelFetch (u_atlas,
			     glyphy_calc_blob_loc (glyphLoc, curveOffset + 1), 0);

    vec4 p12 = vec4 (raw12) * GLYPHY_INV_UNITS - vec4 (renderCoord, renderCoord);
    vec2 p3 = vec2 (raw3.rg) * GLYPHY_INV_UNITS - renderCoord;

    if (max (max (p12.y, p12.w), p3.y) * pixelsPerEm.y < -0.5)
      break;

    uint code = glyphy_calc_root_code (p12.x, p12.z, p3.x);
    if (code != 0U)
    {
      vec2 r = glyphy_solve_vert_poly (p12, p3) * pixelsPerEm.y;

      if ((code & 1U) != 0U)
      {
	ycov -= clamp (r.x + 0.5, 0.0, 1.0);
	ywgt = max (ywgt, clamp (1.0 - abs (r.x) * 2.0, 0.0, 1.0));
      }

      if (code > 1U)
      {
	ycov += clamp (r.y + 0.5, 0.0, 1.0);
	ywgt = max (ywgt, clamp (1.0 - abs (r.y) * 2.0, 0.0, 1.0));
      }
    }
  }

  return glyphy_calc_coverage (xcov, ycov, xwgt, ywgt);
}

