#version 130

out vec3 value;

// Uniforms
uniform sampler2D q;
uniform int n;

void main()
{
	value = texelFetch(q, ivec2(gl_FragCoord.xy), 0).xyz;
}
