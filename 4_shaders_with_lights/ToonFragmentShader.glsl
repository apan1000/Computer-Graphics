#version 330 core

in vec3 fragmentPosition;
in vec3 fragmentColor;
in vec3 fragmentNormal;

// Ouput data
out vec3 color;

// http://www.tomdalling.com/blog/modern-opengl/08-even-more-lighting-directional-lights-spotlights-multiple-lights/
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

vec3 applyLight(Light light, vec3 toV, vec3 normal) {
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

float getColorValue(float intensity) {
	if (intensity > 0.90)
		intensity = 1;
	else if (intensity > 0.65)
		intensity = 0.75;
	else if (intensity > 0.35)
		intensity = 0.45;
	else if (intensity > 0.10)
		intensity = 0.20;
	else
		intensity = 0.0;

	return intensity;
}

void main() {
	// Toon shading
	vec3 toV = -normalize(fragmentPosition);
	vec3 normal = normalize(fragmentNormal);

	vec3 intensity = vec3(0.0);
	for(int i = 0; i < numLights; ++i) {
		intensity += applyLight(lights[i], toV, normal);
	}

	intensity.r = getColorValue(intensity.r);
	intensity.g = getColorValue(intensity.g);
	intensity.b = getColorValue(intensity.b);
	color = pow(intensity, vec3(1.0 / 2.2));;
}
