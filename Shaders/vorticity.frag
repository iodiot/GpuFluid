#version 330 core

// Vorticity confinement (Fedkiw, Stam, Jensen 2001): puts back the small
// swirls that numerical dissipation of the semi-Lagrangian advection eats

// Out
out vec4 value;

// Uniforms
uniform sampler2D velocity;
uniform sampler2D curl;
uniform sampler2D obstacles;
uniform float epsilon;
uniform float dt;

void main()
{
	ivec2 t = ivec2(gl_FragCoord.xy);

	vec2 v = texelFetch(velocity, t, 0).xy;
	value = vec4(v, 0.0, 0.0);

	if (texelFetch(obstacles, t, 0).x > 0.0)
		return;

	float wN = abs(texelFetchOffset(curl, t, 0, ivec2(0, 1)).x);
	float wS = abs(texelFetchOffset(curl, t, 0, ivec2(0, -1)).x);
	float wE = abs(texelFetchOffset(curl, t, 0, ivec2(1, 0)).x);
	float wW = abs(texelFetchOffset(curl, t, 0, ivec2(-1, 0)).x);
	float wC = texelFetch(curl, t, 0).x;

	vec2 eta = 0.5 * vec2(wE - wW, wN - wS);
	vec2 N = eta / (length(eta) + 1e-5);

	value.xy += dt * epsilon * wC * vec2(N.y, -N.x);
}
