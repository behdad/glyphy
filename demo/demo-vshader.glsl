#version 330

uniform mat4 u_matViewProjection;

in vec2 a_position;
in vec2 a_texcoord;
in uint a_glyphLoc;

out vec2 v_texcoord;
flat out uint v_glyphLoc;

void main ()
{
  gl_Position = u_matViewProjection * vec4 (a_position, 0.0, 1.0);
  v_texcoord = a_texcoord;
  v_glyphLoc = a_glyphLoc;
}
