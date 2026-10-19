#version 330 core

// Final pass: draws one of the simulation fields to the screen

// In
in vec2 texCoord;

// Out
out vec4 fragColor;

// Uniforms
uniform sampler2D density;
uniform sampler2D dye;
uniform sampler2D velocity;
uniform sampler2D pressure;
uniform sampler2D temperature;
uniform sampler2D obstacles;
uniform vec2 resolution;
uniform int mode;       // 0 dye, 1 velocity, 2 pressure, 3 classic (2014 look)
uniform float exposure;

const float PI = 3.14159265;

vec3 Hsv2Rgb(vec3 c)
{
	vec3 p = abs(fract(c.xxx + vec3(0.0, 2.0 / 3.0, 1.0 / 3.0)) * 6.0 - 3.0);
	return c.z * mix(vec3(1.0), clamp(p - 1.0, 0.0, 1.0), c.y);
}

// Blue - white - red, for signed fields
vec3 Diverging(float x)
{
	x = clamp(x, -1.0, 1.0);
	vec3 neg = vec3(0.12, 0.35, 0.85), mid = vec3(0.06, 0.06, 0.08), pos = vec3(0.95, 0.35, 0.12);
	return x < 0.0 ? mix(mid, neg, -x) : mix(mid, pos, x);
}

vec3 Background(vec2 uv)
{
	// Soft dark vignette instead of flat black
	float v = 1.0 - 0.55 * dot(uv - 0.5, uv - 0.5) * 2.0;
	return mix(vec3(0.008, 0.010, 0.018), vec3(0.035, 0.042, 0.065), v);
}

void main()
{
	vec2 uv = texCoord;
	float solid = texture(obstacles, uv).x;

	if (mode == 3)
	{
		// 2014: blue smoke, red walls
		float d = texture(density, uv).x;
		fragColor = solid > 0.5 ? vec4(1.0, 0.0, 0.0, 1.0) : vec4(0.0, 0.0, clamp(d, 0.0, 1.0), 1.0);
		return;
	}

	vec3 color;

	if (mode == 0)
	{
		// Dye: filmic-ish exposure so dense areas saturate softly instead of clipping
		vec3 d = max(texture(dye, uv).rgb, 0.0);
		float heat = max(texture(temperature, uv).x, 0.0);
		vec3 smoke = vec3(1.0) - exp(-3.0 * exposure * d);

		// Fake top-left light from the smoke thickness gradient, gives the flat dye some volume
		vec2 px = 1.0 / vec2(textureSize(dye, 0));
		float tE = dot(texture(dye, uv + vec2(px.x, 0)).rgb, vec3(1.0));
		float tW = dot(texture(dye, uv - vec2(px.x, 0)).rgb, vec3(1.0));
		float tN = dot(texture(dye, uv + vec2(0, px.y)).rgb, vec3(1.0));
		float tS = dot(texture(dye, uv - vec2(0, px.y)).rgb, vec3(1.0));
		vec2 grad = vec2(tE - tW, tN - tS);
		float light = clamp(1.0 + 1.8 * dot(grad, normalize(vec2(-1.0, 1.0))), 0.55, 1.5);

		color = Background(uv) * (1.0 - clamp(dot(smoke, vec3(0.333)), 0.0, 1.0)) + smoke * light;
		color += vec3(1.0, 0.45, 0.15) * 0.08 * (1.0 - exp(-heat * 0.5));    // faint glow where it's hot
	}
	else if (mode == 1)
	{
		// Velocity: hue = direction, brightness = speed
		vec2 v = texture(velocity, uv).xy;
		float speed = length(v);
		float hue = atan(v.y, v.x) / (2.0 * PI) + 0.5;
		color = Hsv2Rgb(vec3(hue, 0.85, 1.0 - exp(-speed * 5.0 * exposure)));
	}
	else
	{
		// Pressure: diverging palette plus faint isolines
		float p = texture(pressure, uv).x * 500.0 * exposure;
		float lines = abs(fract(p * 4.0) - 0.5);
		color = Diverging(p) + vec3(0.12) * (1.0 - smoothstep(0.0, 0.06, lines));
	}

	// Obstacles: slate grey with a thin lit rim
	if (solid > 0.0)
	{
		vec2 px = 1.0 / vec2(textureSize(obstacles, 0));
		float rim = 4.0 - texture(obstacles, uv + vec2(px.x, 0)).x - texture(obstacles, uv - vec2(px.x, 0)).x
		              - texture(obstacles, uv + vec2(0, px.y)).x - texture(obstacles, uv - vec2(0, px.y)).x;
		vec3 wall = vec3(0.16, 0.17, 0.21) + vec3(0.35, 0.37, 0.42) * clamp(rim, 0.0, 1.0);
		color = mix(color, wall, clamp(solid, 0.0, 1.0));
	}

	// Colours above are picked in display space, so no extra gamma here
	fragColor = vec4(clamp(color, 0.0, 1.0), 1.0);
}
