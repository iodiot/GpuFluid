#include "StdAfx.h"
#include "FluidSimulator.h"
#include "OpenGL.h"
#include "Main.h"

Vector3Buffer::Vector3Buffer(int n)
	: n(n)
{
	buffer = new Vector3[n * n];
}

Vector3Buffer::~Vector3Buffer()
{
	delete[] buffer;
}

void Vector3Buffer::Reset()
{
	for (int i = 0; i < n * n; ++i)
		buffer[i].x = buffer[i].y = buffer[i].z = 0.0f;
}

void Vector3Buffer::ToTexture(GLuint texture)
{
	glBindTexture(GL_TEXTURE_2D, texture);
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, n, n, GL_RGB, GL_FLOAT, buffer);
}

Vector3& Vector3Buffer::Get(int x, int y)
{
	return buffer[x + y * n];
}

FluidSimulator::FluidSimulator() 
{
	glGenFramebuffers(1, &frameBufferHandle);

	// Load programs	
	addProgram = LoadProgram("default", "add");
	advectProgram = LoadProgram("default", "advect");	
	clearProgram = LoadProgram("default", "clear");
	copyProgram = LoadProgram("default", "copy");
	divergenceProgram = LoadProgram("default", "divergence");
	pressureProgram = LoadProgram("default", "pressure");	
	jacobiProgram = LoadProgram("default", "jacobi");
	buoyancyProgram = LoadProgram("default", "buoyancy");

	// Setup consts
	n = 128;
	dt = 0.1f;
	source = 50.0f;
	force = 1.0f;
	smokeBuoyancy = 0.05f;
	smokeWeight = 0.0005f;
	ambientTemperature = 0.0f;

	// Create textures
	velocity = Create2dTexture(n);
	divergence = Create2dTexture(n);
	pressure = Create2dTexture(n);
	buffer = Create2dTexture(n);
	density = Create2dTexture(n);
	forces = Create2dTexture(n);
	sources = Create2dTexture(n);
	obstacles = Create2dTexture(n);
	temperature = Create2dTexture(n);

	v3b = new Vector3Buffer(n);

	InitalizeObstacles();
	InitializeTemperature();

	v3b->Reset();
	v3b->ToTexture(density);
	v3b->ToTexture(velocity);
}

FluidSimulator::~FluidSimulator()
{
	delete v3b;
}

void FluidSimulator::PreUpdate()
{
}

void FluidSimulator::RenderToTexture(GLuint target)
{	
	glBindFramebuffer(GL_FRAMEBUFFER, frameBufferHandle);
	glViewport(0, 0, n, n);		

	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target, 0);
	
	assert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);

	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

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
	glViewport(0, 0, glutGet(GLUT_WINDOW_WIDTH), glutGet(GLUT_WINDOW_HEIGHT)); 
}

void FluidSimulator::Update()
{
	PreUpdate();

	UpdateForcesAndSources();

	Step();

	PostUpdate();
}

void FluidSimulator::Advect(GLuint target, GLuint q, GLuint velocity)
{
	SetProgram(advectProgram);

	PassDefaultVariables();

	PassTexture(0, "velocity", velocity);
	PassTexture(1, "q", q);
	PassTexture(2, "obstacles", obstacles);

	GLuint location = glGetUniformLocation(currentProgram, "inverseCellSize");
	glUniform1f(location, 1.0f / float(n));

	RenderToTexture(target);
}

void FluidSimulator::PassTexture(GLuint index, const string &textureName, GLuint texture)
{
	glActiveTexture(GL_TEXTURE0 + index);
	glBindTexture(GL_TEXTURE_2D, texture);
	GLuint location = glGetUniformLocation(currentProgram, textureName.c_str());
	glUniform1i(location, index);
}

void FluidSimulator::PassDefaultVariables()
{
	GLuint location = glGetUniformLocation(currentProgram, "n");
	glUniform1i(location, n);

	location = glGetUniformLocation(currentProgram, "dt");
	glUniform1f(location, dt);
}

void FluidSimulator::Copy(GLuint target, GLuint q)
{
	SetProgram(copyProgram);

	PassDefaultVariables();

	PassTexture(0, "q", q);

	RenderToTexture(target);
}

void FluidSimulator::Add(GLuint target, GLuint q, GLuint q0)
{
	SetProgram(addProgram);

	PassDefaultVariables();

	PassTexture(0, "q", q);
	PassTexture(1, "q0", q0);

	RenderToTexture(target);
}

void FluidSimulator::Project(GLuint target, GLuint velocity)
{
	Divergence(divergence, velocity);

	Clear(pressure);

	for (int i = 0; i < 20; ++i)
	{
		Jacobi(buffer, pressure, divergence);
		Copy(pressure, buffer);
	}

	Pressure(target, velocity, pressure);
}

void FluidSimulator::Divergence(GLuint target, GLuint velocity)
{
	SetProgram(divergenceProgram);

	PassDefaultVariables();

	PassTexture(0, "velocity", velocity);
	PassTexture(1, "obstacles", obstacles);

	GLuint location = glGetUniformLocation(currentProgram, "halfInverseCellSize");
	glUniform1f(location, 0.5f / float(n));

	RenderToTexture(target);

	Copy(buffer, target);
}

void FluidSimulator::Jacobi(GLuint target, GLuint pressure, GLuint divergence)
{
	SetProgram(jacobiProgram);

	PassDefaultVariables();

	PassTexture(0, "pressure", pressure);
	PassTexture(1, "divergence", divergence);
	PassTexture(2, "obstacles", obstacles);

	GLuint location = glGetUniformLocation(currentProgram, "alpha");
	glUniform1f(location, 1.0f);

	location = glGetUniformLocation(currentProgram, "inverseBeta");
	glUniform1f(location, 0.25f);

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

	GLuint location = glGetUniformLocation(currentProgram, "gradientScale");
	glUniform1f(location, 0.5f * float(n));

	RenderToTexture(target);
}

GLuint FluidSimulator::GetVelocity()
{
	return velocity;
}

void FluidSimulator::UpdateForcesAndSources()
{
	v3b->Reset();

	/*for (int x = 10; x < n - 10; ++x)
		for (int y = 10; y < n - 10; ++y)
			v3b->Get(x, y).y = 0.01f;
*/

	v3b->ToTexture(forces);

	v3b->Reset();

	/*for (int x = n / 2 - 10; x < n / 2 + 10; ++x)
		for (int y = n / 4 - 10; y < n / 4 + 10; ++y)
			v3b->Get(x, y).x = 1.0f;*/

	v3b->ToTexture(sources);


	//v3b->ToTexture(forces);

	if (!mouseDown[0] && !mouseDown[2]) return;

	int x = int((mouseX / float(windowX)) * n + 1);
	int y = int(((windowY - mouseY) / float(windowY)) * n + 1);

	if (x < 1 || x >= n || y < 1 || y >= n) return;

	// Force
	if (mouseDown[0]) 
	{
		v3b->Get(x, y).x = force;// * (mouseX - originalMouseX);
		v3b->Get(x, y).y = force;// * (originalMouseY - mouseY);
		
		v3b->ToTexture(forces);
		v3b->Reset();
	}

	// Source
	if (mouseDown[2])
	{
		v3b->Get(x, y).x = source;
	
		v3b->ToTexture(sources);
		v3b->Reset();
	}

	originalMouseX = mouseX;
	originalMouseY = mouseY;
}

GLuint FluidSimulator::GetDensity()
{
	return density;
}

void FluidSimulator::SetProgram(GLuint program)
{
	currentProgram = program;
	glUseProgram(currentProgram);
}

void FluidSimulator::InitalizeObstacles()
{
	v3b->Reset();

	for (int x = 0; x < n; ++x)
		for (int y = 0; y < n; ++y)
		{
			// Bounding box
			if (x == 0 || x == n - 3 || y == 0 || y == n - 3)
				v3b->Get(x, y).x = 1.0f;

			// Circle in center
			if ((x - n / 2) * (x - n / 2) + (y - n / 2) * (y - n / 2) < n)
				v3b->Get(x, y).x = 1.0f;
		}

	v3b->ToTexture(obstacles);
}

GLuint FluidSimulator::GetObstacles()
{
	return obstacles;
}

void FluidSimulator::Buoyancy(GLuint target, GLuint velocity, GLuint density, GLuint temperature)
{
	SetProgram(buoyancyProgram);

	PassDefaultVariables();

	PassTexture(0, "velocity", velocity);
	PassTexture(1, "density", density);
	PassTexture(2, "temperature", temperature);

	GLuint location = glGetUniformLocation(currentProgram, "sigma");
	glUniform1f(location, smokeBuoyancy);

	location = glGetUniformLocation(currentProgram, "kappa");
	glUniform1f(location, smokeWeight);

	location = glGetUniformLocation(currentProgram, "ambientTemperature");
	glUniform1f(location, ambientTemperature);

	RenderToTexture(target);
}

void FluidSimulator::Step()
{
	// Self-advection
	Advect(buffer, velocity, velocity);	Copy(velocity, buffer);
	Advect(buffer, density, velocity); Copy(density, buffer);
	Advect(buffer, temperature, velocity); Copy(temperature, buffer);

	// Add body forces
	Add(buffer, temperature, forces); Copy(temperature, buffer);
	Add(buffer, density, sources); Copy(density, buffer);

	Buoyancy(buffer, velocity, density, temperature); Copy(velocity, buffer);

	Project(buffer, velocity); Copy(velocity, buffer);
}

void FluidSimulator::InitializeTemperature()
{
	v3b->Reset();

	for (int x = 0; x < n; ++x)
		for (int y = 0; y < n; ++y)
			v3b->Get(x, y).x = 0;

	v3b->ToTexture(temperature);
}

