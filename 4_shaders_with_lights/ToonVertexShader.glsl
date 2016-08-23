#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition_modelspace;
//TODO: grab normal value from the application
layout(location = 1) in vec3 vertexNormal_modelspace;
//TODO: grab color value from the application
layout(location = 2) in vec3 vertexColor;

// Output data; will be interpolated for each fragment.
out vec3 fragmentPosition;
out vec3 fragmentColor;
out vec3 fragmentNormal;

uniform mat4 ModelTransform;
uniform mat4 Eye;
uniform mat4 Projection;

void main() {
	// Toon shading
	mat4 MVM = inverse(Eye) * ModelTransform;
	vec4 wPosition = MVM * vec4(vertexPosition_modelspace, 1);

	fragmentPosition = wPosition.xyz;
	gl_Position = Projection * wPosition;

	fragmentColor = vertexColor;

	mat4 invm = inverse(MVM);
	invm[0][3] = 0; invm[1][3] = 0; invm[2][3] = 0;
	mat4 NVM = transpose(invm);
	vec4 tnormal = vec4(vertexNormal_modelspace, 0.0);
	fragmentNormal = normalize(vec3(NVM * tnormal));
}
