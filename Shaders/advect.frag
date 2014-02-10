#version 130

// In
in vec3 texCoord;

// Out
out vec3 value;

// Uniforms
uniform sampler2D velocity;
uniform sampler2D q;
uniform sampler2D obstacles;
uniform float dt;
uniform int n;
uniform float inverseCellSize;

vec2 Advect(in vec2 p, in float scale)
{
	vec2 np = p + scale * dt * float(n) * texelFetch(velocity, ivec2(gl_FragCoord), 0).xy;

	return texture(q, np * inverseCellSize).xy;	
}

vec2 MacCormack(in vec2 p)
{
	vec2 uv = texelFetch(velocity, ivec2(p), 0).xy;
	vec2 np = p - dt * float(n) * uv;

	ivec2 inp = ivec2(np + vec2(0.5));

  vec2 qN = texelFetchOffset(q, inp, 0, ivec2(0, 1)).xy;
  vec2 qS = texelFetchOffset(q, inp, 0, ivec2(0, -1)).xy;
  vec2 qE = texelFetchOffset(q, inp, 0, ivec2(1, 0)).xy;
  vec2 qW = texelFetchOffset(q, inp, 0, ivec2(-1, 0)).xy;
	vec2 qC = texelFetch(q, inp, 0).xy;

	vec2 qMin = min(qN, min(qS, min(qE, qW)));
	vec2 qMax = max(qN, max(qS, max(qE, qW)));

	vec2 q_hat_n_1 = Advect(p, -1.0);
	vec2 q_hat_n = Advect(np, 1.0);

	vec2 r = q_hat_n_1 + 0.5 * (qC - q_hat_n);

	r = max(min(r, qMax), qMin);

	return r;
}

void main()
{	
	// Is it obstacle?
	float solid = texelFetch(obstacles, ivec2(gl_FragCoord), 0).x;
	if (solid > 0.0)
	{
		value = vec3(0.0);
		return;
	} 

	value.xy = Advect(gl_FragCoord.xy, -1.0);

	//value = vec3(MacCormack(gl_FragCoord.xy).xy, 1.0); //vec3(Advect(gl_FragCoord.xy, -1.0), 0.0);
}