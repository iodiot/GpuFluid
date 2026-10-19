#version 330 core

out vec4 value;

// Uniforms
uniform sampler2D velocity;
uniform sampler2D density;
uniform sampler2D temperature;
uniform float ambientTemperature;
uniform float sigma;
uniform float kappa;
uniform float dt;
uniform int n;

void main()
{
	ivec2 p = ivec2(gl_FragCoord.xy);
	float t = texelFetch(temperature, p, 0).x;
	vec2 v = texelFetch(velocity, p, 0).xy;

	value = vec4(v, 0.0, 0.0);

	float d = texelFetch(density, p, 0).x;
	value.xy += dt * (d * kappa - (t - ambientTemperature) * sigma) * vec2(0, -1);
}
