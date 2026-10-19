#version 330 core

// In
in vec2 texCoord;

// Out
out vec4 value;

// Uniforms
uniform sampler2D velocity;
uniform sampler2D obstacles;
uniform float dt;
uniform int n;
uniform float halfInverseCellSize;

void main()
{
  ivec2 t = ivec2(gl_FragCoord.xy);

  // Find neighboring velocities
  vec2 vN = texelFetchOffset(velocity, t, 0, ivec2(0, 1)).xy;
  vec2 vS = texelFetchOffset(velocity, t, 0, ivec2(0, -1)).xy;
  vec2 vE = texelFetchOffset(velocity, t, 0, ivec2(1, 0)).xy;
  vec2 vW = texelFetchOffset(velocity, t, 0, ivec2(-1, 0)).xy;

  // Find neighboring obstacles
  vec3 oN = texelFetchOffset(obstacles, t, 0, ivec2(0, 1)).xyz;
  vec3 oS = texelFetchOffset(obstacles, t, 0, ivec2(0, -1)).xyz;
  vec3 oE = texelFetchOffset(obstacles, t, 0, ivec2(1, 0)).xyz;
  vec3 oW = texelFetchOffset(obstacles, t, 0, ivec2(-1, 0)).xyz;

  // Use obstacle velocities for solid cells
  if (oN.x > 0.0) vN = oN.yz;
  if (oS.x > 0.0) vS = oS.yz;
  if (oE.x > 0.0) vE = oE.yz;
  if (oW.x > 0.0) vW = oW.yz;

  float div = -halfInverseCellSize * (vE.x - vW.x + vN.y - vS.y);
	
	value = vec4(div, 0.0, 0.0, 0.0);
}