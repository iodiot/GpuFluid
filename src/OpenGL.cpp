#include "StdAfx.h"
#include "OpenGL.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

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
	GLuint tid;

	glGenTextures(1, &tid);
	glBindTexture(GL_TEXTURE_2D, tid);

	// RGBA32F: RGB32F is not a guaranteed colour-renderable format in core GL
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, n, n, 0, GL_RGBA, GL_FLOAT, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	return tid;
}

GLuint CreateShader(GLenum shaderType, const string &shaderSource, const string &name)
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

		vector<GLchar> infoLog(infoLogLength + 1);
		glGetShaderInfoLog(shader, infoLogLength, 0, infoLog.data());

		fprintf(stderr, "Compile failure in %s:\n%s\n", name.c_str(), infoLog.data());
		exit(EXIT_FAILURE);
	}

	return shader;
}

GLuint CreateProgram(const vector<GLuint> &shaders)
{
	GLuint program = glCreateProgram();

	for (GLuint shader : shaders)
		glAttachShader(program, shader);

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

		vector<GLchar> infoLog(infoLogLength + 1);
		glGetProgramInfoLog(program, infoLogLength, 0, infoLog.data());

		fprintf(stderr, "Link failure: %s\n", infoLog.data());
		exit(EXIT_FAILURE);
	}

	return program;
}

string LoadShaderAsString(const string &fileName)
{
	// Look next to the working directory first, then in the source tree
	const string candidates[] = { "Shaders/" + fileName, string(GPUFLUID_SHADER_DIR) + fileName };

	for (const string &path : candidates)
	{
		ifstream file(path);

		if (file.is_open())
		{
			stringstream ss;
			ss << file.rdbuf();
			return ss.str();
		}
	}

	fprintf(stderr, "Can't find shader %s\n", fileName.c_str());
	exit(EXIT_FAILURE);
}

float NextFloat()
{
	return float(rand()) / float(RAND_MAX);
}

GLuint LoadProgram(const string &vertexShader, const string &fragmentShader)
{
	vector<GLuint> shaders;

	shaders.push_back(CreateShader(GL_VERTEX_SHADER, LoadShaderAsString(vertexShader + ".vert"), vertexShader + ".vert"));
	shaders.push_back(CreateShader(GL_FRAGMENT_SHADER, LoadShaderAsString(fragmentShader + ".frag"), fragmentShader + ".frag"));

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
	glBufferData(GL_ARRAY_BUFFER, sizeof(positionData), positionData, GL_STATIC_DRAW);

	// Populate tex coord buffer
	glBindBuffer(GL_ARRAY_BUFFER, texCoordBufferHandle);
	glBufferData(GL_ARRAY_BUFFER, sizeof(texCoordData), texCoordData, GL_STATIC_DRAW);

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

void PrintLastError(const char *where)
{
	GLenum code = glGetError();

	if (code != GL_NO_ERROR)
		fprintf(stderr, "OpenGL error 0x%04x at %s\n", code, where);
}

bool SaveScreenshot(const string &fileName, int width, int height)
{
	vector<unsigned char> pixels(width * height * 3);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());

	// OpenGL rows go bottom-up, PNG rows top-down
	stbi_flip_vertically_on_write(1);
	return stbi_write_png(fileName.c_str(), width, height, 3, pixels.data(), width * 3) != 0;
}
