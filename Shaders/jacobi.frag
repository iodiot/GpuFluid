#version 130

// In
in vec3 texCoord;

// Out
out vec3 value;

// Uniforms
uniform float dt;
uniform int n;
uniform float alpha;
uniform float inverseBeta;
uniform sampler2D pressure;
uniform sampler2D divergence;
uniform sampler2D obstacles;

void main()
{
  ivec2 t = ivec2(gl_FragCoord.xy);

  // Find neighboring pressures
  vec3 pN = texelFetchOffset(pressure, t, 0, ivec2(0, 1)).xyz;
  vec3 pS = texelFetchOffset(pressure, t, 0, ivec2(0, -1)).xyz;
  vec3 pE = texelFetchOffset(pressure, t, 0, ivec2(1, 0)).xyz;
  vec3 pW = texelFetchOffset(pressure, t, 0, ivec2(-1, 0)).xyz;
  vec3 pC = texelFetch(pressure, t, 0).xyz;

  // Find neighboring obstacles
  vec3 oN = texelFetchOffset(obstacles, t, 0, ivec2(0, 1)).xyz;
  vec3 oS = texelFetchOffset(obstacles, t, 0, ivec2(0, -1)).xyz;
  vec3 oE = texelFetchOffset(obstacles, t, 0, ivec2(1, 0)).xyz;
  vec3 oW = texelFetchOffset(obstacles, t, 0, ivec2(-1, 0)).xyz;

  // Use center pressure for solid cells
  if (oN.x > 0.0) pN = pC;
  if (oS.x > 0.0) pS = pC;
  if (oE.x > 0.0) pE = pC;
  if (oW.x > 0.0) pW = pC;

  vec3 bC = texelFetch(divergence, t, 0).xyz;
  value = (pW + pE + pS + pN + alpha * bC) * inverseBeta;
}