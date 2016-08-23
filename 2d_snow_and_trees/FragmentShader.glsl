#version 330 core

uniform vec4 color = vec4(1.0, 1.0, 1.0, 1.0); // Make color white

// Ouput color
out vec4 outColor;

void main() {
	outColor = color;
}
