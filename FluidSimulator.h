#pragma once

#include "StdAfx.h"

struct Vector3
{
	float x, y, z;
};

class Vector3Buffer
{
public:
	Vector3Buffer(int n);
	~Vector3Buffer();

	void ToTexture(GLuint texture);
	void Reset();
	Vector3& Get(int x, int y);

private:
	Vector3 *buffer;
	int n;
};

class FluidSimulator
{
public:
	FluidSimulator();
	~FluidSimulator();

	void Update();

	GLuint GetVelocity();
	GLuint GetDensity();
	GLuint GetObstacles();

private:
	GLuint frameBufferHandle;

	// Programs
	GLuint advectProgram;
	GLuint copyProgram;
	GLuint addProgram;
	GLuint projectProgram;
	GLuint divergenceProgram;
	GLuint jacobiProgram;
	GLuint clearProgram;
	GLuint pressureProgram;
	GLuint buoyancyProgram;

	GLuint currentProgram;

	// Consts
	int n;
	float dt;
	float source, force;
	float smokeBuoyancy;
	float smokeWeight;
	float ambientTemperature;

	// Textures
	GLuint velocity, density, temperature;
	GLuint divergence, pressure, buffer;
	GLuint forces, sources;
	GLuint obstacles;

	Vector3Buffer *v3b;

	void PreUpdate();
	void PostUpdate();

	// Utils
	void SetProgram(GLuint program);
	void RenderToTexture(GLuint target);
	void PassTexture(GLuint index, const string &textureName, GLuint texture);
	void PassDefaultVariables();

	// Steps
	void Advect(GLuint target, GLuint q, GLuint velocity);
	void Copy(GLuint target, GLuint q);
	void Add(GLuint target, GLuint q, GLuint q0);
	void Project(GLuint target, GLuint velocity);
	void Divergence(GLuint target, GLuint velocity);
	void Jacobi(GLuint target, GLuint pressure, GLuint divergence);
	void Clear(GLuint target);
	void Pressure(GLuint target, GLuint velocity, GLuint pressure);
	void Buoyancy(GLuint target, GLuint velocity, GLuint density, GLuint temperature);

	void UpdateForcesAndSources();

	void InitalizeObstacles();
	void InitializeTemperature();
	void Step();
};

