#version 330 core

// Input vertex data, different for all executions of this shader.
layout(location = 0) in vec3 vertexPosition_modelspace;
layout(location = 1) in vec3 vertexNormal_modelspace;
layout(location = 3) in vec2 vertexUV;
layout(location = 4) in vec3 tangents;

// Output data; will be interpolated for each fragment.
out vec3 fragmentPosition;
out vec3 fragmentNormal;
out vec2 UV;

uniform mat4 ModelTransform;
uniform mat4 Eye;
uniform mat4 Projection;

uniform sampler2D displacementSampler;

void main() {
	mat4 MVM = inverse(Eye) * ModelTransform;

	vec4 newVertexPos;
	vec4 dv;
	float df;
	
	dv = texture( displacementSampler, vertexUV );
	
	df = 0.30*dv.x + 0.59*dv.y + 0.11*dv.z;
	
	newVertexPos = vec4(vertexNormal_modelspace * df * 0.5, 0.0) + vec4(vertexPosition_modelspace,1.0);

	fragmentPosition = (MVM * newVertexPos).xyz;
	gl_Position = Projection * MVM * newVertexPos;

	//transpose of inversed model view matrix
	mat4 invm = inverse(MVM);
	invm[0][3] = 0; invm[1][3] = 0;	invm[2][3] = 0;
	mat4 NVM = transpose(invm);
	vec4 tnormal = vec4(vertexNormal_modelspace * df * 0.5, 0.0) + vec4(vertexNormal_modelspace,1.0);;
	fragmentNormal = vec3(NVM * tnormal);
	UV = vertexUV;
}