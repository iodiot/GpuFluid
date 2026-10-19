#pragma once

#include "StdAfx.h"

GLuint InitializeBuffers();
GLuint Create2dTexture(int n);
GLuint CreateShader(GLenum shaderType, const string &shaderSource, const string &name);
GLuint CreateProgram(const vector<GLuint> &shaders);
string LoadShaderAsString(const string &fileName);
float NextFloat();
GLuint LoadProgram(const string &vertexShader, const string &fragmentShader);
void PrintLastError(const char *where);
bool SaveScreenshot(const string &fileName, int width, int height);
