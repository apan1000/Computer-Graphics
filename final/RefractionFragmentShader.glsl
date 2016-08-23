#version 330 core

in vec3 fragmentPosition;
in vec3 fragmentNormal;
in vec2 UV;

smooth in vec3 RefractDir;

// Ouput data
out vec4 color;

uniform vec3 uLight;
uniform bool DrawSkyBox;

uniform samplerCube cubemap;
uniform sampler2D myTextureSampler;
uniform float opacity;

void main(){
	vec3 normal = normalize(fragmentNormal);
	// Assign color from environmental map(cubemap) texture	
	vec4 texColor = texture(cubemap, RefractDir);
	if(DrawSkyBox) {
		color = vec4(texColor.rgb, 1.0);
	} else {
		vec4 Kd = texture(myTextureSampler, UV);
		color = vec4(mix(Kd, texColor, 0.93).rgb, opacity);
	}
}