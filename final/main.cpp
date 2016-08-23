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


#include <common/shader.hpp>
#include <common/affine.hpp>
#include <common/geometry.hpp>
#include <common/arcball.hpp>
#include <common/texture.hpp>

using namespace glm;

int const LIGHT_COUNT = 4;

float g_groundSize = 100.0f;
float g_groundY = -2.5f;

GLuint lightLocsCube[4][LIGHT_COUNT * 6], numLightsLocs[4];
GLuint isSky, isEye;
GLuint opacityLoc[4];

GLuint addPrograms[4];
GLuint texture[9];
GLuint textureID[4][9];
GLuint bumps[3];
GLuint bumpTex;
GLuint bumpTexID;
GLuint displacementTexID;
GLuint cubeTex;
GLuint cubeTexID;
// Texture rendering
GLuint FramebufferName;
GLuint renderedTexture;
GLuint depthrenderbuffer;

bool isChromaKey = true;
bool isPixelated = true;
float pixels = 1000;

int curBump = 0;

// View properties
glm::mat4 Projection;
float windowWidth = 1280.0f;
float windowHeight = 720.0f;
int frameBufferWidth = 0;
int frameBufferHeight = 0;
float frameRatio;
float fov = 45.0f;
float fovy = fov;
bool animate = true;
bool motionBlurOn = true;
int blurAmount = 3;

// Model properties
glm::mat4 skyRBT;
glm::mat4 eyeRBT;
const glm::mat4 worldRBT = glm::mat4(1.0f);
glm::mat4 arcballRBT = glm::mat4(1.0f);
glm::mat4 aFrame;
//cubes
glm::mat4 objectRBT[9], mbObjectRBT[9];
Model cubes[9], mbCubes[9];
mat4 curRBT[9];
int program_cnt = 1;
//cube animation
int ani_counts[6] = {};
float ani_angles[6] = {};
bool rot_first_col = false, rot_second_col = false, rot_third_col = false,
	rot_first_row = false, rot_second_row = false, rot_third_row = false,
	rot_col = false, rot_row = false;
int col_rot_cnt = 0, row_rot_cnt = 0;

//Sky box
Model skybox;
glm::mat4 skyboxRBT = glm::translate(0.0f, 0.0f, 0.0f);//Will be fixed(cause it is the sky)

vec3 eyePosition = vec3(0.0, 0.25, 6.0);
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

// Deer model
Model deer;
mat4 deerRBT = glm::scale(0.1f, 0.1f, 0.1f)*glm::translate(-25.0f, 2.0f, -10.0f);

GLenum  cube[6] = { GL_TEXTURE_CUBE_MAP_POSITIVE_X,
GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

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
std::vector<std::string> lightProperties = { "position","color","falloff","ambientCoefficient","coneAngle","coneDirection" };

void init_cubeRBT(){
	objectRBT[0] = glm::scale(0.7f, 0.7f, 0.7f)*glm::translate(-1.1f, 1.1f,.0f);
	objectRBT[1] = glm::scale(0.7f, 0.7f, 0.7f)*glm::translate(0.0f,1.1f,.0f);
	objectRBT[2] = glm::scale(0.7f, 0.7f, 0.7f)*glm::translate(1.1f, 1.1f,.0f);
	objectRBT[3] = glm::scale(0.7f, 0.7f, 0.7f)*glm::translate(-1.1f, 0.0f,.0f);
	objectRBT[4] = glm::scale(0.7f, 0.7f, 0.7f)*glm::translate(0.0f, 0.0f, .0f);//Center
	objectRBT[5] = glm::scale(0.7f, 0.7f, 0.7f)*glm::translate(1.1f, 0.0f,.0f);
	objectRBT[6] = glm::scale(0.7f, 0.7f, 0.7f)*glm::translate(-1.1f,-1.1f,.0f);
	objectRBT[7] = glm::scale(0.7f, 0.7f, 0.7f)*glm::translate(0.0f, -1.1f,.0f);
	objectRBT[8] = glm::scale(0.7f, 0.7f, 0.7f)*glm::translate(1.1f, -1.1f,.0f);	
}
void set_program(int p){	
	for (int i = 0; i < 9; i++){
		cubes[i].GLSLProgramID = addPrograms[p];		
	}
	deer.GLSLProgramID = addPrograms[p];
}
void init_shader(int idx, const char * vertexShader_path, const char * fragmentShader_path){
	addPrograms[idx] = LoadShaders(vertexShader_path, fragmentShader_path);
	glUseProgram(addPrograms[idx]);
}
void init_cubemap(const char * baseFileName, int size) {
	glActiveTexture(GL_TEXTURE0 + 3);
	glGenTextures(1, &cubeTexID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexID);
	const char * suffixes[] = { "ft", "bk", "dn", "up", "rt", "lf" };
	GLuint targets[] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z, GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
	};
	GLint w, h;
	glTexStorage2D(GL_TEXTURE_CUBE_MAP, 1, GL_RGBA8, size, size);
	for (int i = 0; i < 6; i++) {
		std::string texName = std::string(baseFileName) + "_" + suffixes[i] + ".bmp";
		unsigned char* data = loadBMP_cube(texName.c_str(), &w, &h);
		glTexSubImage2D(targets[i], 0, 0, 0, w, h,
			GL_BGR, GL_UNSIGNED_BYTE, data); // GL_BGR for correct colours
		delete[] data;
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 3);
}
void init_texture(void){	
	// Initialize textures
	texture[0] = loadBMP_custom("cubemap.bmp");
	for (int i = 0; i < 4; i++) textureID[i][0] = glGetUniformLocation(addPrograms[i], "myTextureSampler");

	texture[1] = loadBMP_custom("deer.bmp");
	for (int i = 0; i < 4; i++) textureID[i][1] = glGetUniformLocation(addPrograms[i], "myTextureSampler");

	//TODO: Initialize bump texture
	bumps[0] = loadBMP_custom("paper.bmp");
	bumps[1] = loadBMP_custom("flower.bmp");
	bumps[2] = loadBMP_custom("bump.bmp");
	bumpTex = bumps[0];
	bumpTexID = glGetUniformLocation(addPrograms[1], "myBumpSampler");
	displacementTexID = glGetUniformLocation(addPrograms[2], "displacementSampler");

	//TODO: Initialize Cubemap texture
	init_cubemap("miramar/miramar", 2048);
}
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

	frameRatio = frameBufferWidth / frameBufferHeight;

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

	// Update texture framebuffer
	glBindTexture(GL_TEXTURE_2D, renderedTexture);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frameBufferWidth, frameBufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, frameBufferWidth, frameBufferHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);
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
		aFrame = transFact(objectRBT[0]) * linearFact(eyeRBT);
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
		else { objectRBT[0] = aFrame * m * glm::inverse(aFrame) * objectRBT[0]; }

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
			std::cout << "Help:\n\tO: Toggle animation.\n\tP: Change programs.\n\tQ, W, E: Rotate columns." <<
				"\n\tA, S, D: Rotate rows.\n\tM: Toggle motion blur.\n\tK, L: Decrease/Increase motion blur." <<
				"\n\t1, 2, 3: Change bump/normal map (can only be seen in program 1 and 2)." <<
				"\n\t-: Toggle directional light.\n\tTab: Toggle pixelation." <<
				"\n\t<- & ->: Decrease or Increase pixelation.\n\tC: Toggle chroma keying." << std::endl;
			break;

		case GLFW_KEY_O:
			animate = !animate;
			break;

		case GLFW_KEY_P:// Change Programs
			program_cnt = ++program_cnt % 4;
			std::cout << "Program " << program_cnt << " active!" << std::endl;
			set_program(program_cnt);
			break;

		case GLFW_KEY_Q: // Rotate first column
			if (!rot_first_col && !rot_row){
				rot_first_col = true;
				col_rot_cnt += 1;
				rot_col = true;
				mat4 temp = objectRBT[0];
				objectRBT[0] = curRBT[0] = objectRBT[6];
				objectRBT[6] = curRBT[6] = temp;
			}
			break;
		case GLFW_KEY_W:// Rotate second column
			if (!rot_second_col && !rot_row) {
				rot_second_col = true;
				col_rot_cnt += 1;
				rot_col = true;
				mat4 temp = objectRBT[1];
				objectRBT[1] = curRBT[1] = objectRBT[7];
				objectRBT[7] = curRBT[7] = temp;
			}
			break;
		case GLFW_KEY_E:// Rotate third column
			if (!rot_third_col && !rot_row) {
				rot_third_col = true;
				col_rot_cnt += 1;
				rot_col = true;
				mat4 temp = objectRBT[2];
				objectRBT[2] = curRBT[2] = objectRBT[8];
				objectRBT[8] = curRBT[8] = temp;
			}
			break;
		case GLFW_KEY_A:// Rotate first row
			if (!rot_first_row && !rot_col) {
				rot_first_row = true;
				row_rot_cnt += 1;
				rot_row = true;
				mat4 temp = objectRBT[0];
				objectRBT[0] = curRBT[0] = objectRBT[2];
				objectRBT[2] = curRBT[2] = temp;
			}
			break;

		case GLFW_KEY_S:// Rotate second row
			if (!rot_second_row && !rot_col) {
				rot_second_row = true;
				row_rot_cnt += 1;
				rot_row = true;
				mat4 temp = objectRBT[3];
				objectRBT[3] = curRBT[3] = objectRBT[5];
				objectRBT[5] = curRBT[5] = temp;
			}
			break;
		case GLFW_KEY_D:// Rotate third row
			if (!rot_third_row && !rot_col) {
				rot_third_row = true;
				row_rot_cnt += 1;
				rot_row = true;
				mat4 temp = objectRBT[6];
				objectRBT[6] = curRBT[6] = objectRBT[8];
				objectRBT[8] = curRBT[8] = temp;
			}
			break;

		case GLFW_KEY_M: // Toggle motion blur
			motionBlurOn = !motionBlurOn;
			break;
		case GLFW_KEY_K: // Decrease blur amount
			blurAmount = max(blurAmount - 1, 1);
			break;
		case GLFW_KEY_L: // Increase blur amount
			blurAmount = min(blurAmount + 1, 3);
			break;

		case GLFW_KEY_1: // Change bump/normal map
			bumpTex = bumps[0];
			break;
		case GLFW_KEY_2: // Change bump/normal map
			bumpTex = bumps[1];
			break;
		case GLFW_KEY_3: // Change bump/normal map
			bumpTex = bumps[2];
			break;

		case GLFW_KEY_MINUS: // Toggle directional light
			if (lights[0].color.x == 0.5) {
				lights[0].color = glm::vec3(0.0f, 0.0f, 0.0f);
				lights[0].ambientCoefficient = 0.0f;
			}
			else {
				lights[0].color = glm::vec3(0.5f, 0.5f, 0.5f);
				lights[0].ambientCoefficient = 0.15f;
			}
			break;

		case GLFW_KEY_TAB: // Toggle pixelation
			isPixelated = !isPixelated;
			break;
		case GLFW_KEY_RIGHT: // Increase pixel count
			pixels = min(pixels + 50, 2000.0f);
			break;
		case GLFW_KEY_LEFT: // Decrease pixel count
			pixels = max(pixels - 50, 50.0f);
			break;

		case GLFW_KEY_C: // Toggle chroma keying
			isChromaKey = !isChromaKey;
			if(isChromaKey)
				glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
			else
				glClearColor(0.3f, 0.6f, 0.8f, 1.0f); // Change to nicer colour when not chroma keying
			break;
		default:
			break;
		}
	}
}

void rotate_cubes(int c1, int c2, int c3, float& ani_angle, int& ani_count, bool& rot, bool is_col)
{
	glm::vec3 axis;
	if (is_col)
		axis = glm::vec3(1.0f, 0.0f, 0.0f);
	else
		axis = glm::vec3(0.0f, 1.0f, 0.0f);

	ani_angle += 0.6f;
	objectRBT[c1] = glm::rotate(glm::mat4(1.0f), ani_angle * 10, axis) * curRBT[c1];
	objectRBT[c2] = glm::rotate(glm::mat4(1.0f), ani_angle * 10, axis) * curRBT[c2];
	objectRBT[c3] = glm::rotate(glm::mat4(1.0f), ani_angle * 10, axis) * curRBT[c3];

	ani_count++;
	if (ani_count > 29) {
		curRBT[c1] = objectRBT[c1];
		curRBT[c2] = objectRBT[c2];
		curRBT[c3] = objectRBT[c3];
		rot = false;
		if (is_col)
		{
			col_rot_cnt -= 1;
			if (col_rot_cnt == 0)
				rot_col = false;
		}
		else
		{
			row_rot_cnt -= 1;
			if (row_rot_cnt == 0)
				rot_row = false;
		}
		ani_angle = 0.0f;
		ani_count = 0;
	}

	mbObjectRBT[c1] = glm::rotate(glm::mat4(1.0f), ani_angle * (-blurAmount), axis) * objectRBT[c1];
	mbObjectRBT[c2] = glm::rotate(glm::mat4(1.0f), ani_angle * (-blurAmount), axis) * objectRBT[c2];
	mbObjectRBT[c3] = glm::rotate(glm::mat4(1.0f), ani_angle * (-blurAmount), axis) * objectRBT[c3];
}

void cube_rotation() {
	if (rot_first_col) {
		rotate_cubes(0, 3, 6, ani_angles[0], ani_counts[0], rot_first_col, true);
	}
	if (rot_second_col) {
		rotate_cubes(1, 4, 7, ani_angles[1], ani_counts[1], rot_second_col, true);
	}
	if (rot_third_col) {
		rotate_cubes(2, 5, 8, ani_angles[2], ani_counts[2], rot_third_col, true);
	}
	if (rot_first_row) {
		rotate_cubes(0, 1, 2, ani_angles[3], ani_counts[3], rot_first_row, false);
	}
	if (rot_second_row) {
		rotate_cubes(3, 4, 5, ani_angles[4], ani_counts[4], rot_second_row, false);
	}
	if (rot_third_row) {
		rotate_cubes(6, 7, 8, ani_angles[5], ani_counts[5], rot_third_row, false);
	}
}

std::string lightUniformString(int index, std::string property)
{
	std::ostringstream oss;
	oss << "lights[" << index << "]." << property;

	return oss.str();
}

void setLightUniformLocs(GLuint* uniformArray, GLuint progID)
{
	for (int i = 0; i < lights.size(); ++i)
	{
		uniformArray[i * 6] = glGetUniformLocation(progID, lightUniformString(i, "position").c_str());
		uniformArray[i * 6 + 1] = glGetUniformLocation(progID, lightUniformString(i, "color").c_str());
		uniformArray[i * 6 + 2] = glGetUniformLocation(progID, lightUniformString(i, "falloff").c_str());
		uniformArray[i * 6 + 3] = glGetUniformLocation(progID, lightUniformString(i, "ambientCoefficient").c_str());
		uniformArray[i * 6 + 4] = glGetUniformLocation(progID, lightUniformString(i, "coneAngle").c_str());
		uniformArray[i * 6 + 5] = glGetUniformLocation(progID, lightUniformString(i, "coneDirection").c_str());
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
	window = glfwCreateWindow((int)windowWidth, (int)windowHeight, "Homework 5 | Fredrik Berglund 20166029", NULL, NULL);
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

	frameRatio = frameBufferWidth / frameBufferHeight;

	// Clear with pure green, for chroma keying
	glClearColor(0.0f, 1.0f, 0.0f, 1.0f);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);	


	Projection = glm::perspective(fov, windowWidth / windowHeight, 0.1f, 100.0f);
	skyRBT = glm::translate(glm::mat4(1.0f), eyePosition);

	aFrame = linearFact(skyRBT);
	// initial eye frame = sky frame;
	eyeRBT = skyRBT;
	
	//init shader
	init_shader(0, "VertexShader.glsl", "FragmentShader.glsl");
	init_shader(1, "BumpVertexShader.glsl", "BumpFragmentShader.glsl");
	init_shader(2, "DisplacementVertexShader.glsl", "DisplacementFragmentShader.glsl");
	init_shader(3, "RefractionVertexShader.glsl", "RefractionFragmentShader.glsl");

	// Initialize model
	deer = Model();
	init_obj2(deer, "deer.obj");
	deer.initialize(DRAW_TYPE::ARRAY, addPrograms[0]);
	deer.set_projection(&Projection);
	deer.set_eye(&eyeRBT);
	deer.set_model(&deerRBT);

	//TODO: Initialize cube model by calling textured cube model
	init_cubeRBT();	
	cubes[0] = Model();
	init_texture_cube(cubes[0]);
	cubes[0].initialize(DRAW_TYPE::ARRAY, addPrograms[0]);

	cubes[0].set_projection(&Projection);
	cubes[0].set_eye(&eyeRBT);
	cubes[0].set_model(&objectRBT[0]);

	for (int i = 1; i < 9; i++) {
		cubes[i] = Model();
		cubes[i].initialize(DRAW_TYPE::ARRAY, cubes[0]);

		cubes[i].set_projection(&Projection);
		cubes[i].set_eye(&eyeRBT);
		cubes[i].set_model(&objectRBT[i]);
	}

	// Motion blur cubes
	for (int i = 0; i < 9; i++) {
		mbCubes[i] = Model();
		mbCubes[i].initialize(DRAW_TYPE::ARRAY, cubes[0]);

		mbCubes[i].set_projection(&Projection);
		mbCubes[i].set_eye(&eyeRBT);
		mbCubes[i].set_model(&mbObjectRBT[i]);
	}
	
	skybox = Model();
	init_skybox(skybox);
	skybox.initialize(DRAW_TYPE::ARRAY, addPrograms[2]);
	skybox.set_projection(&Projection);
	skybox.set_eye(&eyeRBT);
	skybox.set_model(&skyboxRBT);

	arcBall = Model();
	init_sphere(arcBall);
	arcBall.initialize(DRAW_TYPE::INDEX, cubes[0].GLSLProgramID);

	arcBall.set_projection(&Projection);
	arcBall.set_eye(&eyeRBT);
	arcBall.set_model(&arcballRBT);

	// init textures
	init_texture();
	
	mat4 oO[9];
	for(int i=0;i<9;i++) oO[i] = curRBT[i] = mbObjectRBT[i] = objectRBT[i];
	float angle = 0.0f;
	double pre_time = glfwGetTime();
	double pre_time2 = pre_time;
	program_cnt = 0;
	set_program(0);

	// Setup of lights
	Light dirLight;
	dirLight.position = glm::vec4(15.0f, 20.0f, 5.0f, 0.0f);
	dirLight.color = glm::vec3(0.5f, 0.5f, 0.5f);
	dirLight.ambientCoefficient = 0.15f;

	Light pointLight;
	pointLight.position = glm::vec4(0.0f, 0.1f, -0.05f, 1.0f);
	pointLight.color = glm::vec3(0.0f, 1.0f, 1.0f);
	pointLight.falloff = 0.1f;
	pointLight.ambientCoefficient = 0.0f;
	pointLight.coneAngle = 180.0f;

	Light pointLight2;
	pointLight2.position = glm::vec4(0.0f, 0.1f, 0.005f, 1.0f);
	pointLight2.color = glm::vec3(0.7f, 0.1f, 0.6f);
	pointLight2.falloff = 0.4f;
	pointLight2.ambientCoefficient = 0.0f;
	pointLight2.coneAngle = 180.0f;

	Light spotLight;
	spotLight.position = glm::vec4(0.0f, 0.5f, 2.0f, 1.0f);
	spotLight.color = glm::vec3(0.9f, 0.1f, 0.0f);
	spotLight.falloff = 0.01f;
	spotLight.ambientCoefficient = 0.0f;
	spotLight.coneAngle = 35.0f;
	spotLight.coneDirection = glm::vec3(0.0f, 0.0f, 0.0f);

	lights.push_back(dirLight);
	lights.push_back(pointLight);
	lights.push_back(pointLight2);
	lights.push_back(spotLight);

	if (lights.size() != LIGHT_COUNT)
		std::cout << "Change LIGHT_COUNT." << std::endl;

	// Setting lights & getting opacity uniform location
	for (int i = 0; i < 4; ++i) {
		numLightsLocs[i] = glGetUniformLocation(addPrograms[i], "numLights");
		setLightUniformLocs(lightLocsCube[i], addPrograms[i]);

		opacityLoc[i] = glGetUniformLocation(addPrograms[i], "opacity");
	}

	//http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-14-render-to-texture/
	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	glGenFramebuffers(1, &FramebufferName);
	glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);

	// The texture we're going to render to
	glGenTextures(1, &renderedTexture);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, renderedTexture);

	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, frameBufferWidth, frameBufferHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);

	// Poor filtering.
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	// The depth buffer
	glGenRenderbuffers(1, &depthrenderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, depthrenderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, frameBufferWidth, frameBufferHeight);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depthrenderbuffer);

	// Set "renderedTexture" as our colour attachement #0
	glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, renderedTexture, 0);

	// Set the list of draw buffers.
	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers); // "1" is the size of DrawBuffers

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		return false;

	// The fullscreen quad's FBO
	GLuint quad_VertexArrayID;
	glGenVertexArrays(1, &quad_VertexArrayID);
	glBindVertexArray(quad_VertexArrayID);

	static const GLfloat g_quad_vertex_buffer_data[] = {
		-1.0f, -1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f, 0.0f,
		1.0f, -1.0f, 0.0f,
		1.0f,  1.0f, 0.0f,
	};

	GLuint quad_vertexbuffer;
	glGenBuffers(1, &quad_vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_quad_vertex_buffer_data), g_quad_vertex_buffer_data, GL_STATIC_DRAW);

	// Create and compile our GLSL program from the shaders
	GLuint quad_programID = LoadShaders("passthroughVertexShader.glsl", "textureFragmentShader.glsl");

	GLuint texID = glGetUniformLocation(quad_programID, "renderedTexture");
	GLuint timeID = glGetUniformLocation(quad_programID, "time");
	GLuint pixelsID = glGetUniformLocation(quad_programID, "pixels");
	GLuint isPixelatedID = glGetUniformLocation(quad_programID, "isPixelated");
	GLuint heightID = glGetUniformLocation(quad_programID, "frameHeight");
	GLuint widthID = glGetUniformLocation(quad_programID, "frameWidth");
	GLuint frameRatioID = glGetUniformLocation(quad_programID, "frameRatio");
	texture[2] = loadBMP_custom("spaaace.bmp");
	GLuint replaceTexID = glGetUniformLocation(quad_programID, "replaceTexture");
	GLuint isChromaKeyID = glGetUniformLocation(quad_programID, "isChromaKey");

	// Enable blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	do {
		double cur_time = glfwGetTime();
		if (cur_time - pre_time > 0.008) {
			// Render to our framebuffer
			glBindFramebuffer(GL_FRAMEBUFFER, FramebufferName);
			glViewport(0, 0, frameBufferWidth, frameBufferHeight); // Render on the whole framebuffer

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear the screen
			eyeRBT = (view_index == 0) ? skyRBT : objectRBT[0];

			//cube rotation
			if (animate) {
				cube_rotation();
				deerRBT *= glm::rotate(glm::mat4(1.0f), 0.3f, vec3(0.0f, 1.0f, 0.0f));
			}

			// Animate lights
			lights[1].position.x = cos(angle / 2) * 2;
			lights[1].position.y = cos(angle / 4) * 4 + 1.5f;
			lights[1].position.z = sin(angle / 4) * 4 + 1.0f;

			lights[2].position.x = cos(angle / 6) / 2;
			lights[2].position.y = cos(angle / 7) / 4 - 1.5f;

			lights[3].position.x = sin(angle / 6) * 1.5;
			lights[3].coneDirection.x = sin(angle / 5) * 4;
			lights[3].coneDirection.y = cos(angle / 3) / 2 + 0.5f;

			if (animate)
				angle += 0.02f;
			if (angle == 360.0f) angle = 0.0f;

			glUseProgram(addPrograms[program_cnt]);

			if (program_cnt == 3) {
				isSky = glGetUniformLocation(addPrograms[3], "DrawSkyBox");
				glUniform1i(isSky, 0);
				// Pass the cubemap texture to shader
				glActiveTexture(GL_TEXTURE0 + 3);
				glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexID);
				glUniform1i(cubeTex, 3);
			}

			// Pass the first texture value to shader
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, texture[0]);
			glUniform1i(textureID[program_cnt][0], 0);

			glUniform1i(numLightsLocs[program_cnt], (GLint)lights.size());
			setLightUniforms(lightLocsCube[program_cnt]);

			// Draw regular cubes
			glUniform1f(opacityLoc[program_cnt], 1.0);
			cubes[0].draw();

			for (int i = 1; i < 9; i++) {
				cubes[i].draw2(cubes[0]);
			}

			// Draw motion blur cubes
			if (motionBlurOn && !isChromaKey && program_cnt != 1 && program_cnt != 3) {
				glUniform1f(opacityLoc[program_cnt], 0.5);
				for (int i = 0; i < 9; ++i)
					mbCubes[i].draw2(cubes[0]);
				glUniform1f(opacityLoc[program_cnt], 1.0);
			}

			// Pass bump(normalmap) texture value to shader
			if (program_cnt == 1) {
				glActiveTexture(GL_TEXTURE0 + 2);
				glBindTexture(GL_TEXTURE_2D, bumpTex);
				glUniform1i(bumpTexID, 2);
			}
			else if (program_cnt == 2)
			{
				if (animate && cur_time - pre_time2 > 0.1) {
					curBump = (curBump + 1) % 3;
					bumpTex = bumps[curBump];
					pre_time2 = cur_time;
				}
				glActiveTexture(GL_TEXTURE0 + 2);
				glBindTexture(GL_TEXTURE_2D, bumpTex);
				glUniform1i(displacementTexID, 2);
			}

			// Pass the second texture value to shader
			glActiveTexture(GL_TEXTURE0 + 1);
			glBindTexture(GL_TEXTURE_2D, texture[1]);
			glUniform1i(textureID[program_cnt][1], 1);

			deer.draw();

			if (program_cnt == 3) {
				isSky = glGetUniformLocation(addPrograms[3], "DrawSkyBox");
				glUniform1i(isSky, 1);

				glUseProgram(addPrograms[3]);
				isEye = glGetUniformLocation(addPrograms[3], "WorldCameraPosition");
				glUniform3f(isEye, eyePosition.x, eyePosition.y, eyePosition.z);

				cubeTex = glGetUniformLocation(addPrograms[3], "cubemap");
				glActiveTexture(GL_TEXTURE0 + 3);
				glBindTexture(GL_TEXTURE_CUBE_MAP, cubeTexID);
				glUniform1i(cubeTex, 3);

				glDepthMask(GL_FALSE);
				skybox.draw();
				glDepthMask(GL_TRUE);

				glUniform1i(isSky, 0);
			}

			// Arcball
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			switch (object_index)
			{
			case 0:
				arcballRBT = (sky_type == 0) ? worldRBT : skyRBT;
				break;
			case 1:
				arcballRBT = objectRBT[0];
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
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

			// Render to the screen
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glViewport(0, 0, frameBufferWidth, frameBufferHeight); // Render on the whole framebuffer

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			// Use simple vertex and texture shader
			glUseProgram(quad_programID);

			// Bind our texture in Texture Unit 4
			glActiveTexture(GL_TEXTURE0 + 4);
			glBindTexture(GL_TEXTURE_2D, renderedTexture);
			// Set our "renderedTexture" sampler to user Texture Unit 4
			glUniform1i(texID, 4);

			// Set other uniforms for shader
			if(isPixelated)
				glUniform1i(isPixelatedID, 1);
			else
				glUniform1i(isPixelatedID, 0);
			glUniform1f(pixelsID, pixels);
			glUniform1f(heightID, (float)frameBufferHeight);
			glUniform1f(widthID, (float)frameBufferWidth);
			glUniform1f(frameRatioID, frameRatio);

			glUniform1i(isChromaKeyID, isChromaKey);
			glActiveTexture(GL_TEXTURE0 + 5);
			glBindTexture(GL_TEXTURE_2D, texture[2]);
			glUniform1i(replaceTexID, 5);

			// 1st attribute buffer : vertices
			glEnableVertexAttribArray(0);
			glBindBuffer(GL_ARRAY_BUFFER, quad_vertexbuffer);
			glVertexAttribPointer(
				0,                  // attribute 0. No particular reason for 0, but must match the layout in the shader.
				3,                  // size
				GL_FLOAT,           // type
				GL_FALSE,           // normalized?
				0,                  // stride
				(void*)0            // array buffer offset
			);

			// Draw the triangles!
			glDrawArrays(GL_TRIANGLES, 0, 6); // 2*3 indices starting at 0 -> 2 triangles

			glDisableVertexAttribArray(0);

			glfwSwapBuffers(window);
			glfwPollEvents();
			pre_time = cur_time;
		}
	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Clean up data structures and glsl objects	
	for (int i = 0; i < 9; i++) {
		cubes[i].cleanup();
		mbCubes[i].cleanup();
		deer.cleanup();
	}

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}