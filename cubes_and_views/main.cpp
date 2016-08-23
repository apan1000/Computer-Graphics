/*
	Fredrik Berglund 2016
*/

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <iostream>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/model.hpp>

float g_groundSize = 100.0f;
float g_groundY = -2.5f;

GLuint lightLocGround, lightLocRed, lightLocGreen;

// View properties
glm::mat4 Projection;
float windowWidth = 1024.0f;
float windowHeight = 768.0f;
float fov = 45.0f;

bool leftClick = false, rightClick = false;

// Model properties
Model ground, redCube, greenCube;
glm::mat4 skyRBT;
glm::mat4 redCubeRBT;
glm::mat4 greenCubeRBT;
glm::mat4 eyeRBT;

int select_frame = 0;
int number_of_frames = 3;

glm::vec3 vertices[8] = {
	glm::vec3(-0.5, -0.5, 0.5),
	glm::vec3(-0.5, 0.5, 0.5),
	glm::vec3(0.5, 0.5, 0.5),
	glm::vec3(0.5, -0.5, 0.5),
	glm::vec3(-0.5, -0.5, -0.5),
	glm::vec3(-0.5, 0.5, -0.5),
	glm::vec3(0.5, 0.5, -0.5),
	glm::vec3(0.5, -0.5, -0.5)
};

void compute_normal(Model &model, glm::vec3 a, glm::vec3 b, glm::vec3 c)
{
	model.add_normal(glm::normalize(glm::cross(b - a, c - a)));
	model.add_normal(glm::normalize(glm::cross(b - a, c - a)));
	model.add_normal(glm::normalize(glm::cross(b - a, c - a)));
}

void quad(Model &model, int a, int b, int c, int d, glm::vec3 color)
{
	// TODO: quad() function
	model.add_vertex(vertices[a]);
	model.add_vertex(vertices[b]);
	model.add_vertex(vertices[c]);
	model.add_vertex(vertices[a]);
	model.add_vertex(vertices[c]);
	model.add_vertex(vertices[d]);

	compute_normal(model, vertices[a], vertices[b], vertices[c]);
	compute_normal(model, vertices[a], vertices[c], vertices[d]);
	
	model.add_color(color);
	model.add_color(color);
	model.add_color(color);
	model.add_color(color);
	model.add_color(color);
	model.add_color(color);
}

void init_cube(Model &model, glm::vec3 color)
{
	quad(model, 1, 0, 3, 2, color);
	quad(model, 2, 3, 7, 6, color);
	quad(model, 3, 0, 4, 7, color);
	quad(model, 6, 5, 1, 2, color);
	quad(model, 4, 5, 6, 7, color);
	quad(model, 5, 4, 0, 1, color);
}

void init_ground(Model &model)
{
	glm::vec3 a = glm::vec3(-0.5f, 0.0f, -0.5f);
	glm::vec3 b = glm::vec3(0.5f, 0.0f, -0.5f);
	glm::vec3 c = glm::vec3(-0.5f, 0.0f, 0.5f);
	glm::vec3 d = glm::vec3(0.5f, 0.0f, 0.5f);
	model.add_vertex(a);
	model.add_vertex(c);
	model.add_vertex(b);
	model.add_vertex(b);
	model.add_vertex(c);
	model.add_vertex(d);

	compute_normal(model, a, c, b);
	compute_normal(model, b, c, d);

	glm::vec3 color = glm::vec3(0.1, 0.95, 0.1);
	model.add_color(color);
	model.add_color(color);
	model.add_color(color);
	model.add_color(color);
	model.add_color(color);
	model.add_color(color);
}

static void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	bool otherFrame = false;

	if (action == GLFW_PRESS)
	{
		// Change select_frame by Keyboard Input
		switch (key)
		{
		case GLFW_KEY_V:
			select_frame = (select_frame + 1) % number_of_frames;
			break;
		default:
			break;
		}
	}
	else {
		// Compute Transformation with Keyboard Input
		glm::mat4 m = glm::mat4(1.0f);
		switch (key)
		{
			// Rotate around y-axis
			case GLFW_KEY_Q:
				m = glm::rotate(glm::mat4(1.0f), 1.5f, glm::vec3(0.0f, 1.0f, 0.0f));
				break;
			case GLFW_KEY_E:
				m = glm::rotate(glm::mat4(1.0f), -1.5f, glm::vec3(0.0f, 1.0f, 0.0f));
				break;

			 // Rotate around x-axis 
			case GLFW_KEY_R:
				m = glm::rotate(glm::mat4(1.0f), -1.5f, glm::vec3(1.0f, 0.0f, 0.0f));
				break;
			case GLFW_KEY_F:
				m = glm::rotate(glm::mat4(1.0f), 1.5f, glm::vec3(1.0f, 0.0f, 0.0f));
				break;

			// Move in x,y,z
			case GLFW_KEY_W:
				// Forward
				m = glm::translate(glm::mat4(1.0f),  glm::vec3(0.0f, 0.0f, -0.1f));
				break;
			case GLFW_KEY_A:
				m = glm::translate(glm::mat4(1.0f), glm::vec3(-0.1f, 0.0f, 0.0f));
				break;
			case GLFW_KEY_S:
				// Back
				m = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.1f));
				break;
			case GLFW_KEY_D:
				m = glm::translate(glm::mat4(1.0f), glm::vec3(0.1f, 0.0f, 0.0f));
				break;
			case GLFW_KEY_SPACE:
				m = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.1f, 0.0f));
				break;
			case GLFW_KEY_C:
				m = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.1f, 0.0f));
				break;

				// Rotate other object around y-axis
			case GLFW_KEY_LEFT:
				otherFrame = true;
				m = glm::rotate(glm::mat4(1.0f), -1.5f, glm::vec3(0.0f, 1.0f, 0.0f));
				break;
			case GLFW_KEY_RIGHT:
				otherFrame = true;
				m = glm::rotate(glm::mat4(1.0f), 1.5f, glm::vec3(0.0f, 1.0f, 0.0f));
				break;

				// Rotate other object around x-axis 
			case GLFW_KEY_UP:
				otherFrame = true;
				m = glm::rotate(glm::mat4(1.0f), -1.5f, glm::vec3(1.0f, 0.0f, 0.0f));
				break;
			case GLFW_KEY_DOWN:
				otherFrame = true;
				m = glm::rotate(glm::mat4(1.0f), 1.5f, glm::vec3(1.0f, 0.0f, 0.0f));
				break;
			default:
				break;
		}

		// Apply Transformation To Frame
		if (otherFrame)
		{
			bool debug = false;

			glm::mat4 frameToBeChanged = ((select_frame+1)%2 == 0) ? greenCubeRBT : redCubeRBT;

			glm::mat4 t = glm::mat4(
				glm::vec4(0, 0, 0, 0),
				glm::vec4(0, 0, 0, 0),
				glm::vec4(0, 0, 0, 0),
				glm::vec4(1, 1, 1, 0));
			glm::mat4 o_t = (glm::inverse(frameToBeChanged) * t) + glm::mat4(1.0);

			// Cheat option
			o_t = glm::mat4(
				glm::vec4(1, 0, 0, 0),
				glm::vec4(0, 1, 0, 0),
				glm::vec4(0, 0, 1, 0),
				glm::vec4(frameToBeChanged[3][0], frameToBeChanged[3][1], frameToBeChanged[3][2], 1));
			
			// Get rotation from eyeRBT matrix
			glm::mat4 r = glm::mat4(
				glm::vec4(1, 0, 0, 0),
				glm::vec4(0, 1, 0, 0),
				glm::vec4(0, 0, 1, 0),
				glm::vec4(0, 0, 0, 0));
			glm::mat4 e_r = (eyeRBT * r) + glm::mat4(glm::vec4(0, 0, 0, 0),
													glm::vec4(0, 0, 0, 0),
													glm::vec4(0, 0, 0, 0),
													glm::vec4(0, 0, 0, 1));
			/*e_r = glm::mat4(
				glm::vec4(1, 0, 0, 0),
				glm::vec4(0, 1, 0, 0),
				glm::vec4(0, 0, 1, 0),
				glm::vec4(0, 0, 0, 1));*/
			if (debug)
			{
				std::cout << "frameToBeChanged: " << glm::to_string(frameToBeChanged) << std::endl;
				std::cout << "o_t: " << glm::to_string(o_t) << "\n" << std::endl;
				std::cout << "eyeRBT: " << glm::to_string(eyeRBT) << std::endl;
				std::cout << "e_r: " << glm::to_string(e_r) << "\n" << std::endl;
				std::cout << "m: " << glm::to_string(m) << "\n" << std::endl;
			}
			
			// TODO: Move The Objects w.r.t The Current Frame
			glm::mat4 a = o_t * e_r;
			if ((select_frame + 1) % 2 == 0)
			{
				greenCubeRBT = a * m * glm::inverse(a) * frameToBeChanged;
			}
			else
			{
				redCubeRBT = a * m * glm::inverse(a) * frameToBeChanged;
			}
		}
		else {
			switch (select_frame)
			{
			case 0:
				skyRBT = skyRBT * m;
				break;
			case 1:
				redCubeRBT = redCubeRBT * m;
				break;
			case 2:
				greenCubeRBT = greenCubeRBT * m;
				break;
			default:
				break;
			}
		}
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
	window = glfwCreateWindow((int)windowWidth, (int)windowHeight, "Lab 2", NULL, NULL);
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
	glfwSetKeyCallback(window, keyboard_callback);

	// Clear with sky color
	glClearColor(128. / 255., 200. / 255., 255. / 255., 0.);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Enable culling
	glEnable(GL_CULL_FACE);
	// Backface culling
	glCullFace(GL_BACK);

	Projection = glm::perspective(fov, windowWidth / windowHeight, 0.1f, 100.0f);
	skyRBT = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.25, 4.0));

	// initial eye frame = sky frame;
	eyeRBT = skyRBT;

	// Initialize Ground Model
	ground = Model();
	init_ground(ground);
	ground.initialize("VertexShader.glsl", "FragmentShader.glsl");
	ground.set_projection(&Projection);
	ground.set_eye(&eyeRBT);
	glm::mat4 groundRBT = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, g_groundY, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(g_groundSize, 1.0f, g_groundSize));
	ground.set_model(&groundRBT);

	// Initialize Two Cube Models
	redCube = Model();
	init_cube(redCube, glm::vec3(1.0f, 0.0f, 0.0f));
	redCube.initialize("VertexShader.glsl", "FragmentShader.glsl");
	redCube.set_projection(&Projection);
	redCube.set_eye(&eyeRBT);
	redCubeRBT = glm::translate(glm::mat4(1.0f), glm::vec3(-1.5f, 0.5f, 0.0f))
		* glm::rotate(glm::mat4(1.0f), -90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	redCube.set_model(&redCubeRBT);

	greenCube = Model();
	init_cube(greenCube, glm::vec3(0.0f, 1.0f, 0.0f));
	greenCube.initialize("VertexShader.glsl", "FragmentShader.glsl");
	greenCube.set_projection(&Projection);
	greenCube.set_eye(&eyeRBT);
	greenCubeRBT = glm::translate(glm::mat4(1.0f), glm::vec3(1.5f, 0.5f,
		0.0f)) * glm::rotate(glm::mat4(1.0f), 90.0f, glm::vec3(0.0f, 1.0f, 0.0f));
	greenCube.set_model(&greenCubeRBT);

	// Setting Light Vectors
	glm::vec3 lightVec = glm::vec3(0.0f, 1.0f, 0.0f);
	lightLocGround = glGetUniformLocation(ground.GLSLProgramID, "uLight");
	glUniform3f(lightLocGround, lightVec.x, lightVec.y, lightVec.z);

	lightLocRed = glGetUniformLocation(redCube.GLSLProgramID, "uLight");
	glUniform3f(lightLocRed, lightVec.x, lightVec.y, lightVec.z);

	lightLocGreen = glGetUniformLocation(greenCube.GLSLProgramID, "uLight");
	glUniform3f(lightLocGreen, lightVec.x, lightVec.y, lightVec.z);

	float degree = 0.0f;
	float elapsedTime = 0.0f;
	float prevTime = 0.0;
	float currTime = 0.0;
	float distance = 3.0f;
	do {
		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		currTime = glfwGetTime();

		// Change Viewpoint by select_frame
		eyeRBT = (select_frame == 0) ? skyRBT : (select_frame ==
			1) ? redCubeRBT : greenCubeRBT;

		// Draw Two Cube Models
		redCube.draw();
		greenCube.draw();

		ground.draw();
		degree = degree + 6.0f * (currTime - prevTime);
		prevTime = currTime;
		// Swap buffers (Double buffering)
		glfwSwapBuffers(window);
		glfwPollEvents();
	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Clean up data structures and glsl objects
	ground.cleanup();
	redCube.cleanup();
	greenCube.cleanup();

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}
