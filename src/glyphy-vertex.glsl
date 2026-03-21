/*
 * Copyright 2026 Behdad Esfahbod. All Rights Reserved.
 * Based on the Slug algorithm by Eric Lengyel.
 *
 */


/* Requires GLSL 3.30 */


/* Dilate a glyph quad vertex by half a pixel on screen.
 *
 * position:  object-space vertex position (modified in place)
 * texcoord:  em-space sample coordinates (modified in place)
 * corner:    corner direction, each component -1 or +1
 * texPerPos: ratio of texcoord units to position units (e.g. upem / font_size)
 * mvp:       model-view-projection matrix
 * viewport:  viewport size in pixels
 */
void glyphy_dilate (inout vec2 position, inout vec2 texcoord,
			 vec2 corner, vec2 texPerPos,
			 mat4 mvp, vec2 viewport)
{
  vec4 clipPos = mvp * vec4 (position, 0.0, 1.0);

  /* Half pixel in clip space */
  vec2 halfPixelClip = clipPos.w / viewport;

  /* Dilate clip-space position along corner direction */
  vec2 dClip = corner * halfPixelClip;

  /* Map clip-space dilation back to object space.
   * For z=0 geometry: clip.xy = pos.x * mvp[0].xy + pos.y * mvp[1].xy + mvp[3].xy
   * So the 2x2 Jacobian of clip.xy w.r.t. pos.xy is (mvp[0].xy, mvp[1].xy). */
  mat2 mvp2 = mat2 (mvp[0].xy, mvp[1].xy);
  vec2 dPos = inverse (mvp2) * dClip;

  position += dPos;
  texcoord += dPos * texPerPos;
}
