#version 130

// Out
out vec3 value;

// Uniforms
uniform sampler2D q, q0;
uniform float dt;
uniform int n;

void main()
{
	ivec2 p = ivec2(gl_FragCoord);

	vec3 a = texelFetch(q, p, 0).xyz;
	vec3 b = texelFetch(q0, p, 0).xyz;

	value = a + b * dt;
}