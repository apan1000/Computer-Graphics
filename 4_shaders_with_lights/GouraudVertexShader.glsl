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

#define MAX_LIGHTS 10
uniform int numLights;
uniform struct Light {
	vec4 position;
	vec3 color;
	float attenuation;
	float ambientCoefficient;
	float coneAngle;
	vec3 coneDirection;
} lights[MAX_LIGHTS];
uniform vec4 uLight;

void main() {
	// Gouraud shading
	mat4 MVM = inverse(Eye) * ModelTransform;
	vec4 wPosition = MVM * vec4(vertexPosition_modelspace, 1);
	vec3 vertexPosition = wPosition.xyz;

	vec4 wLightPosition = MVM * vec4(uLight);
	vec3 lightPosition = vec3(Projection * wLightPosition);

	vec3 tolight = normalize(lightPosition - vertexPosition);
	vec3 toV = -normalize(vec3(vertexPosition));
	vec3 h = normalize(toV + tolight);

	mat4 invm = inverse(MVM);
	invm[0][3] = 0; invm[1][3] = 0; invm[2][3] = 0;
	mat4 NVM = transpose(invm);
	vec4 tnormal = vec4(vertexNormal_modelspace, 0.0);
	vec3 normal = normalize(vec3(NVM * tnormal));

	float specular = pow(max(0.0, dot(h, normal)), 64.0);
	float diffuse = max(0.0, dot(normal, tolight));
	vec3 intensity = vertexColor * diffuse + vec3(0.6, 0.6, 0.6)*specular;

	fragmentColor = pow(intensity, vec3(1.0 / 2.2));
	fragmentPosition = wPosition.xyz;
	fragmentNormal = vertexNormal_modelspace;
	gl_Position = Projection * wPosition;
}

