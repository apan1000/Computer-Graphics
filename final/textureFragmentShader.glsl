// Fredrik Berglund 2016

#version 330 core

in vec2 UV;

out vec4 color;

uniform sampler2D renderedTexture;
uniform sampler2D replaceTexture;
uniform bool isChromaKey;
uniform bool isPixelated;
uniform float pixels;
uniform float frameHeight;
uniform float frameWidth;
uniform float frameRatio;

// Chroma keying: https://www.shadertoy.com/view/4dX3WN
vec3 rgb2hsv(vec3 rgb) {
	float Cmax = max(rgb.r, max(rgb.g, rgb.b));
	float Cmin = min(rgb.r, min(rgb.g, rgb.b));
	float delta = Cmax - Cmin;

	vec3 hsv = vec3(0., 0., Cmax);
	
	if (Cmax > Cmin)
	{
		hsv.y = delta / Cmax;

		if (rgb.r == Cmax)
			hsv.x = (rgb.g - rgb.b) / delta;
		else
		{
			if (rgb.g == Cmax)
				hsv.x = 2. + (rgb.b - rgb.r) / delta;
			else
				hsv.x = 4. + (rgb.r - rgb.g) / delta;
		}
		hsv.x = fract(hsv.x / 6.);
	}
	return hsv;
}

float chromaKey(vec3 color) {
	vec3 colorToReplace = vec3(0.0f, 1.0f, 0.0f);
	vec3 weights = vec3(4., 1., 2.);

	vec3 hsv = rgb2hsv(color);
	vec3 target = rgb2hsv(colorToReplace);
	float dist = length(weights * (target - hsv));
	return 1. - clamp(3. * dist - 1.5, 0., 1.);
}

void main() {
	vec2 Coord = UV;

	if(isPixelated) {
		float ratio = frameWidth/frameHeight;
		float dx = 10.0 * (1.0 / pixels);
		float dy = 10.0 * ratio * (1.0 / pixels);
		Coord = vec2(dx * floor(UV.x / dx), dy * floor(UV.y / dy));
	}

	color = texture( renderedTexture, Coord );

	if(isChromaKey) {
		vec4 replaceColor = texture( replaceTexture, UV );
		float incrustation = chromaKey(color.rgb);

		color = mix(color, replaceColor, incrustation);
	}
}