#include "StdAfx.h"
#include "OpenGL.h"
#include "FluidSimulator.h"

#include <filesystem>

int windowX = 768;
int windowY = 768;

GLuint vaoHandle;
GLuint viewProgram;

FluidSimulator *fluidSimulator;

enum ViewMode { VIEW_DYE, VIEW_VELOCITY, VIEW_PRESSURE, VIEW_CLASSIC, VIEW_COUNT };
const char *viewNames[VIEW_COUNT] = { "dye", "velocity", "pressure", "classic" };

struct AppState
{
	int view = VIEW_DYE;
	bool paused = false;
	bool demo = false;
	float exposure = 1.0f;
	long frame = 0;

	// Mouse
	double mouseX = 0, mouseY = 0, prevMouseX = 0, prevMouseY = 0;
	bool mouseDown[3] = { false, false, false };
	bool shift = false;

	// Headless capture
	long maxFrames = -1;
	int dumpEvery = 0;
	string dumpDir;
	string screenshotFile;
	int screenshotCount = 0;
	int fps = 0;
} app;

// Inigo Quilez' cosine palette, gives nice saturated colours for the dye
static void Palette(float t, float &r, float &g, float &b)
{
	const float pi2 = 6.2831853f;
	r = 0.5f + 0.5f * cosf(pi2 * (t + 0.00f));
	g = 0.5f + 0.5f * cosf(pi2 * (t + 0.33f));
	b = 0.5f + 0.5f * cosf(pi2 * (t + 0.67f));
}

// Scripted "mouse" for headless runs: three pulsing smoke emitters at the
// bottom and an occasional sideways gust
static void DemoSplats(long frame)
{
	const float t = float(frame);

	for (int i = 0; i < 3; ++i)
	{
		float pulse = 0.5f + 0.5f * sinf(t * 0.035f + i * 1.9f);
		if (pulse < 0.3f) continue;

		float x = 0.25f + 0.25f * i + 0.05f * sinf(t * 0.021f + i * 2.1f);
		float y = 0.08f;

		float r, g, b;
		Palette(t * 0.0015f + i / 3.0f, r, g, b);

		fluidSimulator->AddFluid(x, y, r, g, b, 0.5f * pulse, 0.018f);
		fluidSimulator->AddHeat(x, y, 1.0f * pulse, 0.022f);
		fluidSimulator->AddForce(x, y, 0.025f * sinf(t * 0.05f + i * 1.3f), 0.04f * pulse, 0.025f);
	}

	// Gust from the side every few seconds
	long phase = frame % 240;
	if (phase > 120 && phase < 150)
	{
		bool fromLeft = (frame / 240) % 2 == 0;
		fluidSimulator->AddForce(fromLeft ? 0.08f : 0.92f, 0.55f + 0.1f * sinf(t * 0.01f), fromLeft ? 0.12f : -0.12f, 0.0f, 0.05f);
	}
}

static void MouseSplats()
{
	if (!app.mouseDown[0] && !app.mouseDown[1]) return;

	float x = float(app.mouseX / windowX);
	float y = float(1.0 - app.mouseY / windowY);
	float dx = float((app.mouseX - app.prevMouseX) / windowX);
	float dy = float(-(app.mouseY - app.prevMouseY) / windowY);

	if (x < 0 || x > 1 || y < 0 || y > 1) return;

	bool addFluid = app.mouseDown[1] || (app.mouseDown[0] && app.shift);

	if (addFluid)
	{
		// Source: coloured smoke
		float r, g, b;
		Palette(float(glfwGetTime()) * 0.1f, r, g, b);
		fluidSimulator->AddFluid(x, y, r, g, b, 1.0f, 0.025f);
	}
	else
	{
		// Force: push along the drag and warm the smoke up a bit
		const float strength = 6.0f;
		fluidSimulator->AddForce(x, y, dx * strength, dy * strength, 0.03f);
		fluidSimulator->AddHeat(x, y, 0.5f, 0.03f);
	}
}

static void SaveFrame(const string &fileName)
{
	if (SaveScreenshot(fileName, windowX, windowY))
		printf("Saved %s\n", fileName.c_str());
	else
		fprintf(stderr, "Failed to save %s\n", fileName.c_str());
}

void OnDisplay()
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, windowX, windowY);

	glUseProgram(viewProgram);

	glClearColor(0.0, 0.0, 0.0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	// Pass uniform variables into shader
	struct { const char *name; GLuint texture; } inputs[] = {
		{ "density", fluidSimulator->GetDensity() },
		{ "dye", fluidSimulator->GetDye() },
		{ "velocity", fluidSimulator->GetVelocity() },
		{ "pressure", fluidSimulator->GetPressure() },
		{ "temperature", fluidSimulator->GetTemperature() },
		{ "obstacles", fluidSimulator->GetObstacles() },
	};

	for (int i = 0; i < 6; ++i)
	{
		glActiveTexture(GL_TEXTURE0 + i);
		glBindTexture(GL_TEXTURE_2D, inputs[i].texture);
		glUniform1i(glGetUniformLocation(viewProgram, inputs[i].name), i);
	}

	glUniform2f(glGetUniformLocation(viewProgram, "resolution"), float(windowX), float(windowY));
	glUniform1i(glGetUniformLocation(viewProgram, "mode"), app.view);
	glUniform1f(glGetUniformLocation(viewProgram, "exposure"), app.exposure);

	glBindVertexArray(vaoHandle);
	glDrawArrays(GL_TRIANGLES, 0, 6);
}

void OnIdle()
{
	if (!app.paused)
	{
		if (app.demo) DemoSplats(app.frame);
		MouseSplats();

		fluidSimulator->Update();
		++app.frame;
	}

	app.prevMouseX = app.mouseX;
	app.prevMouseY = app.mouseY;
}

static void OnFramebufferSize(GLFWwindow *window, int w, int h)
{
	windowX = max(w, 1);
	windowY = max(h, 1);

	glViewport(0, 0, windowX, windowY);
}

static void OnMouseButton(GLFWwindow *window, int button, int action, int mods)
{
	// GLFW: 0 left, 1 right, 2 middle (GLUT had right at 2)
	if (button < 0 || button > 2) return;

	app.mouseDown[button] = (action == GLFW_PRESS);
	app.shift = (mods & GLFW_MOD_SHIFT) != 0;

	glfwGetCursorPos(window, &app.mouseX, &app.mouseY);
	app.prevMouseX = app.mouseX;
	app.prevMouseY = app.mouseY;
}

static void OnMouseMovement(GLFWwindow *window, double x, double y)
{
	// Cursor is in window coordinates, simulation in framebuffer pixels (HiDPI)
	int ww, wh;
	glfwGetWindowSize(window, &ww, &wh);
	app.mouseX = x * windowX / max(ww, 1);
	app.mouseY = y * windowY / max(wh, 1);
}

static void UpdateTitle(GLFWwindow *window)
{
	char title[256];
	const FluidSettings &s = fluidSimulator->GetSettings();
	snprintf(title, sizeof(title), "GPU Fluid - %dx%d - %s%s%s - %d fps", s.n, s.n, viewNames[app.view],
		s.vorticity > 0.0f ? "" : " - no vorticity", app.paused ? " - paused" : "", app.fps);
	glfwSetWindowTitle(window, title);
}

static void OnKeyboardInput(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (action != GLFW_PRESS) return;

	switch (key)
	{
	case GLFW_KEY_ESCAPE: glfwSetWindowShouldClose(window, GLFW_TRUE); break;
	case GLFW_KEY_1: app.view = VIEW_DYE; break;
	case GLFW_KEY_2: app.view = VIEW_VELOCITY; break;
	case GLFW_KEY_3: app.view = VIEW_PRESSURE; break;
	case GLFW_KEY_4: app.view = VIEW_CLASSIC; break;
	case GLFW_KEY_TAB: app.view = (app.view + 1) % VIEW_COUNT; break;
	case GLFW_KEY_SPACE: app.paused = !app.paused; break;
	case GLFW_KEY_R: fluidSimulator->Reset(); app.frame = 0; break;
	case GLFW_KEY_O: fluidSimulator->ToggleObstacle(); break;
	case GLFW_KEY_V: fluidSimulator->ToggleVorticity(); break;
	case GLFW_KEY_D: app.demo = !app.demo; break;
	case GLFW_KEY_EQUAL: case GLFW_KEY_KP_ADD: app.exposure *= 1.25f; break;
	case GLFW_KEY_MINUS: case GLFW_KEY_KP_SUBTRACT: app.exposure /= 1.25f; break;
	case GLFW_KEY_S:
		{
			char name[64];
			snprintf(name, sizeof(name), "gpufluid-%03d.png", app.screenshotCount++);
			OnDisplay();
			SaveFrame(name);
		}
		break;
	}

	UpdateTitle(window);
}

static void PrintUsage()
{
	printf(
		"GpuFluid - GPU based 2d fluid simulator\n\n"
		"Usage: GpuFluid [options]\n"
		"  --grid N            simulation grid N x N (default 512)\n"
		"  --size WxH          window size (default 768x768)\n"
		"  --iterations N      Jacobi pressure iterations (default 40)\n"
		"  --vorticity F       vorticity confinement, 0 = off (default 0.5)\n"
		"  --no-obstacle       start without the circle in the middle\n"
		"  --view NAME         dye | velocity | pressure | classic\n"
		"  --demo              scripted smoke emitters, no mouse needed\n"
		"  --frames N          quit after N simulation steps\n"
		"  --dump DIR          save every K-th frame as DIR/frame-XXXXX.png\n"
		"  --dump-every K      (default 1)\n"
		"  --screenshot FILE   save the last frame to FILE on exit\n");
}

int main(int argc, char **argv)
{
	FluidSettings settings;

	for (int i = 1; i < argc; ++i)
	{
		string arg = argv[i];
		bool hasValue = i + 1 < argc;

		if (arg == "--grid" && hasValue) settings.n = max(32, atoi(argv[++i]));
		else if (arg == "--size" && hasValue) sscanf(argv[++i], "%dx%d", &windowX, &windowY);
		else if (arg == "--iterations" && hasValue) settings.jacobiIterations = max(1, atoi(argv[++i]));
		else if (arg == "--vorticity" && hasValue) settings.vorticity = float(atof(argv[++i]));
		else if (arg == "--no-obstacle") settings.obstacle = false;
		else if (arg == "--demo") app.demo = true;
		else if (arg == "--frames" && hasValue) app.maxFrames = atol(argv[++i]);
		else if (arg == "--dump" && hasValue) { app.dumpDir = argv[++i]; if (!app.dumpEvery) app.dumpEvery = 1; }
		else if (arg == "--dump-every" && hasValue) app.dumpEvery = max(1, atoi(argv[++i]));
		else if (arg == "--screenshot" && hasValue) app.screenshotFile = argv[++i];
		else if (arg == "--view" && hasValue)
		{
			string name = argv[++i];
			for (int v = 0; v < VIEW_COUNT; ++v)
				if (name == viewNames[v]) app.view = v;
		}
		else { PrintUsage(); return arg == "--help" || arg == "-h" ? 0 : 1; }
	}

	if (!app.dumpDir.empty())
	{
		std::error_code ec;
		std::filesystem::create_directories(app.dumpDir, ec);
	}

	if (!glfwInit())
	{
		fprintf(stderr, "Failed to initialize GLFW\n");
		return 1;
	}

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);   // required on macOS

	GLFWwindow *window = glfwCreateWindow(windowX, windowY, "GPU Fluid", nullptr, nullptr);
	if (!window)
	{
		fprintf(stderr, "Failed to create an OpenGL 3.3 core window\n");
		glfwTerminate();
		return 1;
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(app.maxFrames > 0 ? 0 : 1);

	if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
	{
		fprintf(stderr, "Failed to load OpenGL functions\n");
		return 1;
	}

	printf("OpenGL %s, %s\n", glGetString(GL_VERSION), glGetString(GL_RENDERER));

	glfwGetFramebufferSize(window, &windowX, &windowY);

	viewProgram = LoadProgram("default", "view");

	vaoHandle = InitializeBuffers();

	fluidSimulator = new FluidSimulator(settings);

	// Bind handlers and start
	glfwSetFramebufferSizeCallback(window, OnFramebufferSize);
	glfwSetMouseButtonCallback(window, OnMouseButton);
	glfwSetCursorPosCallback(window, OnMouseMovement);
	glfwSetKeyCallback(window, OnKeyboardInput);
	UpdateTitle(window);

	double fpsTime = glfwGetTime();
	int fpsFrames = 0;

	while (!glfwWindowShouldClose(window))
	{
		OnIdle();
		OnDisplay();

		if (app.dumpEvery && !app.dumpDir.empty() && app.frame % app.dumpEvery == 0)
		{
			char name[512];
			snprintf(name, sizeof(name), "%s/frame-%05ld.png", app.dumpDir.c_str(), app.frame);
			SaveFrame(name);
		}

		bool last = app.maxFrames > 0 && app.frame >= app.maxFrames;
		if (last && !app.screenshotFile.empty())
			SaveFrame(app.screenshotFile);

		glfwSwapBuffers(window);
		glfwPollEvents();

		// FPS in the title
		++fpsFrames;
		double now = glfwGetTime();
		if (now - fpsTime > 1.0)
		{
			app.fps = int(fpsFrames / (now - fpsTime) + 0.5);
			if (app.maxFrames > 0) printf("frame %ld, %d fps\n", app.frame, app.fps);
			UpdateTitle(window);
			fpsTime = now;
			fpsFrames = 0;
		}

		if (last) break;
	}

	PrintLastError("exit");

	delete fluidSimulator;

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}
