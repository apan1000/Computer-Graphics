#version 330 core

in vec3 fragmentPosition;
in vec3 fragmentNormal;
// Interpolated values from the vertex shaders
in vec3 fragmentColor;

// Ouput data
out vec3 color;

uniform vec3 uLight;

void main(){
	vec3 toLight = normalize(uLight - fragmentPosition);
	vec3 toV = -normalize(vec3(fragmentPosition));
	vec3 h = normalize(toV + toLight);
	vec3 normal = normalize(fragmentNormal);

	float specular = pow(max(0.0, dot(h, normal)), 64.0);
	float diffuse = max(0.0, dot(normal, toLight));
	vec3 intensity = vec3(0.1,0.1,0.1) + fragmentColor * diffuse + vec3(0.45,0.45,0.45) * specular;

	color = intensity;
}