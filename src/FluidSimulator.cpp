#include "StdAfx.h"
#include "FluidSimulator.h"
#include "OpenGL.h"
#include "Main.h"

Vector3Buffer::Vector3Buffer(int n)
	: buffer(n * n), n(n)
{
}

void Vector3Buffer::Reset()
{
	for (Vector3 &v : buffer)
		v.x = v.y = v.z = 0.0f;
}

void Vector3Buffer::ToTexture(GLuint texture)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, n, n, GL_RGB, GL_FLOAT, buffer.data());
}

Vector3& Vector3Buffer::Get(int x, int y)
{
	return buffer[x + y * n];
}

FluidSimulator::FluidSimulator(const FluidSettings &settings)
	: settings(settings), storedVorticity(settings.vorticity > 0.0f ? settings.vorticity : 0.5f)
{
	glGenFramebuffers(1, &frameBufferHandle);

	// Load programs
	advectProgram = LoadProgram("default", "advect");
	splatProgram = LoadProgram("default", "splat");
	clearProgram = LoadProgram("default", "clear");
	divergenceProgram = LoadProgram("default", "divergence");
	pressureProgram = LoadProgram("default", "pressure");
	jacobiProgram = LoadProgram("default", "jacobi");
	buoyancyProgram = LoadProgram("default", "buoyancy");
	curlProgram = LoadProgram("default", "curl");
	vorticityProgram = LoadProgram("default", "vorticity");

	// Create textures
	const int n = settings.n;
	velocity = Create2dTexture(n);
	divergence = Create2dTexture(n);
	pressure = Create2dTexture(n);
	buffer = Create2dTexture(n);
	density = Create2dTexture(n);
	temperature = Create2dTexture(n);
	dye = Create2dTexture(n);
	curl = Create2dTexture(n);
	obstacles = Create2dTexture(n);

	v3b = new Vector3Buffer(n);

	Reset();
}

FluidSimulator::~FluidSimulator()
{
	delete v3b;

	GLuint textures[] = { velocity, divergence, pressure, buffer, density, temperature, dye, curl, obstacles };
	glDeleteTextures(sizeof(textures) / sizeof(textures[0]), textures);
	glDeleteFramebuffers(1, &frameBufferHandle);
}

void FluidSimulator::Reset()
{
	InitalizeObstacles();

	Clear(velocity);
	Clear(density);
	Clear(temperature);
	Clear(dye);
	Clear(pressure);

	PostUpdate();
}

void FluidSimulator::RenderToTexture(GLuint target)
{
	glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
	glViewport(0, 0, settings.n, settings.n);

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target, 0);

	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	glBindVertexArray(vaoHandle);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void FluidSimulator::PostUpdate()
{
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);

	// Render to the screen
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, windowX, windowY);
}

void FluidSimulator::Update()
{
	Step();

	PostUpdate();
}

void FluidSimulator::Advect(GLuint target, GLuint q, GLuint velocity, float dissipation)
{
	SetProgram(advectProgram);

	PassDefaultVariables();

	PassTexture(0, "velocity", velocity);
	PassTexture(1, "q", q);
	PassTexture(2, "obstacles", obstacles);

	PassFloat("inverseCellSize", 1.0f / float(settings.n));
	PassFloat("dissipation", dissipation);

	RenderToTexture(target);
}

void FluidSimulator::PassTexture(GLuint index, const string &textureName, GLuint texture)
{
	glActiveTexture(GL_TEXTURE0 + index);
	glBindTexture(GL_TEXTURE_2D, texture);
	GLint location = glGetUniformLocation(currentProgram, textureName.c_str());
	glUniform1i(location, index);
}

void FluidSimulator::PassFloat(const char *name, float value)
{
	glUniform1f(glGetUniformLocation(currentProgram, name), value);
}

void FluidSimulator::PassDefaultVariables()
{
	glUniform1i(glGetUniformLocation(currentProgram, "n"), settings.n);
	PassFloat("dt", settings.dt);
}

void FluidSimulator::Project(GLuint &velocity)
{
	Divergence(divergence, velocity);

	// Pressure from the previous frame is a good first guess (warm start)
	for (int i = 0; i < settings.jacobiIterations; ++i)
	{
		Jacobi(buffer, pressure, divergence);
		swap(pressure, buffer);
	}

	Pressure(buffer, velocity, pressure); swap(velocity, buffer);
}

void FluidSimulator::Divergence(GLuint target, GLuint velocity)
{
	SetProgram(divergenceProgram);

	PassDefaultVariables();

	PassTexture(0, "velocity", velocity);
	PassTexture(1, "obstacles", obstacles);

	PassFloat("halfInverseCellSize", 0.5f / float(settings.n));

	RenderToTexture(target);
}

void FluidSimulator::Jacobi(GLuint target, GLuint pressure, GLuint divergence)
{
	SetProgram(jacobiProgram);

	PassDefaultVariables();

	PassTexture(0, "pressure", pressure);
	PassTexture(1, "divergence", divergence);
	PassTexture(2, "obstacles", obstacles);

	PassFloat("alpha", 1.0f);
	PassFloat("inverseBeta", 0.25f);

	RenderToTexture(target);
}

void FluidSimulator::Clear(GLuint target)
{
	SetProgram(clearProgram);

	PassDefaultVariables();

	RenderToTexture(target);
}

void FluidSimulator::Pressure(GLuint target, GLuint velocity, GLuint pressure)
{
	SetProgram(pressureProgram);

	PassDefaultVariables();

	PassTexture(0, "velocity", velocity);
	PassTexture(1, "pressure", pressure);
	PassTexture(2, "obstacles", obstacles);

	PassFloat("gradientScale", 0.5f * float(settings.n));

	RenderToTexture(target);
}

void FluidSimulator::Curl(GLuint target, GLuint velocity)
{
	SetProgram(curlProgram);

	PassDefaultVariables();

	PassTexture(0, "velocity", velocity);

	RenderToTexture(target);
}

void FluidSimulator::Vorticity(GLuint target, GLuint velocity, GLuint curl)
{
	SetProgram(vorticityProgram);

	PassDefaultVariables();

	PassTexture(0, "velocity", velocity);
	PassTexture(1, "curl", curl);
	PassTexture(2, "obstacles", obstacles);

	PassFloat("epsilon", settings.vorticity);

	RenderToTexture(target);
}

void FluidSimulator::Splat(GLuint &field, float x, float y, float r, float g, float b, float radius)
{
	SetProgram(splatProgram);

	PassDefaultVariables();

	PassTexture(0, "q", field);
	PassTexture(1, "obstacles", obstacles);

	glUniform2f(glGetUniformLocation(currentProgram, "point"), x * settings.n, y * settings.n);
	glUniform3f(glGetUniformLocation(currentProgram, "color"), r, g, b);
	PassFloat("radius", max(radius * settings.n, 1.0f));

	RenderToTexture(buffer);
	swap(field, buffer);

	PostUpdate();
}

void FluidSimulator::AddForce(float x, float y, float dx, float dy, float radius)
{
	Splat(velocity, x, y, dx, dy, 0.0f, radius);
}

void FluidSimulator::AddHeat(float x, float y, float amount, float radius)
{
	Splat(temperature, x, y, amount, 0.0f, 0.0f, radius);
}

void FluidSimulator::AddFluid(float x, float y, float r, float g, float b, float amount, float radius)
{
	Splat(density, x, y, amount, 0.0f, 0.0f, radius);
	Splat(dye, x, y, r * amount, g * amount, b * amount, radius);
}

void FluidSimulator::ToggleObstacle()
{
	settings.obstacle = !settings.obstacle;
	InitalizeObstacles();
}

void FluidSimulator::ToggleVorticity()
{
	if (settings.vorticity > 0.0f)
	{
		storedVorticity = settings.vorticity;
		settings.vorticity = 0.0f;
	}
	else
		settings.vorticity = storedVorticity;
}

void FluidSimulator::SetProgram(GLuint program)
{
	currentProgram = program;
	glUseProgram(currentProgram);
}

void FluidSimulator::InitalizeObstacles()
{
	const int n = settings.n;

	// Circle radius scales with the grid (2014: r^2 < n at n = 128)
	const float radius = 0.088f * n;

	v3b->Reset();

	for (int x = 0; x < n; ++x)
		for (int y = 0; y < n; ++y)
		{
			// Bounding box
			if (x == 0 || x == n - 1 || y == 0 || y == n - 1)
				v3b->Get(x, y).x = 1.0f;

			// Circle in center
			float dx = x + 0.5f - n * 0.5f, dy = y + 0.5f - n * 0.5f;
			if (settings.obstacle && dx * dx + dy * dy < radius * radius)
				v3b->Get(x, y).x = 1.0f;
		}

	v3b->ToTexture(obstacles);
}

void FluidSimulator::Buoyancy(GLuint target, GLuint velocity, GLuint density, GLuint temperature)
{
	SetProgram(buoyancyProgram);

	PassDefaultVariables();

	PassTexture(0, "velocity", velocity);
	PassTexture(1, "density", density);
	PassTexture(2, "temperature", temperature);

	PassFloat("sigma", settings.smokeBuoyancy);
	PassFloat("kappa", settings.smokeWeight);
	PassFloat("ambientTemperature", settings.ambientTemperature);

	RenderToTexture(target);
}

void FluidSimulator::Step()
{
	// Self-advection
	Advect(buffer, velocity, velocity, settings.velocityDissipation); swap(velocity, buffer);
	Advect(buffer, density, velocity, settings.dyeDissipation); swap(density, buffer);
	Advect(buffer, dye, velocity, settings.dyeDissipation); swap(dye, buffer);
	Advect(buffer, temperature, velocity, settings.temperatureDissipation); swap(temperature, buffer);

	// Add body forces
	Buoyancy(buffer, velocity, density, temperature); swap(velocity, buffer);

	if (settings.vorticity > 0.0f)
	{
		Curl(curl, velocity);
		Vorticity(buffer, velocity, curl); swap(velocity, buffer);
	}

	Project(velocity);
}
