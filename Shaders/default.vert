#version 130

// In
in vec3 vertexPosition;
in vec3 vertexTexCoord;

// Out
out vec3 texCoord;


void main()
{
	texCoord = vertexTexCoord;

	gl_Position = vec4(vertexPosition, 1.0);
}