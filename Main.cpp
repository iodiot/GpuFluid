#include "StdAfx.h"
#include "OpenGL.h"
#include "FluidSimulator.h"

int windowX = 512;
int windowY = 512;

int mouseX, mouseY, originalMouseX, originalMouseY;
int mouseDown[3];

GLuint vaoHandle;
GLuint viewProgram;

FluidSimulator *fluidSimulator;


void OnDisplay()
{
	glUseProgram(viewProgram);

	glClearColor(0.0, 0.0, 0.0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	// Pass uniform variables into shader
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, fluidSimulator->GetDensity());
	GLuint location = glGetUniformLocation(viewProgram, "density");
	glUniform1i(location, 0);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, fluidSimulator->GetVelocity());
	location = glGetUniformLocation(viewProgram, "velocity");
	glUniform1i(location, 1);

	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, fluidSimulator->GetObstacles());
	location = glGetUniformLocation(viewProgram, "obstacles");
	glUniform1i(location, 2);

	location = glGetUniformLocation(viewProgram, "resolution");
	glUniform2fv(location, 1, glm::value_ptr(vec2(windowX, windowY)));

	glBindVertexArray(vaoHandle);
	glDrawArrays(GL_TRIANGLES, 0, 6);
	 
	glutSwapBuffers();
}

void OnReshape(int w, int h)
{
	windowX = w;
	windowY = h;

	glViewport(0, 0, w, h);
}

void OnIdle()
{
	fluidSimulator->Update();

	glutPostRedisplay();
}

void OnMouseClick(int button, int state, int x, int y)
{
	originalMouseX = mouseX = x;
	originalMouseX = mouseY = y;

	mouseDown[button] = (state == GLUT_DOWN);
}

void OnMouseMovement(int x, int y)
{
	mouseX = x;
	mouseY = y;
}

void OnKeyboardInput(unsigned char key, int x, int y)
{
	switch (key)
	{
	case 27: glutLeaveMainLoop();	break;
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	int argc = 0;
	glutInit(&argc, 0);

	glutInitDisplayMode(GLUT_DOUBLE | GLUT_ALPHA | GLUT_DEPTH);
	glutInitContextVersion(3, 1);
	glutInitContextProfile(GLUT_CORE_PROFILE);
	glutInitWindowSize(windowX, windowY); 
	glutInitWindowPosition(300, 200);

	glutCreateWindow("GPU Fluid");

	LoadFunctions();

	viewProgram = LoadProgram("default", "view");

	vaoHandle = InitializeBuffers();

	fluidSimulator = new FluidSimulator();

	// Bind handlers and start
	glutDisplayFunc(OnDisplay); 
	glutReshapeFunc(OnReshape);
	glutIdleFunc(OnIdle);
	glutMouseFunc(OnMouseClick);
	glutKeyboardFunc(OnKeyboardInput);

	glutMotionFunc(OnMouseMovement);

	glutMainLoop();

	delete fluidSimulator;

	return 0;
}