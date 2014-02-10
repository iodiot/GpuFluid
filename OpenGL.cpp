#include "StdAfx.h"
#include "OpenGL.h"

// Two triangles
const float positionData[] = {
	-1.0f, -1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,

	1.0f, 1.0f, 0.0f,
	1.0f, -1.0f, 0.0f,
	-1.0f, 1.0f, 0.0f,
};

const float texCoordData[] = {
	0.0f, 0.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,

	1.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f
};

GLuint Create2dTexture(int n)
{
	GLuint tid, sid;

	glGenTextures(1, &tid);
	glBindTexture(GL_TEXTURE_2D, tid);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, n, n, 0, GL_RGB, GL_FLOAT, 0); 

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	glTexParameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE, 0);
	glTexParameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE, 0);

	glGenSamplers(1, &sid);
	glSamplerParameteri(sid, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(sid, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindSampler(tid, sid);

	return tid;
}

GLuint Create3dTexture(int n)
{
	GLuint tid, sid;

	glGenTextures(1, &tid);
	glBindTexture(GL_TEXTURE_3D, tid);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, n, n, n, 0, GL_RGB, GL_FLOAT, 0); 

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 0);

	glTexParameteri(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE, 0);
	glTexParameteri(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE, 0);

	glGenSamplers(1, &sid);
	glSamplerParameteri(sid, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glSamplerParameteri(sid, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glBindSampler(tid, sid);

	return tid;
}

GLuint CreateShader(GLenum shaderType, const string &shaderSource)
{
	GLuint shader = glCreateShader(shaderType);

	const char *sourseData = shaderSource.c_str();
	glShaderSource(shader, 1, &sourseData, 0);

	glCompileShader(shader);

	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *infoLog = new GLchar[infoLogLength + 1];
		glGetShaderInfoLog(shader, infoLogLength, 0, infoLog);

		char str[0xff];
		sprintf(str, "Compile failure in %s shader:\n%s\n", shaderType == GL_VERTEX_SHADER ? "vertex" : "fragment", infoLog);
		OutputDebugStringA(str);
		delete[] infoLog;
	}

	return shader;
}

GLuint CreateProgram(const vector<GLuint> &shaders)
{
	GLuint program = glCreateProgram();

	vector<GLuint>::const_iterator it;
	for(it = shaders.begin(); it != shaders.end(); ++it)
		glAttachShader(program, *it);

	glBindAttribLocation(program, 0, "vertexPosition");
	glBindAttribLocation(program, 1, "vertexTexCoord");

	glLinkProgram(program);

	// Verify the link status
	GLint status;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint infoLogLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

		GLchar *infoLog = new GLchar[infoLogLength + 1];
		glGetProgramInfoLog(program, infoLogLength, 0, infoLog);

		char str[0xff];
		sprintf(str, "Link failure: %s\n", infoLog);
		OutputDebugStringA(str);
		delete[] infoLog;
	}

	return program;
}

string LoadShaderAsString(const string &fileName)
{
	string line, result;
	ifstream file;

	file.open(fileName);

	if (file.is_open())
	{
		while(!file.eof())
		{
			getline(file ,line); 
			result += line + '\n';
		}
	}

	file.close();

	return result;
}

float NextFloat()
{
	return float(rand()) / float(RAND_MAX);
}

GLuint LoadProgram(const string &vertexShader, const string &fragmentShader)
{
	vector<GLuint> shaders;

	shaders.push_back(CreateShader(GL_VERTEX_SHADER, LoadShaderAsString("Shaders/" + vertexShader + ".vert")));
	shaders.push_back(CreateShader(GL_FRAGMENT_SHADER, LoadShaderAsString("Shaders/" + fragmentShader + ".frag")));

	GLuint handle = CreateProgram(shaders);

	for_each(shaders.begin(), shaders.end(), glDeleteShader);

	return handle;
}

GLuint InitializeBuffers()
{
	GLuint handle;

	// Create buffer objects
	GLuint vboHandles[2];
	glGenBuffers(2, vboHandles);
	GLuint positionBufferHandle = vboHandles[0];
	GLuint texCoordBufferHandle = vboHandles[1];

	// Populate position buffer
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), positionData, GL_STATIC_DRAW);

	// Populate tex coord buffer
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, 12 * sizeof(float), texCoordData, GL_STATIC_DRAW);

	// Create and set-up vertex array object
	glGenVertexArrays(1, &handle);
	glBindVertexArray(handle);

	// Enable vertex attribute arrays
	glEnableVertexAttribArray(0);		// positions
	glEnableVertexAttribArray(1);		// tex coords

	// Map index 0 to position buffer
	glBindBuffer(GL_ARRAY_BUFFER, positionBufferHandle);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (GLubyte *) 0);

	// Map index 1 to tex coord buffer buffer
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBufferHandle);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (GLubyte *) 0);

	return handle;
}

void PrintLastError()
{
	GLenum code = glGetError(); 

	if (code != GL_NO_ERROR) 
	{
    const GLubyte *message = gluErrorString(code);
		char str[0xff];
		sprintf(str, "OpenGL error: %s\n", message);
		OutputDebugStringA(str);
	}
}