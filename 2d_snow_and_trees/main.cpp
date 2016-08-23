/*
	Fredrik Berglund 2016
*/

// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <vector>
#include <random>
#include <algorithm>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>

// Include GLM
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

// Shader library
#include <common/shader.hpp>

#define BUFFER_OFFSET( offset ) ((GLvoid*) (offset))

GLFWwindow* window;
int windowWidth = 1024;
int windowHeight = 768;
double windowRatio = (double)windowWidth / windowHeight;;

GLuint programID;
GLuint program2ID;
GLuint sf_vertexArrayObject;
GLuint sf_vertexBufferObject;
GLuint tree_vertexArrayObject;
GLuint tree_vertexBufferObject;
GLuint snow_vertexArrayObject;
GLuint snow_vertexBufferObject;
GLuint bg_vertexArrayObject;
GLuint bg_vertexBufferObject;
GLuint bg_colors_vbo;

std::vector<glm::vec3> sf_vertex_buffer_data;
std::vector<glm::vec3> tree_vertex_buffer_data;
std::vector<glm::vec3> bg_vertex_buffer_data;
std::vector<glm::vec3> snow_vertex_buffer_data;

glm::mat4 Projection;
glm::mat4 View;

double startTime;
double startTime2;
double currentTime;
bool isMoveTime = false;
bool mouse_down = false;
// Mouse positions
double xpos, ypos;

float bg_rotation;

std::default_random_engine generator;
// Random number generator
double rng(double min, double max) {
	std::uniform_real_distribution<double> distribution(min, max);
	double rand_num = (double)distribution(generator);
	//std::cout << rand_num << std::endl;
	return rand_num;
}

// Classes
class SnowFlake
{

public:
	double x, y, z, dx, dy, scale;
	float rotation, r_speed;

	SnowFlake()
	{
		this->x = -0.2;
		this->y = -1.2;
		this->dx = 0.0;
		this->dy = 0.001;
		this->scale = 0.2;
		this->rotation = 0.0f;
		this->r_speed = 0.01f;
	}

	SnowFlake(double x, double y, double z, double dx, double dy, double scale, float r_speed)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->dx = dx;
		this->dy = dy;
		this->scale = scale;
		this->rotation = 0.0f;
		this->r_speed = r_speed;
	}

	bool operator==(const SnowFlake& rhs) const
	{
		return this->x == rhs.x && this->y == rhs.y && this->z == rhs.z;
	}

	void move()
	{
		x += dx;
		y += dy;
		rotation = r_speed * (float)glfwGetTime();

		if (dy > -0.01)
			dy -= rng(0.001, 0.003);

		if (dx < -0.006)
			dx += 0.001;
		else if (dx > 0.006)
			dx -= 0.001;
	}

	bool isOutOfBounds()
	{
		return this->y < -0.9 || this->x < (-0.9 * windowRatio) || this->x > (0.9 * windowRatio);
	}
};

class Tree
{

public:
	double x, y, z, scaleX, scaleY;
	int counter = 0;

	Tree()
	{
		this->x = -0.7;
		this->y = 0.0;
		this->z = 0.0;
		this->scaleX = 0.8;
		this->scaleY = 0.8;
	}

	Tree(double x, double y, double z, double scaleX, double scaleY)
	{
		this->x = x;
		this->y = y;
		this->z = z;
		this->scaleX = scaleX;
		this->scaleY = scaleY;
	}

	void move()
	{
		// Bounce/dance effect
		if (this->counter <= 4)
		{
			y -= 0.01;
			scaleY -= 0.01;
			scaleX -= 0.02;
		}
		else if (this->counter <= 6)
		{
			y += 0.02;
			scaleY += 0.02;
			scaleX += 0.04;
		}
		else
		{
			this->counter = 0;
		}
		++(this->counter);
	}
};

// Create a vector where we will store the SnowFlake objects
std::vector<SnowFlake> snowflakes;
// Create a vector where we will store the Tree objects
std::vector<Tree> trees;

// Add a snowflake at a random position
void add_snowflake() {
	double x = rng(-0.95*windowRatio, 0.95*windowRatio);
	double y = rng(0.9, 1.3);
	double z = rng(0.001, 0.1);
	double dx = rng(-0.005, 0.005);
	double dy = rng(-0.02, -0.01);
	double scale;
	if (z > 0.03)
		scale = rng(0.05, 0.08);
	else
		scale = rng(0.01, 0.05);
	float rotation = (float)rng(-200.0, 200.0);

	snowflakes.push_back(SnowFlake(x, y, z, dx, dy, scale, rotation));
}

// Add a snowflake at given position
void add_snowflake(double x, double y) {

	// Normalize x and y to the window coordinates
	x = (x - (double)windowWidth / 2) / ((double)windowWidth / 2) * windowRatio / 1.33;
	y = ((double)windowHeight / 2 - y) / ((double)windowHeight / 2);

	double z = rng(0.001, 0.1);
	double dx = rng(-0.01, 0.01);
	double dy = 0.03;
	double scale;
	if(z > 0.03)
		scale = rng(0.05, 0.08);
	else
		scale = rng(0.01, 0.05);
	float rotation = (float)rng(-200.0, 200.0);

	snowflakes.push_back(SnowFlake(x*1.1, y*0.8, z, dx, dy, scale, rotation));
}

// Callback functions
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key == GLFW_KEY_R && action == GLFW_PRESS)
		snowflakes.clear();
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		mouse_down = true;
	else
		mouse_down = false;
}

void window_size_callback(GLFWwindow* window, int width, int height)
{
	if (height < 80)
	{
		height = 80;
		glfwSetWindowSize(window, width, height);
	}
	else
	{
		windowWidth = width;
		windowHeight = height;
		windowRatio = (double)width / height;
		// std::cout << "New ratio: " << windowRatio << "!" << std::endl;

		// Update projection matrix to keep aspect ratio
		Projection = glm::perspective(45.0f, (float)windowRatio, 0.1f, 100.0f);

		// Set the viewport to be the entire window
		glViewport(0, 0, width, height);
	}
}

// Koch snowflake generation
void koch_line(glm::vec3 a, glm::vec3 b, int iter)
{
	glm::vec3 c = a + (b - a)*1.0f / 3.0f;
	glm::vec3 d = a + (b - a)*2.0f / 3.0f;
	glm::vec3 e = a + (b - a)*1.0f / 2.0f; // Middle point

	// Normal vector, if we define dx=x2-x1 and dy=y2-y1, then the normals are (-dy, dx) and (dy, -dx).
	double dx = d[0] - c[0];
	double dy = d[1] - c[1];
	glm::vec3 normal = glm::vec3(-dy, dx, 0.0f);
	glm::vec3 normal2 = glm::vec3(dy, -dx, 0.0f);
	e = e + (normal - normal2) / glm::distance(normal, normal2) * glm::distance(c, d);

	if (iter <= 0)
	{
		return;
	}
	else
	{
		sf_vertex_buffer_data.push_back(c);
		sf_vertex_buffer_data.push_back(e);
		sf_vertex_buffer_data.push_back(d);

		--iter;

		koch_line(a, b, iter);
		koch_line(a, c, iter);
		koch_line(c, e, iter);
		koch_line(e, d, iter);
		koch_line(d, b, iter);
	}
}

void generate_array_and_buffer(GLuint& vao, GLuint& vbo, std::vector<glm::vec3>& vertex_buffer_data)
{
	// Generates Vertex Array Objects in the GPU’s memory and passes back their identifiers
	// Create a vertex array object that represents vertex attributes stored in a vertex buffer
	// object.
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create and initialize a buffer object, Generates our buffers in the GPU’s memory
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3)*vertex_buffer_data.size(),
		&vertex_buffer_data[0], GL_STATIC_DRAW);
}

// Initialize model
void init_model(void)
{
	// Snowflake
	glm::vec3 a = glm::vec3(-0.5f, -0.25f, 0.0f);
	glm::vec3 b = glm::vec3(0.0f, sqrt(0.75) - 0.25f, 0.0f);
	glm::vec3 c = glm::vec3(0.5f, -0.25f, 0.0f);

	sf_vertex_buffer_data.push_back(a);
	sf_vertex_buffer_data.push_back(b);
	sf_vertex_buffer_data.push_back(c);

	int koch_count = 4;
	koch_line(a, b, koch_count);
	koch_line(b, c, koch_count);
	koch_line(c, a, koch_count);

	// Tree
	glm::vec3 t1 = glm::vec3(-0.4f, -0.25f, 0.0f);
	glm::vec3 t2 = glm::vec3(0.0f, 0.75f, 0.0f);
	glm::vec3 t3 = glm::vec3(0.4f, -0.25f, 0.0f);

	tree_vertex_buffer_data.push_back(t1);
	tree_vertex_buffer_data.push_back(t2);
	tree_vertex_buffer_data.push_back(t3);

	// Trunk
	glm::vec3 tr1 = glm::vec3(-0.08f, -0.25f, 0.0f);
	glm::vec3 tr2 = glm::vec3(0.08f, -0.25f, 0.0f);
	glm::vec3 tr3 = glm::vec3(0.08f, -0.8f, 0.0f);
	glm::vec3 tr4 = glm::vec3(-0.08f, -0.8f, 0.0f);

	tree_vertex_buffer_data.push_back(tr1);
	tree_vertex_buffer_data.push_back(tr2);
	tree_vertex_buffer_data.push_back(tr3);

	tree_vertex_buffer_data.push_back(tr3);
	tree_vertex_buffer_data.push_back(tr4);
	tree_vertex_buffer_data.push_back(tr1);

	// Snow
	glm::vec3 s1 = glm::vec3(-1.3f, 0.25f, 0.0f);
	glm::vec3 s2 = glm::vec3(1.3f, 0.25f, 0.0f);
	glm::vec3 s3 = glm::vec3(1.3f, -0.25f, 0.0f);
	glm::vec3 s4 = glm::vec3(-1.3f, -0.25f, 0.0f);

	snow_vertex_buffer_data.push_back(s1);
	snow_vertex_buffer_data.push_back(s2);
	snow_vertex_buffer_data.push_back(s3);

	snow_vertex_buffer_data.push_back(s3);
	snow_vertex_buffer_data.push_back(s4);
	snow_vertex_buffer_data.push_back(s1);

	// Background
	glm::vec3 bg1 = glm::vec3(-1.0f, 1.0f, 0.0f);
	glm::vec3 bg2 = glm::vec3(1.0f, 1.0f, 0.0f);
	glm::vec3 bg3 = glm::vec3(1.0f, -1.0f, 0.0f);
	glm::vec3 bg4 = glm::vec3(-1.0f, -1.0f, 0.0f);

	bg_vertex_buffer_data.push_back(bg1);
	bg_vertex_buffer_data.push_back(bg2);
	bg_vertex_buffer_data.push_back(bg3);

	bg_vertex_buffer_data.push_back(bg3);
	bg_vertex_buffer_data.push_back(bg4);
	bg_vertex_buffer_data.push_back(bg1);

	// Generate a vertex array object and a buffer object for the the snowflakes
	generate_array_and_buffer(sf_vertexArrayObject, sf_vertexBufferObject, sf_vertex_buffer_data);

	// Generate a vertex array object and a buffer object for the the tree
	generate_array_and_buffer(tree_vertexArrayObject, tree_vertexBufferObject, tree_vertex_buffer_data);

	// Generate a vertex array object and a buffer object for the the snow/ground
	generate_array_and_buffer(snow_vertexArrayObject, snow_vertexBufferObject, snow_vertex_buffer_data);

	// Generate a vertex array object and a buffer object for the the background
	generate_array_and_buffer(bg_vertexArrayObject, bg_vertexBufferObject, bg_vertex_buffer_data);
	// Generate a buffer object for the background colors
	float colors[] = {
		1.0f, 1.0f,  0.4f,
		1.0f, 1.0f,  0.4f,
		0.0f, 0.2f,  0.6f,
		0.0f, 0.2f,  0.6f,
		0.0f, 0.2f,  0.6f,
		1.0f, 1.0f,  0.4f
	};
	glGenBuffers(1, &bg_colors_vbo);
	glBindBuffer(GL_ARRAY_BUFFER, bg_colors_vbo);
	glBufferData(GL_ARRAY_BUFFER, 18 * sizeof(float), colors, GL_STATIC_DRAW);
}

// Draw model
void draw_snowflakes()
{
	glUseProgram(programID);
	glBindVertexArray(sf_vertexArrayObject);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, sf_vertexBufferObject);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), BUFFER_OFFSET(0));

	float colorVec[4] = { 0.94f, 0.95f, 0.9f, 1.0f };
	GLint colorLoc = glGetUniformLocation(programID, "color");
	glProgramUniform4fv(programID, colorLoc, 1, colorVec);

	for (auto &flake : snowflakes) {
		glm::mat4 Model = glm::mat4(1.0);

		// Rotate model in z-axis
		glm::mat4 rotation = glm::rotate(flake.rotation, glm::vec3(0, 0, 1));
		// Translate model’s center in x-y plane
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(flake.x, flake.y, flake.z));
		glm::mat4 RBT = translation * rotation;

		// Scale model in x-y plane
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(flake.scale, flake.scale, 0.0f));
		//Apply to MVP matrix
		glm::mat4 MVP = Projection * View * RBT * scale * Model;

		GLuint MatrixID = glGetUniformLocation(programID, "MVP");
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		glDrawArrays(GL_TRIANGLES, 0, (GLsizei)sf_vertex_buffer_data.size());

		if (isMoveTime) {
			flake.move();
		}
	}

	if (isMoveTime) {
		// Remove a snowflake if it is outside the frame
		snowflakes.erase(
			std::remove_if(
				snowflakes.begin(), snowflakes.end(),
				[](SnowFlake sf) { return sf.isOutOfBounds(); }),
			snowflakes.end());
	}

	glDisableVertexAttribArray(0);
}

void draw_trees()
{
	glUseProgram(programID);
	glBindVertexArray(tree_vertexArrayObject);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, tree_vertexBufferObject);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), BUFFER_OFFSET(0));

	float colorVec[4] = { 0.1f, 0.5f, 0.2f, 1.0f };
	float colorVec2[4] = { 0.5f, 0.3f, 0.1f, 1.0f };
	GLint colorLoc = glGetUniformLocation(programID, "color");

	for (auto &tree : trees) {
		glm::mat4 Model = glm::mat4(1.0f);

		// Rotate model in z-axis
		glm::mat4 rotation = glm::rotate(0.0f, glm::vec3(0, 0, 1));
		// Translate model’s center in x-y plane
		glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(tree.x, tree.y, tree.z));
		glm::mat4 RBT = translation * rotation;

		// Scale model in x-y plane
		glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(tree.scaleX, tree.scaleY, 0.0f));
		//Apply to MVP matrix
		glm::mat4 MVP = Projection * View * RBT * scale;

		GLuint MatrixID = glGetUniformLocation(programID, "MVP");
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		glProgramUniform4fv(programID, colorLoc, 1, colorVec);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glProgramUniform4fv(programID, colorLoc, 1, colorVec2);
		glDrawArrays(GL_TRIANGLES, 3, (GLsizei)tree_vertex_buffer_data.size());

		if (isMoveTime) {
			tree.move();
		}
	}

	glDisableVertexAttribArray(0);
}

void draw_snow()
{
	glUseProgram(programID);
	glBindVertexArray(snow_vertexArrayObject);
	glEnableVertexAttribArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, snow_vertexBufferObject);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), BUFFER_OFFSET(0));

	// Color of the top of the tree
	float colorVec[4] = { 0.85f, 0.85f, 0.9f, 1.0f };
	GLint colorLoc = glGetUniformLocation(programID, "color");

	glm::mat4 Model = glm::mat4(1.0f);

	// Rotate model in z-axis
	glm::mat4 rotation = glm::rotate(-0.2f, glm::vec3(0, 0, 1));
	// Translate model’s center in x-y plane
	glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.8, 0.005));
	glm::mat4 RBT = translation * rotation;

	// Scale model in x-y plane
	glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(1 * windowRatio, 1.2, 0.0f));
	//Apply to MVP matrix
	glm::mat4 MVP = Projection * View * RBT * scale;

	GLuint MatrixID = glGetUniformLocation(program2ID, "MVP");
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

	glProgramUniform4fv(programID, colorLoc, 1, colorVec);
	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)snow_vertex_buffer_data.size());

	glDisableVertexAttribArray(0);
}

void draw_bg()
{
	glUseProgram(program2ID);
	glBindVertexArray(bg_vertexArrayObject);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, bg_vertexBufferObject);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), BUFFER_OFFSET(0));
	glBindBuffer(GL_ARRAY_BUFFER, bg_colors_vbo);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL);

	glm::mat4 Model = glm::mat4(1.0f);

	// Rotate model in z-axis
	bg_rotation = -8.0f * (float)glfwGetTime();
	glm::mat4 rotation = glm::rotate(bg_rotation, glm::vec3(0, 0, 1));
	// Translate model’s center in x-y plane
	glm::mat4 translation = glm::translate(glm::mat4(1.0f), glm::vec3(0, -0.6, 0));
	glm::mat4 RBT = translation * rotation;

	// Scale model in x-y plane
	double scaleRatio = std::max(windowRatio, 1.0);
	glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(2.0 * scaleRatio, 2.0 * scaleRatio, 0.0f));
	//Apply to MVP matrix
	glm::mat4 MVP = Projection * View * RBT * scale;

	GLuint MatrixID = glGetUniformLocation(program2ID, "MVP");
	glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

	glDrawArrays(GL_TRIANGLES, 0, (GLsizei)bg_vertex_buffer_data.size());

	glDisableVertexAttribArray(0);
	glDisableVertexAttribArray(1);
}

int main(int argc, char* argv[])
{
	// Step 1: Initialization
	if (!glfwInit())
	{
		return -1;
	}
	glfwWindowHint(GLFW_SAMPLES, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

	// GLFW create window and context
	window = glfwCreateWindow(windowWidth, windowHeight, "Homework #1 | Fredrik Berglund 20166029 | Snow Animation", NULL, NULL);
	if (!window)
	{
		return -1;
	}
	glfwMakeContextCurrent(window);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);

	glfwSwapInterval(1);

	// Callbacks
	glfwSetKeyCallback(window, key_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetWindowSizeCallback(window, window_size_callback);

	// Initialize GLEW
	glewExperimental = GL_TRUE;
	if (glewInit() != GLEW_OK)
	{
		return -1;
	}
	// END

	Projection = glm::perspective(45.0f, (float)windowRatio, 0.1f, 100.0f);
	View = glm::lookAt(glm::vec3(0, 0, 2),
						glm::vec3(0, 0, 0),
						glm::vec3(0, 1, 0));
	//glm::mat4 Model = glm::mat4(1.0f);
	//glm::mat4 MVP = Projection * View * Model;

	// Initialize OpenGL and GLSL
	glClearColor(0.05f, 0.25f, 0.4f, 0.0f);
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	// For rendering the front side of the object only
	glEnable(GL_CULL_FACE); // cull face
	glCullFace(GL_BACK); // cull back face
	glFrontFace(GL_CW); // GL_CCW for counter clock-wise

	programID = LoadShaders("VertexShader.glsl", "FragmentShader.glsl");
	program2ID = LoadShaders("VertexShader.glsl", "InterpolationFragmentShader.glsl");

	// Set viewport to window size
	glViewport(0, 0, windowWidth, windowHeight);

	init_model();

	// Add initial snowflakes
	for (int i = 0; i < 10; ++i) {
		add_snowflake();
	}

	// Add trees
	trees.push_back(Tree(-0.7, -0.45, 0.03, 1.2, 1.2));
	for (int i = 1; i < 12; ++i) {
		double x = -4.0 + i * 0.65 + rng(-0.2, 0.2);
		double y = rng(-0.4, -0.3);
		double z = rng(0.002, 0.006);
		double scaleX = rng(0.2, 0.35);
		double scaleY = rng(0.2, 0.35);
		trees.push_back(Tree(x, y, z, scaleX, scaleY));
	}

	// Step 2: Main event loop
	startTime, startTime2 = glfwGetTime();
	int tick = 0;
	do {
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glfwGetCursorPos(window, &xpos, &ypos);

		currentTime = glfwGetTime();

		if (currentTime - startTime > 0.05)
		{
			++tick;
		}
		// Add snowflake if mouse is pressed
		if (mouse_down && tick > 2) {
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			add_snowflake(xpos, ypos);
			tick = 0;
		}

		// Add snowflakes at fixed interval
		if (currentTime - startTime2 > 0.3) {
			startTime2 = currentTime;
			add_snowflake();
		}

		if (currentTime - startTime > 0.05) {
			isMoveTime = true;
			startTime = currentTime;
		}
		else {
			isMoveTime = false;
		}

		draw_snowflakes();
		draw_trees();
		draw_snow();
		draw_bg();

		glfwSwapBuffers(window);
		glfwPollEvents();
	} while (!glfwWindowShouldClose(window));

	// Step 3: Termination
	sf_vertex_buffer_data.clear();
	tree_vertex_buffer_data.clear();
	snow_vertex_buffer_data.clear();
	bg_vertex_buffer_data.clear();

	glDeleteBuffers(1, &sf_vertexBufferObject);
	glDeleteBuffers(1, &tree_vertexBufferObject);
	glDeleteBuffers(1, &snow_vertexBufferObject);
	glDeleteBuffers(1, &bg_vertexBufferObject);
	glDeleteBuffers(1, &bg_colors_vbo);
	glDeleteProgram(programID);
	glDeleteProgram(program2ID);
	glDeleteVertexArrays(1, &sf_vertexArrayObject);
	glDeleteVertexArrays(1, &tree_vertexArrayObject);
	glDeleteVertexArrays(1, &snow_vertexArrayObject);
	glDeleteVertexArrays(1, &bg_vertexArrayObject);

	glfwTerminate();

	return 0;
}
