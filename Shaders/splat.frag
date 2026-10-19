#version 330 core

// Adds a gaussian blob of 'color' around 'point' (mouse or demo script)

// Out
out vec4 value;

// Uniforms
uniform sampler2D q;
uniform sampler2D obstacles;
uniform vec2 point;
uniform vec3 color;
uniform float radius;

void main()
{
	ivec2 p = ivec2(gl_FragCoord.xy);

	vec4 base = texelFetch(q, p, 0);
	float solid = texelFetch(obstacles, p, 0).x;

	vec2 d = gl_FragCoord.xy - point;
	float w = exp(-dot(d, d) / (radius * radius));

	value = base + (solid > 0.0 ? 0.0 : w) * vec4(color, 0.0);
}
