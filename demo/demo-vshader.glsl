#version 330

uniform mat4 u_matViewProjection;
uniform vec2 u_viewport;

in vec2 a_position;
in vec2 a_texcoord;
in vec2 a_normal;
in vec2 a_jacobian;
in uint a_glyphLoc;

out vec2 v_texcoord;
flat out uint v_glyphLoc;

void main ()
{
  /* Transform undilated position to clip space */
  vec4 clipPos = u_matViewProjection * vec4 (a_position, 0.0, 1.0);

  /* Half pixel in NDC */
  vec2 halfPixelNDC = clipPos.w / u_viewport;

  /* Dilate in clip space along corner direction */
  clipPos.xy += a_normal * halfPixelNDC;
  gl_Position = clipPos;

  /* Compute object-space dilation to get texcoord correction.
   * d(clip.xy) = MVP * d(pos), so d(pos) = inverse(MVP_2x2) * d(clip.xy).
   * For the texcoord: d(tex) = d(pos) * jacobian. */
  vec2 dClip = a_normal * halfPixelNDC;
  mat2 mvp2 = mat2 (u_matViewProjection[0].xy, u_matViewProjection[1].xy);
  vec2 dPos = inverse (mvp2) * dClip;

  v_texcoord = a_texcoord + dPos * a_jacobian;
  v_glyphLoc = a_glyphLoc;
}
