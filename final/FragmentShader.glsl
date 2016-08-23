#version 330 core

in vec3 fragmentPosition;
in vec3 fragmentNormal;
in vec2 UV;

// Ouput data
layout(location = 0) out vec4 color;

#define MAX_LIGHTS 10
uniform int numLights;
uniform struct Light {
	vec4 position;
	vec3 color;
	float falloff;
	float ambientCoefficient;
	float coneAngle;
	vec3 coneDirection;
} lights[MAX_LIGHTS];

uniform mat4 Eye;
uniform sampler2D myTextureSampler;
uniform float opacity;

vec3 applyLight(Light light, vec3 toV, vec3 normal, vec3 fragmentColor) {
	vec3 toLight;
	float attenuation = 1.0;

	// Position in view space
	vec3 lightPosition = vec3(inverse(Eye) * vec4(light.position.xyz, 1));

	if(light.position.w == 0.0) { // Directional light
		toLight = normalize(lightPosition);
		attenuation = 1.0;
	} else { // Point light
		toLight = normalize(lightPosition - fragmentPosition);
		float distanceToLight = length(lightPosition - fragmentPosition);
		float radius = sqrt(1.0 / (light.falloff * 0.001));

		if(distanceToLight < radius) { // Cut off light at radius
			attenuation = 1.0 / (1.0 + light.falloff * pow(distanceToLight, 2));

			// Check if inside spot light cone
			vec3 coneDirection = vec3(inverse(Eye) * vec4(light.coneDirection, 1));
			float lightToSurfaceAngle = degrees(acos(dot(-toLight, normalize(coneDirection - lightPosition))));
			if(lightToSurfaceAngle > light.coneAngle) {
				attenuation = 0.0;
			}
		}
		else {
			attenuation = 0.0;
		}
	}

    vec3 h = normalize(toV + toLight);

	vec3 ambient = light.ambientCoefficient * fragmentColor * light.color;

	float specularCoefficient = pow(max(0.0, dot(h, normal)), 128.0);
	vec3 specular = specularCoefficient * light.color;

	float diffuseCoefficient = max(0.0, dot(normal, toLight));
	vec3 diffuse = diffuseCoefficient * light.color * fragmentColor;

	return ambient + attenuation*(diffuse + specular);
}

void main() {
	// Phong reflection model
	vec3 toV = -normalize(fragmentPosition);
	vec3 normal = normalize(fragmentNormal);
	
	vec3 fragmentColor = vec3(1.0, 1.0, 0.0);
	fragmentColor = texture(myTextureSampler, UV).rgb;

	vec3 intensity = vec3(0.0);
	for(int i = 0; i < numLights; ++i) {
		intensity += applyLight(lights[i], toV, normal, fragmentColor);
	}

	color = vec4(pow(intensity, vec3(1.0 / 2.2)), opacity); // Apply gamma correction
}
