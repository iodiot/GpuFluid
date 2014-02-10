#version 130

struct Ray
{
	vec3 origin, direction;
};

struct Plane
{	
	vec3 point, normal;
};

struct Box
{
	vec3 leftBottom, rightTop;
};

struct Sphere
{
	vec3 center;
	float radius;
};

struct ShadeData
{
	float t;
	vec3 normal;
	vec3 color;
};

const float BIG_FLOAT = 100500.0;

// In
in vec3 texCoord;
in vec3 cameraU;
in vec3 cameraW;
in vec3 cameraV;

// Out
out vec4 fragColor;

// Uniforms
uniform float cameraViewPlaneDistance;
uniform vec3 cameraPosition;
uniform vec2 resolution;

const Plane ground = Plane(vec3(0.0), vec3(0.0, 1.0, 0.0));
const Box smokeBox = Box(vec3(0.0), vec3(1.0, 0.5, 0.5));
const Sphere sun = Sphere(vec3(1.71), 0.1);

bool IntersectsPlane(in Ray r, in Plane p, in float tm, out ShadeData sd)
{
	sd.t = dot(p.point - r.origin, p.normal) / dot(r.direction, p.normal);

	if (sd.t > 0 && sd.t < tm)
	{
		sd.color = vec3(1.0);
		sd.normal = p.normal;
				
		return true;
	}

	return false;
}

bool PointInBox(in vec3 p, in Box b)
{
	const vec3 eps = vec3(0.0001);

	b.leftBottom -= eps;
	b.rightTop += eps;

	if (p.x >= b.leftBottom.x && p.y >= b.leftBottom.y && p.z >= b.leftBottom.z)
		if (p.x <= b.rightTop.x && p.y <= b.rightTop.y && p.z <= b.rightTop.z)
			return true;
	
	return false;
}

bool IntersectsFace(in Ray r, in Plane p, in Box b, in float tm, out ShadeData sd)
{
	const vec3 eps = vec3(0.00001);

	if (IntersectsPlane(r, p, tm, sd))
	{
		vec3 hitPoint = r.origin + r.direction * sd.t;
	
		if (PointInBox(hitPoint, b))
			return true;
	} 

	return false;
}

bool IntersectsBox(in Ray r, in Box b, in float tm, out ShadeData sd)
{
	ShadeData rsd;
	bool result = false;

	sd.t = tm;

	// Top faces
	if (IntersectsFace(r, Plane(b.rightTop, vec3(0.0, 1.0, 0.0)), b, sd.t, rsd))
	{
		sd = rsd;
		sd.color = vec3(1.0, 0.0, 0.0);
		result = true;
	}
 
 	if (IntersectsFace(r, Plane(b.rightTop, vec3(0.0, 0.0, 1.0)), b, sd.t, rsd))
	{
		sd = rsd;
		sd.color = vec3(0.0, 0.0, 1.0);
		result = true;
	}

	if (IntersectsFace(r, Plane(b.rightTop, vec3(1.0, 0.0, 0.0)), b, sd.t, rsd))
	{
		sd = rsd;
		sd.color = vec3(0.0, 1.0, 0.0);
		result = true;
	}

	// Bottom faces
	if (IntersectsFace(r, Plane(b.leftBottom, vec3(0.0, 1.0, 0.0)), b, sd.t, rsd))
	{
		sd = rsd;
		sd.color = vec3(1.0, 1.0, 0.0);
		result = true;
	}
 
 	if (IntersectsFace(r, Plane(b.leftBottom, vec3(0.0, 0.0, 1.0)), b, sd.t, rsd))
	{
		sd = rsd;
		sd.color = vec3(0.0, 1.0, 1.0);
		result = true;
	}

	if (IntersectsFace(r, Plane(b.leftBottom, vec3(1.0, 0.0, 0.0)), b, sd.t, rsd))
	{
		sd = rsd;
		sd.color = vec3(1.0, 0.0, 1.0);
		result = true;
	}

	return result;
}

bool IntersectsSphere(in Ray r, in Sphere s, in float tm, out ShadeData sd)
{
	vec3 temp = r.origin - s.center;
	float a = dot(r.direction, r.direction);
	float b = 2.0 * dot(temp, r.direction);
	float c = dot(temp, temp) - s.radius * s.radius;
	float disc = b * b - 4.0 * a * c;

	// No roots?
	if (disc < 0)
		return false;

	float e = sqrt(disc);
	float denom = 2.0 * a;

	// First root
	sd.t = (-b - e) / denom;	
	if (sd.t > 0 && sd.t < tm)
	{
		sd.normal = (temp + r.direction * sd.t) / s.radius;
		sd.color = vec3(1.0);
		return true;
	}

	return false;
}

bool IntersectsSomething(in Ray r, out ShadeData sd)
{
	ShadeData rsd;
	bool result = false;

	sd.t = BIG_FLOAT;

	if (IntersectsSphere(r, sun, sd.t, rsd))
	{
		sd = rsd;
		sd.color = vec3(1.0);
		result = true;
	}

	if (IntersectsBox(r, smokeBox, sd.t, rsd))
	{
		sd = rsd;
		result = true;
	}

	if (IntersectsPlane(r, ground, sd.t, rsd))
	{
		sd = rsd;

		vec3 hitPoint = r.origin + r.direction * sd.t;

		// Chess plates
		bool f1 = mod(hitPoint.x, 0.25) < 0.125;
		bool f2 = mod(hitPoint.z, 0.25) < 0.125;
		if (f1 ^^ f2)
			sd.color = vec3(0.6, 0.8, 0.6);
		else
			sd.color = vec3(0.3, 0.4, 0.3);

		result = true;
	}

	return result;
}

Ray BuildRay()
{
	Ray r;

	r.origin = cameraPosition;

	vec2 p = (texCoord.xy - 0.5) * resolution;

	r.direction = normalize(p.x * cameraU + p.y * cameraV - cameraViewPlaneDistance * cameraW);
	
	return r; 
}

vec3 ShadePixel(in Ray r, in ShadeData sd)
{
	if (sd.color.x == 1.0 && sd.color.y == 1.0 && sd.color.z == 1.0)
		return sd.color;

	// Diffuse light
	vec3 hitPoint = r.origin + r.direction * sd.t;
	vec3 s = normalize(sun.center - hitPoint);
	return sd.color * max(dot(s, sd.normal), 0.0);
}

void main()
{
	Ray r = BuildRay();
	ShadeData sd;
	vec3 finalColor = vec3(0.0);

	if (IntersectsSomething(r, sd))
	{
		finalColor = ShadePixel(r, sd);
	
		// Cast reflection ray
		r.origin = r.origin + r.direction * (sd.t - 0.01);
		r.direction = reflect(r.direction, sd.normal);

		if (IntersectsSomething(r, sd))
			finalColor += 0.25 * ShadePixel(r, sd);
	}
	
	fragColor = vec4(clamp(finalColor, 0.0, 1.0), 1.0);
}