#version 330 core

// In
in vec2 texCoord;

// Out
out vec4 value;

// Uniforms
uniform sampler2D velocity;
uniform sampler2D q;
uniform sampler2D obstacles;
uniform float dt;
uniform int n;
uniform float inverseCellSize;
uniform float dissipation;

// Semi-Lagrangian step: trace the cell centre back along the velocity
vec4 Advect(in vec2 p, in float scale)
{
	vec2 np = p + scale * dt * float(n) * texelFetch(velocity, ivec2(gl_FragCoord.xy), 0).xy;

	return texture(q, np * inverseCellSize);
}

void main()
{
	// Is it obstacle?
	float solid = texelFetch(obstacles, ivec2(gl_FragCoord.xy), 0).x;
	if (solid > 0.0)
	{
		value = vec4(0.0);
		return;
	}

	value = dissipation * Advect(gl_FragCoord.xy, -1.0);
}
