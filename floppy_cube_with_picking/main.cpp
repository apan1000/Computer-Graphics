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
#include <glm/gtc/quaternion.hpp>
#include <glm/ext.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/affine.hpp>
#include <common/geometry.hpp>
#include <common/arcball.hpp>
#include <common/picking.hpp>

const int NUM_CUBES = 9;
const int MAX_PARENTS = 2;
const int MAX_CHILDREN = 4;

class RubiksCube
{
	RubiksCube* parents[MAX_PARENTS];
	RubiksCube* children[MAX_CHILDREN];

	int num_parents = 0;
	int num_children = 0;
	int id;

	double cur_rotation = 0;

public:
	RubiksCube() {}

	RubiksCube(int id) : id(id)
	{
	}

	int addChild(RubiksCube* child)
	{
		if (num_children < MAX_CHILDREN)
		{
			children[num_children] = child;
			++num_children;
			return 0;
		}
		return -1;
	}

	int addParent(RubiksCube* parent)
	{
		if (num_parents < MAX_PARENTS)
		{
			parents[num_parents] = parent;
			++num_parents;
			return 0;
		}
		return -1;
	}

	void setChildren(RubiksCube* c1, RubiksCube* c2, RubiksCube* c3 = NULL, RubiksCube* c4 = NULL)
	{
		children[0] = c1;
		children[1] = c2;
		children[2] = c3;
		children[3] = c4;
		if (c3 && c4)
			num_children = 4;
		else if (c3)
			num_children = 3;
		else
			num_children = 2;
	}

	void setParents(RubiksCube* p1, RubiksCube* p2 = NULL)
	{
		parents[0] = p1;
		parents[1] = p2;
		if (p2)
			num_parents = 2;
		else
			num_parents = 1;
	}

	void switchChildren(RubiksCube* old_child, RubiksCube* new_child)
	{
		for (int i = 0; i < num_children; ++i)
		{
			if (children[i] == old_child)
			{
				children[i] = new_child;
				return;
			}
		}
	}

	void switchParents(RubiksCube* old_parent, RubiksCube* new_parent)
	{
		for (int i = 0; i < num_parents; ++i)
		{
			if (parents[i] == old_parent)
			{
				parents[i] = new_parent;
				return;
			}
		}
	}

	// Return child #x
	RubiksCube* child(int x)
	{
		if (num_children > x)
			return children[x];
		return NULL;
	}

	// Return parent #x
	RubiksCube* parent(int x)
	{
		if (num_parents > x)
			return parents[x];
		return NULL;
	}

	// Rotates by angle and changes parents when cur_rotation reaches +/-180 degrees
	void rotate(double angle)
	{
		double new_rotation = this->cur_rotation + angle;

		if (this->num_children == 2 && (new_rotation >= 180 || new_rotation <= -180))
		{
			RubiksCube* p0 = (this->child(0)->parent(0) == this) ? this->child(0)->parent(1) : this->child(0)->parent(0);
			RubiksCube* p1 = (this->child(1)->parent(0) == this) ? this->child(1)->parent(1) : this->child(1)->parent(0);

			this->child(0)->switchParents(p0, p1);
			this->child(1)->switchParents(p1, p0);

			p0->switchChildren(this->child(0), this->child(1));
			p1->switchChildren(this->child(1), this->child(0));
		}

		this->cur_rotation = fmod(new_rotation, 180);
	}

	// Returns the angle needed to align with middle cube if within +/-15 degrees
	double getAlignAngle()
	{
		double ret_val = 0;

		if(this->cur_rotation >= 165)
		{
			ret_val = 180 - this->cur_rotation;
			return ret_val;
		}
		else if (this->cur_rotation <= -165)
		{
			ret_val = -180 - this->cur_rotation;
			return ret_val;
		}
		else if (this->cur_rotation <= 15 && this->cur_rotation >= -15)
		{
			ret_val = 0 - this->cur_rotation;
			return ret_val;
		}

		return 0;
	}

	int getID() const
	{
		return this->id;
	}

	double rotation() const
	{
		return this->cur_rotation;
	}

	void rotation(double rotation)
	{
		this->cur_rotation = rotation;
	}

	int numChildren() const
	{
		return this->num_children;
	}

	int numParents() const
	{
		return this->num_parents;
	}

	// Returns true if other is a parent to this RubiksCube
	bool hasParent(const RubiksCube& other) const
	{
		if (num_parents > 0 && *parents[0] == other)
			return true;
		else if (num_parents > 1 &&  *parents[1] == other)
		{
			return true;
		}

		return false;
	}

	bool operator==(const RubiksCube& other)
	{
		return this->id == other.id;
	}
};

RubiksCube rubiksCubes[NUM_CUBES];
int rotatingCubes[3];
int pick_num = 0;
int picked_nums[2] = { -1, -1 };
int picked_cube_num = -1; // The currently picked cube number (ID - 1)

glm::vec3 axisVec; // The axis to rotate around
int rot_mid = -1; // Number of the cube we rotate around (ID - 1)

bool picking = true;

float g_groundSize = 100.0f;
float g_groundY = -2.5f;

GLint lightLocGround, lightLocArc;
GLint lightLoc[NUM_CUBES];

// View properties
glm::mat4 Projection;
float windowWidth = 1024.0f;
float windowHeight = 768.0f;
int frameBufferWidth = 0;
int frameBufferHeight = 0;
float fov = 45.0f;
float fovy = fov;

// Model properties
Model ground;
Model rubikModels[NUM_CUBES];
Model pickedModel;

glm::mat4 skyRBT;
glm::mat4 rubikRBT[NUM_CUBES] = {
	glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 1.2f, -1.0f)),
	glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.2f, -1.0f)),
	glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 1.2f, -1.0f)),
	glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, 0.2f, -1.0f)),
	glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.2f, -1.0f)),
	glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 0.2f, -1.0f)),
	glm::translate(glm::mat4(1.0f), glm::vec3(-1.0f, -0.8f, -1.0f)),
	glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.8f, -1.0f)),
	glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, -0.8f, -1.0f))
};
glm::mat4 eyeRBT;
glm::mat4 worldRBT = glm::mat4(1.0f);
glm::mat4 aFrame;
glm::mat4 pickedRBT = glm::mat4(1.0f);

// Arcball manipulation
Model arcBall;
glm::mat4 arcballRBT = glm::mat4(1.0f);
float arcBallScreenRadius = 0.25f * min(windowWidth, windowHeight); // for the initial assignment
float screenToEyeScale = 0.01f;
float arcBallScale = 0.01f;
glm::mat4 arc_scale_m = glm::mat4(1.0f);

glm::vec2 arc_screen_coords;

glm::vec3 startVec;
glm::vec3 endVec;

// Mouse interaction variables
bool l_mouse_down = false;
bool r_mouse_down = false;
bool m_mouse_down = false;
double mouse_x, mouse_y;

// Function definition
static void window_size_callback(GLFWwindow*, int, int);
static void mouse_button_callback(GLFWwindow*, int, int, int);
static void cursor_pos_callback(GLFWwindow*, double, double);
static void keyboard_callback(GLFWwindow*, int, int, int, int);
void update_fovy(void);

// Used to update the rotational axis when the entire floppy cube is rotated
void update_axisVec()
{
	if (rot_mid == 1 || rot_mid == 7 || rot_mid == 3 || rot_mid == 5) // up, down, left or right middle cube
	{
		glm::vec4 a;
		if (rot_mid == 1 || rot_mid == 3)
			a = rubikRBT[rot_mid][3] - rubikRBT[4][3];
		else
			a = rubikRBT[4][3] - rubikRBT[rot_mid][3];

		axisVec = glm::vec3(a[0], a[1], a[2]);
	}
	else if (rot_mid == 4) // the middle cube
	{
		int c1, c2;
		glm::vec4 a;

		if (rotatingCubes[1] == 1 || rotatingCubes[1] == 7)
		{
			c1 = 0;
			c2 = 2;
			a = rubikRBT[(rubiksCubes[rot_mid].child(c2 + 1))->getID() - 1][3] - rubikRBT[rot_mid][3];
		}
		else if (rotatingCubes[1] == 3 || rotatingCubes[1] == 5)
		{
			c1 = 1;
			c2 = 3;
			a = rubikRBT[(rubiksCubes[rot_mid].child(c1 - 1))->getID() - 1][3] - rubikRBT[rot_mid][3];
		}

		axisVec = glm::vec3(a[0], a[1], a[2]);
	}
}

// Helper function: Update the vertical field-of-view(float fovy in global)
void update_fovy()
{
	if (frameBufferWidth >= frameBufferHeight)
	{
		fovy = fov;
	}
	else {
		const float RAD_PER_DEG = 0.5f * glm::pi<float>() / 180.0f;
		fovy = (float) atan2(sin(fov * RAD_PER_DEG) * ((float) frameBufferHeight / (float) frameBufferWidth), cos(fov * RAD_PER_DEG)) / RAD_PER_DEG;
	}
}

// TODO: Modify GLFW window resized callback function
static void window_size_callback(GLFWwindow* window, int width, int height)
{
	// Get resized size and set to current window
	windowWidth = (float)width;
	windowHeight = (float)height;

	// glViewport accept pixel size, please use glfwGetFramebufferSize rather than window size.
	// window size != framebuffer size
	glfwGetFramebufferSize(window, &frameBufferWidth, &frameBufferHeight);
	glViewport(0, 0, (GLsizei) frameBufferWidth, (GLsizei) frameBufferHeight);

	// re-allocate textures with respect to new framebuffer width and height
	reallocate_picking_texture(frameBufferWidth, frameBufferHeight);

	// Update projection matrix
	Projection = glm::perspective(fov, ((float) frameBufferWidth / (float) frameBufferHeight), 0.1f, 100.0f);

	arcBallScreenRadius = 0.25f * min(windowWidth, windowHeight);

	update_fovy();
}

void setRotatingCubes(int c1, int c2)
{
	picking = false;

	rotatingCubes[0] = rubiksCubes[rot_mid].getID() - 1;
	rotatingCubes[1] = (rubiksCubes[rot_mid].child(c1))->getID() - 1;
	rotatingCubes[2] = (rubiksCubes[rot_mid].child(c2))->getID() - 1;
}

void checkSelection()
{
	rot_mid = -1;

	if (rubiksCubes[picked_nums[0]].hasParent(rubiksCubes[picked_nums[1]]))
	{
		rot_mid = picked_nums[1];

		if (rubiksCubes[rot_mid].numParents() == 1) // Not middle cube
		{
			// Check if parent and children have same rotation
			if (rubiksCubes[rot_mid].rotation() != rubiksCubes[rot_mid].child(0)->rotation()
				|| rubiksCubes[rot_mid].rotation() != rubiksCubes[rot_mid].child(1)->rotation())
			{
				std::cout << "Parent and children have different rotations." << std::endl;
				return;
			}

			glm::vec4 a;
			if (rot_mid == 1 || rot_mid == 3)
				a = rubikRBT[rot_mid][3] - rubikRBT[4][3];
			else
				a = rubikRBT[4][3] - rubikRBT[rot_mid][3];

			axisVec = glm::vec3(a[0], a[1], a[2]);

			setRotatingCubes(0, 1);
		}
		else if (rubiksCubes[rot_mid].numParents() == 0) // 1 is Middle cube, 0 is it's child
		{
			int c1, c2;
			glm::vec4 a;

			if (picked_nums[0] == 1 || picked_nums[0] == 7)
			{
				c1 = 0;
				c2 = 2;
				a = rubikRBT[(rubiksCubes[rot_mid].child(c2 + 1))->getID() - 1][3] - rubikRBT[rot_mid][3];
			}
			else if (picked_nums[0] == 3 || picked_nums[0] == 5)
			{
				c1 = 1;
				c2 = 3;
				a = rubikRBT[(rubiksCubes[rot_mid].child(c1 - 1))->getID() - 1][3] - rubikRBT[rot_mid][3];
			}

			// Check if parent and children have same rotation
			if (rubiksCubes[rot_mid].rotation() != rubiksCubes[rot_mid].child(c1)->rotation()
				|| rubiksCubes[rot_mid].rotation() != rubiksCubes[rot_mid].child(c2)->rotation())
			{
				std::cout << "Parent and children have different rotations." << std::endl;
				return;
			}

			axisVec = glm::vec3(a[0], a[1], a[2]);

			setRotatingCubes(c1, c2);
		}
	}
	else if (rubiksCubes[picked_nums[1]].hasParent(rubiksCubes[picked_nums[0]]))
	{
		rot_mid = picked_nums[0];

		if (rubiksCubes[rot_mid].numParents() == 1) // Not middle cube
		{
			if (rubiksCubes[rot_mid].rotation() != rubiksCubes[rot_mid].child(0)->rotation()
				|| rubiksCubes[rot_mid].rotation() != rubiksCubes[rot_mid].child(1)->rotation())
			{
				std::cout << "Parent and children have different rotations." << std::endl;
				return;
			}

			glm::vec4 a;
			if (rot_mid == 1 || rot_mid == 3)
				a = rubikRBT[rot_mid][3] - rubikRBT[4][3];
			else
				a = rubikRBT[4][3] - rubikRBT[rot_mid][3];

			axisVec = glm::vec3(a[0], a[1], a[2]);

			setRotatingCubes(0, 1);
		}
		else if (rubiksCubes[rot_mid].numParents() == 0) // 0 is Middle cube, 1 is it's child
		{
			int c1, c2;
			glm::vec4 a;

			if (picked_nums[1] == 1 || picked_nums[1] == 7)
			{
				c1 = 0;
				c2 = 2;
				a = rubikRBT[(rubiksCubes[rot_mid].child(c2 + 1))->getID() - 1][3] - rubikRBT[rot_mid][3];
			}
			else if (picked_nums[1] == 3 || picked_nums[1] == 5)
			{
				c1 = 1;
				c2 = 3;
				a = rubikRBT[(rubiksCubes[rot_mid].child(c1 - 1))->getID() - 1][3] - rubikRBT[rot_mid][3];
			}

			// Check if parent and children have same rotation
			if (rubiksCubes[rot_mid].rotation() != rubiksCubes[rot_mid].child(c1)->rotation()
				|| rubiksCubes[rot_mid].rotation() != rubiksCubes[rot_mid].child(c2)->rotation())
			{
				std::cout << "Parent and children have different rotations." << std::endl;
				return;
			}

			axisVec = glm::vec3(a[0], a[1], a[2]);

			setRotatingCubes(c1, c2);
		}
	}
	else if (rubiksCubes[picked_nums[0]].numParents() == rubiksCubes[picked_nums[1]].numParents()) // 0 and 1 have same amount of parents
	{
		for (int i = 0; i < rubiksCubes[picked_nums[0]].numParents(); ++i)
		{
			for (int j = 0; j < rubiksCubes[picked_nums[1]].numParents(); ++j)
			{
				if (rubiksCubes[picked_nums[0]].parent(i) == rubiksCubes[picked_nums[1]].parent(j))
				{
					// Check for unallowed children-of-middle-cube combinations
					if (( (rubiksCubes[picked_nums[0]].getID() - 1) == 1
						&& (rubiksCubes[picked_nums[1]].getID() - 1) != 7 )
						||
						( (rubiksCubes[picked_nums[1]].getID() - 1) == 1
							&& (rubiksCubes[picked_nums[0]].getID() - 1) != 7 ))
					{
						return;
					}
					else if (( (rubiksCubes[picked_nums[0]].getID() - 1) == 3
						&& (rubiksCubes[picked_nums[1]].getID() - 1) != 5)
						||
						( (rubiksCubes[picked_nums[1]].getID() - 1) == 3
							&& (rubiksCubes[picked_nums[0]].getID() - 1 != 5) ))
					{
						return;
					}
					else if (((rubiksCubes[picked_nums[0]].getID() - 1) == 5
						&& (rubiksCubes[picked_nums[1]].getID() - 1) == 7)
						||
						((rubiksCubes[picked_nums[1]].getID() - 1) == 5
							&& (rubiksCubes[picked_nums[0]].getID() - 1 == 7)))
					{
						return;
					}


					rot_mid = rubiksCubes[picked_nums[0]].parent(i)->getID() - 1;

					if (rubiksCubes[rot_mid].numParents() == 1) // Not middle cube
					{
						// Check if parent and children have same rotation
						if (rubiksCubes[rot_mid].rotation() != rubiksCubes[rot_mid].child(0)->rotation()
							|| rubiksCubes[rot_mid].rotation() != rubiksCubes[rot_mid].child(1)->rotation())
						{
							std::cout << "Parent and children have different rotations." << std::endl;
							return;
						}

						glm::vec4 a;
						if (rot_mid == 1 || rot_mid == 3)
							a = rubikRBT[rot_mid][3] - rubikRBT[4][3];
						else
							a = rubikRBT[4][3] - rubikRBT[rot_mid][3];

						axisVec = glm::vec3(a[0], a[1], a[2]);

						setRotatingCubes(0, 1);
						return;
					}
					else if (rubiksCubes[rot_mid].numParents() == 0) // 0 is Middle cube, 1 is it's child
					{
						int c1, c2;
						glm::vec4 a;

						if (picked_nums[1] == 1 || picked_nums[1] == 7)
						{
							c1 = 0;
							c2 = 2;
							a = rubikRBT[(rubiksCubes[rot_mid].child(c2 + 1))->getID() - 1][3] - rubikRBT[rot_mid][3];
						}
						else if (picked_nums[1] == 3 || picked_nums[1] == 5)
						{
							c1 = 1;
							c2 = 3;
							a = rubikRBT[(rubiksCubes[rot_mid].child(c1 - 1))->getID() - 1][3] - rubikRBT[rot_mid][3];
						}

						// Check if parent and children have same rotation
						if (rubiksCubes[rot_mid].rotation() != rubiksCubes[rot_mid].child(c1)->rotation()
							|| rubiksCubes[rot_mid].rotation() != rubiksCubes[rot_mid].child(c2)->rotation())
						{
							std::cout << "Parent and children have different rotations." << std::endl;
							return;
						}

						axisVec = glm::vec3(a[0], a[1], a[2]);

						setRotatingCubes(c1, c2);
						return;
					}
				}
			}
		}
	}
}

// Align the cubes if they are within some degrees of each other
void alignCubes()
{
	if (rot_mid == 4)
	{
		int parent_nums[4] = { 1, 3, 5, 7 };
		for (int i : parent_nums)
		{
			if (i != rotatingCubes[1] && i != rotatingCubes[2])
			{
				int c_id1 = rubiksCubes[i].child(0)->getID() - 1;
				int c_id2 = rubiksCubes[i].child(1)->getID() - 1;

				double angle = rubiksCubes[i].getAlignAngle();

				if (angle != 0)
				{
					glm::mat4 A = transFact(rubikRBT[i]) * linearFact(eyeRBT);

					glm::quat my_quat = glm::angleAxis((float)angle, axisVec);
					if (glm::isnan(my_quat.x) || glm::isnan(my_quat.y) || glm::isnan(my_quat.z))
						return;
					glm::mat4 Q = glm::toMat4(my_quat);

					// Rotate other cubes
					rubiksCubes[i].rotate(angle);
					rubiksCubes[c_id1].rotation(rubiksCubes[i].rotation());
					rubiksCubes[c_id2].rotation(rubiksCubes[i].rotation());

					rubikRBT[i] = A * Q * glm::inverse(A) * rubikRBT[i];
					rubikRBT[c_id1] = A * Q * glm::inverse(A) * rubikRBT[c_id1];
					rubikRBT[c_id2] = A * Q * glm::inverse(A) * rubikRBT[c_id2];
				}
			}
		}
	}
	else
	{
		glm::mat4 A = transFact(rubikRBT[rot_mid]) * linearFact(eyeRBT);

		double angle = rubiksCubes[rot_mid].getAlignAngle();

		glm::quat my_quat = glm::angleAxis((float)angle, axisVec);
		if (glm::isnan(my_quat.x) || glm::isnan(my_quat.y) || glm::isnan(my_quat.z))
			return;
		glm::mat4 Q = glm::toMat4(my_quat);

		rubiksCubes[rot_mid].rotate(angle);
		rubiksCubes[rotatingCubes[1]].rotation(rubiksCubes[rotatingCubes[0]].rotation());
		rubiksCubes[rotatingCubes[2]].rotation(rubiksCubes[rotatingCubes[0]].rotation());

		// Apply rotation to all cubes
		for (int i = 0; i < 3; ++i)
		{
			rubikRBT[rotatingCubes[i]] = A * Q * glm::inverse(A) * rubikRBT[rotatingCubes[i]];
		}
	}
}

// TODO: Fill up GLFW mouse button callback function
static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if(button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
	{
		if (picking)
		{
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			int target = pick((int)xpos, (int)ypos, frameBufferWidth, frameBufferHeight);

			if (target > 0)
			{
				if (picked_nums[(pick_num + 1) % 2] == target - 1)
					return;
				picked_nums[pick_num] = target - 1;

				picked_cube_num = picked_nums[pick_num];

				pick_num = (pick_num + 1) % 2;
			}
			else
				return;

			if (pick_num == 0)
			{
				checkSelection();

				picked_nums[0] = picked_nums[1];
				pick_num = 1;
			}
		}
		else
			l_mouse_down = true;
	}
	else
		l_mouse_down = false;

	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
		r_mouse_down = true;
	else
		r_mouse_down = false;

	if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
		m_mouse_down = true;
	else
		m_mouse_down = false;
}

// TODO: Fill up GLFW cursor position callback function
static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	const double new_x = xpos;
	const double new_y = ypos;

	glm::mat4 Q = glm::mat4(1.0f);
	glm::mat4 T;
	glm::mat4 L;
	glm::quat my_quat;

	if (r_mouse_down)
		T = transFact(rubikRBT[4]);
	else if (m_mouse_down)
		T = transFact(eyeRBT);
	else
		T = transFact(arcballRBT);

	L = linearFact(eyeRBT);
	
	glm::mat4 A = T * L;

	double delta_x = (new_x - mouse_x);
	double delta_y = (new_y - mouse_y);

	if (l_mouse_down && !picking)
	{
		float x1 = (float)mouse_x - arc_screen_coords[0];
		float y1 = (float)-mouse_y + arc_screen_coords[1];
		float z1 = sqrt(max(0.0f, (pow(arcBallScreenRadius, 2) - pow(x1, 2) - pow(y1, 2))));
		startVec = glm::normalize(glm::vec3(x1, y1, z1));

		float x2 = (float)xpos - arc_screen_coords[0];
		float y2 = (float)-ypos + arc_screen_coords[1];

		float z2 = sqrt(max(0.0f, (pow(arcBallScreenRadius, 2) - pow(x2, 2) - pow(y2, 2))));
		endVec = glm::normalize(glm::vec3(x2, y2, z2));

		glm::vec3 k = glm::cross(startVec, endVec);
		glm::vec3 kproj = glm::proj(k, axisVec);
		k = glm::normalize(kproj);
		double angle = glm::degrees(glm::acos(glm::dot(startVec, endVec)));
		double angle_diff = 0;
		int pos_or_neg = 1;

		if (glm::distance(axisVec, kproj) > 1)
		{
			pos_or_neg = -1;
		}
		else
		{
			pos_or_neg = 1;
		}

		my_quat = glm::angleAxis((float)angle, k);
		if (glm::isnan(my_quat.x) || glm::isnan(my_quat.y) || glm::isnan(my_quat.z))
			return;
		Q = glm::toMat4(my_quat);

		if (rot_mid == 4)
		{
			for (int i = 0; i < 9; ++i)
			{
				if(i != rotatingCubes[0] && i != rotatingCubes[1] && i != rotatingCubes[2])
					rubiksCubes[i].rotate(angle * (-pos_or_neg)); // Rotate other cubes in oposite direction
			}
		}
		else
		{
			rubiksCubes[rot_mid].rotate(angle * pos_or_neg);
			rubiksCubes[rotatingCubes[1]].rotation(rubiksCubes[rotatingCubes[0]].rotation());
			rubiksCubes[rotatingCubes[2]].rotation(rubiksCubes[rotatingCubes[0]].rotation());
		}

		// Apply rotation to all cubes
		for (int i = 0; i < 3; ++i)
		{
			//rubiksCubes[rotatingCubes[i]].rotate(angle, now);
			rubikRBT[rotatingCubes[i]] = A * Q * glm::inverse(A) * rubikRBT[rotatingCubes[i]];
		}
	}
	else if (r_mouse_down)
	{
		float x1 = (float)mouse_x - arc_screen_coords[0];
		float y1 = (float)-mouse_y + arc_screen_coords[1];
		float z1 = sqrt(max( 0.0f, (pow(arcBallScreenRadius, 2) - pow(x1, 2) - pow(y1, 2)) ));
		startVec = glm::normalize(glm::vec3(x1, y1, z1));

		float x2 = (float)xpos - arc_screen_coords[0];
		float y2 = (float)-ypos + arc_screen_coords[1];

		float z2 = sqrt(max(0.0f, (pow(arcBallScreenRadius, 2) - pow(x2, 2) - pow(y2, 2))));
		endVec = glm::normalize(glm::vec3(x2, y2, z2));

		glm::vec3 k = glm::normalize(glm::cross(startVec, endVec));
		float angle = glm::degrees(glm::acos(glm::dot(startVec, endVec)));

		my_quat = glm::angleAxis(angle, k);
		if (glm::isnan(my_quat.x) || glm::isnan(my_quat.y) || glm::isnan(my_quat.z))
			return;
		Q = glm::toMat4(my_quat);

		// Apply rotation to all cubes
		for (int i = 0; i < NUM_CUBES; ++i)
		{
			rubikRBT[i] = A * Q * glm::inverse(A) * rubikRBT[i];
		}

		// Update the axis vector we rotate around
		update_axisVec();
	}
	else if (m_mouse_down)
	{
		Q = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -delta_y * 0.015));
		eyeRBT = A * Q * glm::inverse(A) * eyeRBT;
	}

	mouse_x = new_x;
	mouse_y = new_y;
}

static void keyboard_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_H:
			std::cout << "CS380 Homework Assignment 2" << std::endl;
			std::cout << "keymaps:" << std::endl;
			std::cout << "h\t\t\t Help command" << std::endl;
			std::cout << "p\t\t\t Enable/Disable picking" << std::endl;
			std::cout << "left mouse btn\t\t Pick cubes when picking, else rotate selected group" << std::endl;
			std::cout << "right mouse btn\t\t Rotate whole floppy cube" << std::endl;
			std::cout << "middle mouse btn\t Move floppy cube toward/away from camera" << std::endl;
			break;
		case GLFW_KEY_P:
			// TODO: Enable/Disable picking
			picking = !picking;

			// Reset picked_nums & pick_num
			picked_nums[0] = picked_nums[1] = -1;
			pick_num = 0;

			alignCubes();
			break;
		default:
			break;
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
	window = glfwCreateWindow((int)windowWidth, (int)windowHeight, "Homework 3: 20166029 - Fredrik Berglund", NULL, NULL);
	if (window == NULL) {
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Initialize GLEW
	glewExperimental = (GLboolean) true; // Needed for core profile
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
	// Update arcBallScreenRadius with framebuffer size
	arcBallScreenRadius = 0.25f * min((float) frameBufferWidth, (float) frameBufferHeight); // for the initial assignment

	// Clear with sky color
	glClearColor((GLclampf)(128. / 255.), (GLclampf)(200. / 255.), (GLclampf)(255. / 255.), (GLclampf) 0.);

	// Enable depth test
	glEnable(GL_DEPTH_TEST);
	// Accept fragment if it closer to the camera than the former one
	glDepthFunc(GL_LESS);
	// Enable culling
	glEnable(GL_CULL_FACE);
	// Backface culling
	glCullFace(GL_BACK);

	// Initialize framebuffer object and picking textures
	picking_initialize(frameBufferWidth, frameBufferHeight);

	Projection = glm::perspective(fov, ((float) frameBufferWidth / (float) frameBufferHeight), 0.1f, 100.0f);
	skyRBT = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.25, 4.0));

	// initial eye frame = sky frame;
	eyeRBT = skyRBT;

	// Initialize Ground Model
	ground = Model();
	init_ground(ground);
	ground.initialize(DRAW_TYPE::ARRAY, "VertexShader.glsl", "FragmentShader.glsl"); //
	ground.set_projection(&Projection);
	ground.set_eye(&eyeRBT);
	glm::mat4 groundRBT = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, g_groundY, 0.0f)) * glm::scale(glm::mat4(1.0f), glm::vec3(g_groundSize, 1.0f, g_groundSize));
	ground.set_model(&groundRBT);

	glm::vec3 colors[6] = {
		glm::vec3(225/255.f, 50/255.f, 50/255.f),
		glm::vec3(50/255.f, 225/255.f, 50/255.f),
		glm::vec3(50/255.f, 50/255.f, 225/255.f),
		glm::vec3(190/255.f, 190/255.f, 190/255.f),
		glm::vec3(200/255.f, 150/255.f, 0/255.f),
		glm::vec3(20/255.f, 20/255.f, 20/255.f),
	};
	glm::vec3 colors2[6] = {
		glm::vec3(255 / 255.f, 0 / 255.f, 0 / 255.f),
		glm::vec3(0 / 255.f, 255 / 255.f, 0 / 255.f),
		glm::vec3(0 / 255.f, 0 / 255.f, 255 / 255.f),
		glm::vec3(255 / 255.f, 255 / 255.f, 255 / 255.f),
		glm::vec3(255 / 255.f, 200 / 255.f, 0 / 255.f),
		glm::vec3(0 / 255.f, 0 / 255.f, 0 / 255.f),
	};

	for (int i = 0; i < NUM_CUBES; ++i)
	{
		rubikModels[i] = Model();
		init_rubic(rubikModels[i], colors);
		rubikModels[i].initialize(DRAW_TYPE::ARRAY, "VertexShader.glsl", "ReflectiveFragmentShader.glsl");
		rubikModels[i].initialize_picking("PickingVertexShader.glsl", "PickingFragmentShader.glsl");
		rubikModels[i].set_projection(&Projection);
		rubikModels[i].set_eye(&eyeRBT);
		rubikModels[i].set_model(&rubikRBT[i]);

		rubikModels[i].objectID = i+1;
	}

	pickedModel = Model();
	init_rubic(pickedModel, colors2);
	pickedModel.initialize(DRAW_TYPE::ARRAY, "VertexShader.glsl", "FragmentShader.glsl");
	pickedModel.initialize_picking("PickingVertexShader.glsl", "PickingFragmentShader.glsl");
	pickedModel.set_projection(&Projection);
	pickedModel.set_eye(&eyeRBT);
	pickedModel.set_model(&pickedRBT);

	// TODO: Initialize arcBall
	// Initialize your arcBall with DRAW_TYPE::INDEX (it uses GL_ELEMENT_ARRAY_BUFFER to draw sphere)
	arcBall = Model();
	init_sphere(arcBall);
	arcBall.initialize(DRAW_TYPE::INDEX, "VertexShader.glsl", "FragmentShader.glsl");
	arcBall.set_projection(&Projection);
	arcBall.set_eye(&eyeRBT);
	arcBall.set_model(&arcballRBT);

	// Setting Light Vectors
	glm::vec3 lightVec = glm::vec3(0.0f, 1.0f, 0.0f);
	lightLocGround = glGetUniformLocation(ground.GLSLProgramID, "uLight");
	glUniform3f(lightLocGround, lightVec.x, lightVec.y, lightVec.z);

	for (int i = 0; i < NUM_CUBES; ++i)
	{
		lightLoc[i] = glGetUniformLocation(rubikModels[i].GLSLProgramID, "uLight");
		glUniform3f(lightLoc[i], lightVec.x, lightVec.y, lightVec.z);
	}

	lightLocArc = glGetUniformLocation(arcBall.GLSLProgramID, "uLight");
	glUniform3f(lightLocArc, lightVec.x, lightVec.y, lightVec.z);

	// Initialize RubiksCubes
	for (int i = 0; i < NUM_CUBES; ++i)
	{
		rubiksCubes[i] = RubiksCube(i + 1);
	}
	rubiksCubes[4].setChildren(&rubiksCubes[1], &rubiksCubes[5], &rubiksCubes[7], &rubiksCubes[3]);

	rubiksCubes[1].addParent(&rubiksCubes[4]);
	rubiksCubes[5].addParent(&rubiksCubes[4]);
	rubiksCubes[7].addParent(&rubiksCubes[4]);
	rubiksCubes[3].addParent(&rubiksCubes[4]);

	rubiksCubes[1].setChildren(&rubiksCubes[0], &rubiksCubes[2]);
	rubiksCubes[5].setChildren(&rubiksCubes[2], &rubiksCubes[8]);
	rubiksCubes[7].setChildren(&rubiksCubes[8], &rubiksCubes[6]);
	rubiksCubes[3].setChildren(&rubiksCubes[6], &rubiksCubes[0]);

	rubiksCubes[0].setParents(&rubiksCubes[3], &rubiksCubes[1]);
	rubiksCubes[2].setParents(&rubiksCubes[1], &rubiksCubes[5]);
	rubiksCubes[8].setParents(&rubiksCubes[5], &rubiksCubes[7]);
	rubiksCubes[6].setParents(&rubiksCubes[7], &rubiksCubes[3]);

	do {
		// first pass: picking shader
		// binding framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, picking_fbo);
		// Background: RGB = 0x000000 => objectID: 0
		glClearColor((GLclampf) 0.0f, (GLclampf) 0.0f, (GLclampf) 0.0f, (GLclampf) 0.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// drawing objects in framebuffer (picking process)
		for (int i = 0; i < NUM_CUBES; ++i)
		{
			rubikModels[i].drawPicking();
		}

		// second pass: your drawing
		// unbinding framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor((GLclampf)(128. / 255.), (GLclampf)(200. / 255.), (GLclampf)(255. / 255.), (GLclampf)0.);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		if (r_mouse_down)
			arcballRBT = rubikRBT[4];
		else if (rot_mid != -1 && !picking)
		{
			arcballRBT = rubikRBT[rot_mid];
		}
		else
			arcballRBT = eyeRBT;
		glm::mat4 arcEye = glm::inverse(eyeRBT) * arcballRBT; // arcBall in eye-coordinates

		// Only change scale when not transforming
		if (!l_mouse_down && !m_mouse_down)
		{
			arcBallScale = arcBallScreenRadius * compute_screen_eye_scale(arcEye[3][2], fovy, frameBufferHeight);
			arc_scale_m = glm::scale(arcBallScale, arcBallScale, arcBallScale);
		}
		arcballRBT *= arc_scale_m;

		// Set arc_screen_coords
		glm::vec3 arc_coords = glm::vec3(arcEye[3][0], arcEye[3][1], arcEye[3][2]);
		arc_screen_coords = eye_to_screen(arc_coords, Projection, frameBufferWidth, frameBufferHeight);
		arc_screen_coords.y = -(arc_screen_coords.y - frameBufferHeight);

		// Draw rubik models
		for (int i = 0; i < NUM_CUBES; ++i)
		{
			if(!picking || r_mouse_down || i != picked_cube_num || picked_nums[0] == -1)
				rubikModels[i].draw();
		}

		pickedRBT = rubikRBT[picked_cube_num];
		if(!r_mouse_down && picking && picked_nums[0] != -1)
			pickedModel.draw();
		ground.draw();

		// TODO: Draw wireframe of arcball with dynamic radius
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE); // draw wireframe
		arcBall.draw();
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); // draw filled models again

		// Swap buffers (Double buffering)
		glfwSwapBuffers(window);
		glfwPollEvents();
	} // Check if the ESC key was pressed or the window was closed
	while (glfwGetKey(window, GLFW_KEY_ESCAPE) != GLFW_PRESS &&
		glfwWindowShouldClose(window) == 0);

	// Clean up data structures and glsl objects
	ground.cleanup();
	for (int i = 0; i < NUM_CUBES; ++i)
		rubikModels[i].cleanup();
	arcBall.cleanup();

	// Cleanup textures
	delete_picking_resources();

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}
