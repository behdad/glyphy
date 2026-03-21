#version 330

uniform mat4 u_matViewProjection;

/* Per-vertex attributes */
in vec2 a_position;         /* Object-space vertex position */
in vec2 a_texcoord;         /* Em-space sample coordinates */

/* Per-vertex but constant across glyph (flat) */
in vec4 a_bandTransform;    /* (scale_x, scale_y, offset_x, offset_y) */
in ivec4 a_glyphData;      /* (atlas_x, atlas_y, num_hbands, num_vbands) */

/* Outputs to fragment shader */
out vec2 v_texcoord;
flat out vec4 v_bandTransform;
flat out ivec4 v_glyphData;

void main ()
{
  gl_Position = u_matViewProjection * vec4 (a_position, 0.0, 1.0);
  v_texcoord = a_texcoord;
  v_bandTransform = a_bandTransform;
  v_glyphData = a_glyphData;
}
