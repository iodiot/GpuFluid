#version 330 core

// In
layout(location = 0) in vec3 vertexPosition;
layout(location = 1) in vec2 vertexTexCoord;

// Out
out vec2 texCoord;

void main()
{
	texCoord = vertexTexCoord;

	gl_Position = vec4(vertexPosition, 1.0);
}
