#version 130

// In
in vec3 texCoord;

// Out
out vec3 value;

// Uniforms
uniform sampler2D velocity;
uniform sampler2D pressure;
uniform sampler2D obstacles;
uniform int n;
uniform float gradientScale;

void main()
{
  ivec2 t = ivec2(gl_FragCoord.xy);

  vec3 oC = texelFetch(obstacles, t, 0).xyz;
  if (oC.x > 0) 
	{
		value = vec3(oC.yz, 0.0);
		return;
  }

  // Find neighboring pressures
  float pN = texelFetchOffset(pressure, t, 0, ivec2(0, 1)).x;
  float pS = texelFetchOffset(pressure, t, 0, ivec2(0, -1)).x;
  float pE = texelFetchOffset(pressure, t, 0, ivec2(1, 0)).x;
  float pW = texelFetchOffset(pressure, t, 0, ivec2(-1, 0)).x;
  float pC = texelFetch(pressure, t, 0).x;

  // Find neighboring obstacles
  vec3 oN = texelFetchOffset(obstacles, t, 0, ivec2(0, 1)).xyz;
  vec3 oS = texelFetchOffset(obstacles, t, 0, ivec2(0, -1)).xyz;
  vec3 oE = texelFetchOffset(obstacles, t, 0, ivec2(1, 0)).xyz;
  vec3 oW = texelFetchOffset(obstacles, t, 0, ivec2(-1, 0)).xyz;

  // Use center pressure for solid cells
  vec2 obstV = vec2(0);
  vec2 vMask = vec2(1);

  if (oN.x > 0.0) { pN = pC; obstV.y = oN.z; vMask.y = 0; }
  if (oS.x > 0.0) { pS = pC; obstV.y = oS.z; vMask.y = 0; }
  if (oE.x > 0.0) { pE = pC; obstV.x = oE.y; vMask.x = 0; }
  if (oW.x > 0.0) { pW = pC; obstV.x = oW.y; vMask.x = 0; }

  // Enforce the free-slip boundary condition
  vec2 oldV = texelFetch(velocity, t, 0).xy;
  vec2 grad = vec2(pE - pW, pN - pS) * gradientScale;
  vec2 newV = oldV - grad;

  value = vec3((vMask * newV) + obstV, 0.0);  
}