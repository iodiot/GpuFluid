#version 330 core

// Out
out vec4 value;

// Uniforms
uniform sampler2D velocity;

void main()
{
	ivec2 t = ivec2(gl_FragCoord.xy);

	float vN = texelFetchOffset(velocity, t, 0, ivec2(0, 1)).x;
	float vS = texelFetchOffset(velocity, t, 0, ivec2(0, -1)).x;
	float vE = texelFetchOffset(velocity, t, 0, ivec2(1, 0)).y;
	float vW = texelFetchOffset(velocity, t, 0, ivec2(-1, 0)).y;

	// z-component of curl, in cell units
	value = vec4(0.5 * ((vE - vW) - (vN - vS)), 0.0, 0.0, 0.0);
}
