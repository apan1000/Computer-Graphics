#version 330 core

in vec3 fragmentPosition;
in vec3 fragmentColor;
in vec3 fragmentNormal;

// Ouput data
out vec3 color;

uniform vec4 uLight;

void main() {
	// Gouraud shading
	color = fragmentColor;
}
