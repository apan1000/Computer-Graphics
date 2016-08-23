/*
	Fredrik Berglund 2016
*/

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/affine.hpp>
#include <common/geometry.hpp>
#include <common/arcball.hpp>

int const OBJ_COUNT = 3;
int const LIGHT_COUNT = 6;

float g_groundSize = 100.0f;
float g_groundY = -2.5f;

GLuint lightLocGround[LIGHT_COUNT*6], lightLocObjects[OBJ_COUNT][LIGHT_COUNT*6], numLightsLocs[1+OBJ_COUNT];

// View properties
glm::mat4 Projection;
float windowWidth = 1024.0f;
float windowHeight = 768.0f;
int frameBufferWidth = 0;
int frameBufferHeight = 0;
float fov = 45.0f;
float fovy = fov;

// Model properties
Model ground, objects[3];
glm::mat4 skyRBT;
glm::mat4 eyeRBT;
const glm::mat4 worldRBT = glm::mat4(1.0f);
glm::mat4 objectRBTs[OBJ_COUNT] = {
	glm::scale(2.1f, 2.1f, 2.1f) * glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(0.0f, 1.0f, 0.0f)) * glm::translate(glm::vec3(0.0f, -2.0f, -8.0f)),
	glm::scale(7.0f, 7.0f, 7.0f) * glm::rotate(glm::mat4(1.0f), 10.0f, glm::vec3(1.0f, 0.0f, 0.0f)) * glm::translate(glm::vec3(0.1f, 0.05f, 0.04f)),
	glm::scale(0.01f, 0.01f, 0.01f) * glm::rotate(glm::mat4(1.0f), 60.0f, glm::vec3(0.0f, 0.0f, 1.0f)) * glm::translate(glm::vec3(20.0f, -50.0f, 0.0f))
};
glm::mat4 arcballRBT = glm::mat4(1.0f);
glm::mat4 aFrame;

// Mouse interaction
bool MOUSE_LEFT_PRESS = false; bool MOUSE_MIDDLE_PRESS = false; bool MOUSE_RIGHT_PRESS = false;

// Transformation
glm::mat4 m = glm::mat4(1.0f);

// Manipulation index
int object_index = 0; int view_index = 0; int sky_type = 0;

// Arcball manipulation
Model arcBall;
float arcBallScreenRadius = 0.25f * min(windowWidth, windowHeight);
float arcBallScale = 0.01f; float ScreenToEyeScale = 0.01f;
float prev_x = 0.0f; float prev_y = 0.0f;

// Time
double then = glfwGetTime();
double now = then;

// Lights
// http://www.tomdalling.com/blog/modern-opengl/08-even-more-lighting-directional-lights-spotlights-multiple-lights/
struct Light {
	glm::vec4 position;
	glm::vec3 color;
	float falloff;
	float ambientCoefficient;
	float coneAngle;
	glm::vec3 coneDirection;
};
std::vector<Light> lights;
std::vector<std::string> lightProperties = {"position","color","falloff","ambientCoefficient","coneAngle","coneDirection"};
float lightMove = 0;

static bool non_ego_cube_manipulation()
{
	return object_index != 0 && view_index != object_index;
}

static bool use_arcball()
{
	return (object_index == 0 && sky_type == 0) || non_ego_cube_manipulation();
}

static void window_size_callback(GLFWwindow* window, int width, int height)
{
	// Get resized size and set to current window
	windowWidth = (float)width;
	windowHeight = (float)height;

	// glViewport accept pixel size, please use glfwGetFramebufferSize rather than window size.
	// window size != framebuffer size
	glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
	glViewport(0, 0, frameBufferWidth, frameBufferHeight);

	arcBallScreenRadius = 0.25f * min(frameBufferWidth, frameBufferHeight);

	if (frameBufferWidth >= frameBufferHeight)
	{
		fovy = fov;
	}
	else {
		const float RAD_PER_DEG = 0.5f * glm::pi<float>() / 180.0f;
		fovy = atan2(sin(fov * RAD_PER_DEG) * (float)frameBufferHeight / (float)frameBufferWidth, cos(fov * RAD_PER_DEG)) / RAD_PER_DEG;
	}

	// Update projection matrix
	Projection = glm::perspective(fov, windowWidth / windowHeight, 0.1f, 100.0f);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	MOUSE_LEFT_PRESS |= (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS);
	MOUSE_RIGHT_PRESS |= (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS);
	MOUSE_MIDDLE_PRESS |= (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS);

	MOUSE_LEFT_PRESS &= !(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE);
	MOUSE_RIGHT_PRESS &= !(button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE);
	MOUSE_MIDDLE_PRESS &= !(button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE);

	if (action == GLFW_RELEASE) {
		prev_x = 0.0f; prev_y = 0.0f;
	}
}

void setWrtFrame()
{
	switch (object_index)
	{
	case 0:
		// world-sky: transFact(worldRBT) * linearFact(skyRBT), sky-sky: transFact(skyRBT) * linearFact(skyRBT)
		aFrame = (sky_type == 0) ? linearFact(skyRBT) : skyRBT;
		break;
	case 1:
		aFrame = transFact(objectRBTs[0]) * linearFact(eyeRBT);
		break;
	}
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (view_index != 0 && object_index == 0) return;
	// Convert mouse pointer into screen space. (http://gamedev.stackexchange.com/questions/83570/why-is-the-origin-in-computer-graphics-coordinates-at-the-top-left)
	xpos = xpos * ((float)frameBufferWidth / windowWidth);
	ypos = (float)frameBufferHeight - ypos * ((float)frameBufferHeight / windowHeight) - 1.0f;

	double dx_t = xpos - prev_x;
	double dy_t = ypos - prev_y;
	double dx_r = xpos - prev_x;
	double dy_r = ypos - prev_y;

	if (view_index == 0 && object_index == 0)
	{
		if (sky_type == 0) { dx_t = -dx_t; dy_t = -dy_t; dx_r = -dx_r; dy_r = -dy_r; }
		else { dx_r = -dx_r; dy_r = -dy_r; }
	}

	if (MOUSE_LEFT_PRESS)
	{
		if (prev_x - 1e-16< 1e-8 && prev_y - 1e-16 < 1e-8) {
			prev_x = (float)xpos; prev_y = (float)ypos;
			return;
		}

		if (use_arcball())
		{
			// 1. Get eye coordinate of arcball and compute its screen coordinate
			glm::vec4 arcball_eyecoord = glm::inverse(eyeRBT) * arcballRBT * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			glm::vec2 arcballCenter = eye_to_screen(
				glm::vec3(arcball_eyecoord),
				Projection,
				frameBufferWidth,
				frameBufferHeight
				);

			// compute z index
			glm::vec2 p1 = glm::vec2(prev_x, prev_y) - arcballCenter;
			glm::vec2 p2 = glm::vec2(xpos, ypos) - arcballCenter;

			glm::vec3 v1 = glm::normalize(glm::vec3(p1.x, p1.y, sqrt(max(0.0f, pow(arcBallScreenRadius, 2) - pow(p1.x, 2) - pow(p1.y, 2)))));
			glm::vec3 v2 = glm::normalize(glm::vec3(p2.x, p2.y, sqrt(max(0.0f, pow(arcBallScreenRadius, 2) - pow(p2.x, 2) - pow(p2.y, 2)))));

			glm::quat w1, w2;
			// 2. Compute arcball rotation (Chatper 8)
			if (object_index == 0 && view_index == 0 && sky_type == 0) { w1 = glm::quat(0.0f, -v1); w2 = glm::quat(0.0f, v2); }
			else { w1 = glm::quat(0.0f, v2); w2 = glm::quat(0.0f, -v1); }

			// Arcball: axis k and 2*theta (Chatper 8)
			glm::quat w = w1 * w2;
			m = glm::toMat4(w);
		}
		else // ego motion
		{
			glm::quat xRotation = glm::angleAxis((float)-dy_r * 0.1f, glm::vec3(1.0f, 0.0f, 0.0f));
			glm::quat yRotation = glm::angleAxis((float)dx_r * 0.1f, glm::vec3(0.0f, 1.0f, 0.0f));

			glm::quat w = yRotation * xRotation;
			m = glm::toMat4(w);
		}

		// Apply transformation with auxiliary frame
		setWrtFrame();
		if (object_index == 0) { skyRBT = aFrame * m * glm::inverse(aFrame) * skyRBT; }
		else { objectRBTs[0] = aFrame * m * glm::inverse(aFrame) * objectRBTs[0]; }

		prev_x = (float)xpos; prev_y = (float)ypos;
	}
}


void toggleEyeMode()
{
	view_index = (view_index + 1) % 2;
	if (view_index == 0) {
		std::cout << "Using sky view" << std::endl;
	}
	else {
		std::cout << "Using object " << view_index << " view" << std::endl;
	}
}

void cycleManipulation()
{
	object_index = (object_index + 1) % 2;
	if (object_index == 0) {
		std::cout << "Manipulating sky frame" << std::endl;
	}
	else {
		std::cout << "Manipulating object " << object_index << std::endl;
	}
}

void cycleSkyAMatrix()
{
	if (object_index == 0 && view_index == 0) {
		sky_type = (sky_type + 1) % 2;
		if (sky_type == 0) {
			std::cout << "world-sky" << std::endl;
		}
		else {
			std::cout << "sky-sky" << std::endl;
		}
	}
	else {
		std::cout << "Unable to change sky mode" << std::endl;
	}
}

static void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	glm::mat4 m;
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_H:
			std::cout << "CS380 Homework Assignment 2" << std::endl;
			std::cout << "keymaps:" << std::endl;
			std::cout << "h\t\t Help command" << std::endl;
			std::cout << "v\t\t Change eye matrix" << std::endl;
			std::cout << "o\t\t Change current manipulating object" << std::endl;
			std::cout << "m\t\t Change auxiliary frame between world-sky and sky-sky" << std::endl;
			break;
		case GLFW_KEY_V:
			
			break;
		case GLFW_KEY_O:
			
			break;
		case GLFW_KEY_M:
			
			break;
		default:
			break;
		}
	}
}

std::string lightUniformString(int index, std::string property)
{
	std::ostringstream oss;
	oss << "lights[" << index << "]." << property;

	return oss.str();
}

void setLightUniformLocs(GLuint* uniformArray, Model& model)
{
	for (int i = 0; i < lights.size(); ++i)
	{
		uniformArray[i * 6] = glGetUniformLocation(model.GLSLProgramID, lightUniformString(i, "position").c_str());
		uniformArray[i * 6 + 1] = glGetUniformLocation(model.GLSLProgramID, lightUniformString(i, "color").c_str());
		uniformArray[i * 6 + 2] = glGetUniformLocation(model.GLSLProgramID, lightUniformString(i, "falloff").c_str());
		uniformArray[i * 6 + 3] = glGetUniformLocation(model.GLSLProgramID, lightUniformString(i, "ambientCoefficient").c_str());
		uniformArray[i * 6 + 4] = glGetUniformLocation(model.GLSLProgramID, lightUniformString(i, "coneAngle").c_str());
		uniformArray[i * 6 + 5] = glGetUniformLocation(model.GLSLProgramID, lightUniformString(i, "coneDirection").c_str());
	}
}

void setLightUniforms(GLuint* uniformArray)
{
	for (int i = 0; i < lights.size(); ++i)
	{
		glUniform4fv(uniformArray[i * 6], 1, glm::value_ptr(lights[i].position));
		glUniform3fv(uniformArray[i * 6 + 1], 1, glm::value_ptr(lights[i].color));
		glUniform1f(uniformArray[i * 6 + 2], lights[i].falloff);
		glUniform1f(uniformArray[i * 6 + 3], lights[i].ambientCoefficient);
		glUniform1f(uniformArray[i * 6 + 4], lights[i].coneAngle);
		glUniform3fv(uniformArray[i * 6 + 5], 1, glm::value_ptr(lights[i].coneDirection));
	}
}

int main(void)
{
	// Initialise GLFW
	if (!glfwInit())
	{
		return -1;
	}

	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// Open a window and create its OpenGL context
	window = glfwCreateWindow((int)windowWidth, (int)windowHeight, "Homework 4 | Fredrik Berglund 20166029", NULL, NULL);
	if (window == NULL) {
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = true; // Needed for core profile
	if (glewInit() != GLEW_OK) {
		return -1;
	}

	// Ensure we can capture the escape key being pressed below
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
	glfwSetWindowSizeCallback(window, window_size_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetKeyCallback(window, keyboard_callback);

	glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);

	// Clear with sky color
	glClearColor((GLclampf)(128. / 255.), (GLclampf)(200. / 255.), (GLclampf)(255. / 255.), (GLclampf) 0.);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);	


	Projection = glm::perspective(fov, windowWidth / windowHeight, 0.1f, 100.0f);
	skyRBT = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.45, 5.5));

	aFrame = linearFact(skyRBT);

	// initial eye frame = sky frame;
	eyeRBT = skyRBT;

	// Initialize Ground Model
	ground = Model();
	init_ground(ground);
	ground.initialize(DRAW_TYPE::ARRAY, "PhongVertexShader.glsl", "PhongFragmentShader.glsl");
	ground.set_projection(&Projection);
	ground.set_eye(&eyeRBT);
	glm::mat4 groundRBT = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, g_groundY, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(g_groundSize, 1.0f, g_groundSize));
	ground.set_model(&groundRBT);

	//TODO: Initialize model by loading .obj file
	std::vector<std::pair<std::string, std::string>> shaders = {
		std::make_pair("PhongVertexShader.glsl", "PhongFragmentShader.glsl"),
		std::make_pair("ToonVertexShader.glsl", "ToonFragmentShader.glsl"),
		std::make_pair("FlatVertexShader.glsl", "FlatFragmentShader.glsl")
	};

	init_obj(objects[0], "cat.obj", glm::vec3(0.15, 0.15, 0.15));
	init_obj(objects[1], "bunny.obj", glm::vec3(0.85, 0.85, 0.85));
	init_obj(objects[2], "gipshand.obj", glm::vec3(0.7, 0.55, 0.2));

	for (int i = 0; i < OBJ_COUNT; ++i)
	{
		objects[i].initialize(DRAW_TYPE::ARRAY, shaders[i].first.c_str(), shaders[i].second.c_str());
		objects[i].set_projection(&Projection);
		objects[i].set_eye(&eyeRBT);
		objects[i].set_model(&objectRBTs[i]);
	}

	arcBall = Model();
	init_sphere(arcBall);
	arcBall.initialize(DRAW_TYPE::INDEX, "PhongVertexShader.glsl", "PhongFragmentShader.glsl");

	arcBall.set_projection(&Projection);
	arcBall.set_eye(&eyeRBT);
	arcBall.set_model(&arcballRBT);

	// Setup of lights
	Light dirLight;
	dirLight.position = glm::vec4(15.0f, 20.0f, -45.0f, 0.0f);
	dirLight.color = glm::vec3(0.15f, 0.08f, 0.008f);
	dirLight.ambientCoefficient = 0.06f;

	Light pointLight;
	pointLight.position = glm::vec4(0.0f, 0.8f, -0.1f, 1.0f);
	pointLight.color = glm::vec3(0.0f, 1.0f, 1.0f);
	pointLight.falloff = 0.6f;
	pointLight.ambientCoefficient = 0.0f;
	pointLight.coneAngle = 180.0f;

	Light pointLight2;
	pointLight2.position = glm::vec4(0.0f, 0.8f, 0.0f, 1.0f);
	pointLight2.color = glm::vec3(0.64f, 0.97f, 0.39f);
	pointLight2.falloff = 0.07f;
	pointLight2.ambientCoefficient = 0.0f;
	pointLight2.coneAngle = 180.0f;

	Light spotLight;
	spotLight.position = glm::vec4(0.0f, 6.0f, 0.0f, 1.0f);
	spotLight.color = glm::vec3(1.0f, 0.0f, 0.0f);
	spotLight.falloff = 0.01f;
	spotLight.ambientCoefficient = 0.0f;
	spotLight.coneAngle = 8.0f;
	spotLight.coneDirection = glm::vec3(0.0f, 0.0f, 0.0f);

	Light spotLight2;
	spotLight2.position = glm::vec4(0.0f, 0.0f, 5.0f, 1.0f);
	spotLight2.color = glm::vec3(0.85f, 0.0f, 0.85f);
	spotLight2.falloff = 0.001f;
	spotLight2.ambientCoefficient = 0.0f;
	spotLight2.coneAngle = 4.0f;
	spotLight2.coneDirection = glm::vec3(0.0f, 2.0f, 0.0f);

	Light spotLight3;
	spotLight3.position = glm::vec4(3.0f, 0.0f, 1.0f, 1.0f);
	spotLight3.color = glm::vec3(0.6f, 0.6f, 0.6f);
	spotLight3.falloff = 0.01f;
	spotLight3.ambientCoefficient = 0.0f;
	spotLight3.coneAngle = 30.0f;
	spotLight2.coneDirection = glm::vec3(0.0f, 0.3f, 0.0f);

	lights.push_back(dirLight);
	lights.push_back(pointLight);
	lights.push_back(pointLight2);
	lights.push_back(spotLight);
	lights.push_back(spotLight2);
	lights.push_back(spotLight3);

	if (lights.size() != LIGHT_COUNT)
		std::cout << "Change LIGHT_COUNT." << std::endl;

	// Setting lights
	numLightsLocs[0] = glGetUniformLocation(ground.GLSLProgramID, "numLights");
	setLightUniformLocs(lightLocGround, ground);
	
	for (int i = 0; i < OBJ_COUNT; ++i)
	{
		numLightsLocs[1+i] = glGetUniformLocation(objects[i].GLSLProgramID, "numLights");
		setLightUniformLocs(lightLocObjects[i], objects[i]);
	}

	do {
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		now = glfwGetTime();

		if (now - then > 0.02) {
			then = now;

			lightMove += 0.02f;
			// Animate lights
			lights[0].position.x = cos(lightMove/20) * 50;
			lights[0].position.y = sin(lightMove/20) * 50 + 0.9;

			lights[1].position.x = cos(lightMove) * 2;
			lights[1].position.y = abs(sin(lightMove/2)) * 4;
			lights[1].position.z = sin(lightMove*2) * 2;

			lights[2].position.y = abs(sin(lightMove)) * 5 + 1.0f;
			lights[2].position.z = abs(sin(lightMove)) * 20 - 25.0f;
			lights[2].color.r = abs(sin(lightMove * 5));
			lights[2].color.g = abs(sin(lightMove * 2));
			lights[2].color.b = abs(sin(lightMove * 3));

			lights[3].position.x = cos(lightMove*1.5) * 6;
			lights[3].coneDirection.x = cos(lightMove*2) * 3;

			lights[4].coneDirection.x = sin(lightMove) * 2;
			lights[4].coneDirection.y = cos(lightMove) / 2 + 0.5f;

			lights[5].coneDirection.y = abs(sin(lightMove / 2)) + 2.0f;
			lights[5].coneAngle = abs(sin(lightMove/6)) * 30;

			// Animate objects
			objectRBTs[0] = glm::translate(glm::vec3(0.0f, sin(lightMove*2)/10, sin(lightMove*2)/3)) * glm::rotate(glm::mat4(1.0f), sin(lightMove*40)*2, glm::vec3(0.0f, 0.0f, 1.0f)) * objectRBTs[0];

			objectRBTs[1] *= glm::rotate(1.0f, glm::vec3(0.0f, 1.0f, 0.0f));
		}

		eyeRBT = (view_index == 0) ? skyRBT : objectRBTs[0];

		// Pass light values to the shader
		glUseProgram(ground.GLSLProgramID);
		glUniform1i(numLightsLocs[0], (GLint)lights.size());
		setLightUniforms(lightLocGround);

		for (int i = 0; i < OBJ_COUNT; ++i)
		{
			glUseProgram(objects[i].GLSLProgramID);
			glUniform1i(numLightsLocs[1+i], (GLint)lights.size());
			setLightUniforms(lightLocObjects[i]);

			// Draw objects
			objects[i].draw();
		}
		

		// Draw wireframe of arcBall with dynamic radius
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		switch (object_index)
		{
		case 0:
			arcballRBT = (sky_type == 0) ? worldRBT : skyRBT;
			break;
		case 1:
			arcballRBT = objectRBTs[0];
			break;
		default:
			break;
		}

		ScreenToEyeScale = compute_screen_eye_scale(
			(glm::inverse(eyeRBT) * arcballRBT * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)).z,
			fovy,
			frameBufferHeight
			);
		arcBallScale = ScreenToEyeScale * arcBallScreenRadius;
		arcballRBT = arcballRBT * glm::scale(worldRBT, glm::vec3(arcBallScale, arcBallScale, arcBallScale));
		//arcBall.draw();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		ground.draw();
		// Swap buffers (Double buffering)
		glfwSwapBuffers(window);
		glfwPollEvents();
	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Clean up data structures and glsl objects
	ground.cleanup();
	for (int i = 0; i < 3; ++i)
	{
		objects[i].cleanup();
	}

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}
