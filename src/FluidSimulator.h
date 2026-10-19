#pragma once

#include "StdAfx.h"

struct Vector3
{
	float x, y, z;
};

// CPU-side grid, used to upload static fields (obstacles)
class Vector3Buffer
{
public:
	Vector3Buffer(int n);

	void ToTexture(GLuint texture);
	void Reset();
	Vector3& Get(int x, int y);

private:
	vector<Vector3> buffer;
	int n;
};

struct FluidSettings
{
	int n = 512;                    // grid size (n x n cells)
	int jacobiIterations = 40;
	float dt = 0.1f;
	float smokeBuoyancy = 0.05f;    // sigma: hot smoke rises
	float smokeWeight = 0.0005f;    // kappa: dense smoke sinks
	float ambientTemperature = 0.0f;
	float vorticity = 0.5f;         // vorticity confinement strength (0 = off)
	float velocityDissipation = 0.999f;
	float dyeDissipation = 0.988f;
	float temperatureDissipation = 0.99f;
	bool obstacle = true;           // the circle in the centre
};

class FluidSimulator
{
public:
	FluidSimulator(const FluidSettings &settings);
	~FluidSimulator();

	void Update();
	void Reset();

	// Splats are in [0, 1] domain coordinates, radius as a fraction of the domain
	void AddForce(float x, float y, float dx, float dy, float radius);
	void AddHeat(float x, float y, float amount, float radius);
	void AddFluid(float x, float y, float r, float g, float b, float amount, float radius);

	void ToggleObstacle();
	void ToggleVorticity();

	GLuint GetVelocity() const { return velocity; }
	GLuint GetDensity() const { return density; }
	GLuint GetDye() const { return dye; }
	GLuint GetPressure() const { return pressure; }
	GLuint GetTemperature() const { return temperature; }
	GLuint GetObstacles() const { return obstacles; }
	int GetN() const { return settings.n; }
	const FluidSettings &GetSettings() const { return settings; }

private:
	FluidSettings settings;
	float storedVorticity;

	GLuint frameBufferHandle;

	// Programs
	GLuint advectProgram;
	GLuint splatProgram;
	GLuint divergenceProgram;
	GLuint jacobiProgram;
	GLuint clearProgram;
	GLuint pressureProgram;
	GLuint buoyancyProgram;
	GLuint curlProgram;
	GLuint vorticityProgram;

	GLuint currentProgram;

	// Textures
	GLuint velocity, density, temperature, dye;
	GLuint divergence, pressure, curl, buffer;
	GLuint obstacles;

	Vector3Buffer *v3b;

	void PostUpdate();

	// Utils
	void SetProgram(GLuint program);
	void RenderToTexture(GLuint target);
	void PassTexture(GLuint index, const string &textureName, GLuint texture);
	void PassDefaultVariables();
	void PassFloat(const char *name, float value);
	void Splat(GLuint &field, float x, float y, float r, float g, float b, float radius);

	// Steps
	void Advect(GLuint target, GLuint q, GLuint velocity, float dissipation);
	void Project(GLuint &velocity);
	void Divergence(GLuint target, GLuint velocity);
	void Jacobi(GLuint target, GLuint pressure, GLuint divergence);
	void Clear(GLuint target);
	void Pressure(GLuint target, GLuint velocity, GLuint pressure);
	void Buoyancy(GLuint target, GLuint velocity, GLuint density, GLuint temperature);
	void Curl(GLuint target, GLuint velocity);
	void Vorticity(GLuint target, GLuint velocity, GLuint curl);

	void InitalizeObstacles();
	void Step();
};
