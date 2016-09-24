
#include <vector>
#include <glm.hpp>
#include <string>
#include <iostream>
#include <fstream>

#include "control.h"

// GLEW
//#define GLEW_STATIC
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

#define CONTAINER  0
#define CAMERA 1
#define PLAYER 2
#define LIGHT 3
#define SPHERE 4
#define SPHERE_INTERIOR 5
#define BOX 6
#define BOX_INTERIOR 7
#define TORUS 8

#define MAX_OBJECTS 20
#define UNIFORMS_PER_OBJECT 10

namespace rme
{

	class Material
	{
	public:
		glm::vec3 color;
		float shininess;
		float luminance;
		int shading;
		Material();
		// Texture?
		//float n; index of refraction
	};
	
	class Object3D
	{
	public:
		std::string name;
		glm::vec3 position;
		glm::vec3 correction;
		glm::vec3 velocity;
		glm::vec3 direction;
		//glm::mat4 translation;
		float radius;
		glm::vec3 shape;
		int geometry;
		float mass;
		float charge;
		float age;
		bool collisions;
		bool physics;
		std::vector<Object3D> children;
		glm::vec3 color;
	//	Material *material;
		Object3D(std::string n);
	};

	struct shaderObject3D {
		GLuint position; //vec3
		GLuint direction; //vec3
		//GLuint translation; //mat4
		GLuint radius; //float
		GLuint age; //float
		GLuint shape; //vec3
		GLuint geometry; //int
		GLuint mass; //float
		GLuint shininess; //float
		GLuint luminance; //float
		GLuint color; //vec3
		GLuint shading; //int
	};

	class Camera :public Object3D
	{
	public:
		glm::vec4 rotation;
		Camera(std::string n);
	};
	 
	class Sphere :public Object3D 
	{
	public:
		Sphere(std::string n);
	};

	class BoxInterior :public Object3D
	{ 
	public:
		BoxInterior(std::string n);
	};

	class Scene
	{
		float map(glm::vec3, int exclude);
		float sdSphere(glm::vec3 p, float s);
		float sdTorus(glm::vec3 p, glm::vec2 t);
		float sdRoundBox(glm::vec3 p, glm::vec3 b, float r);
		float sdBoxInterior(glm::vec3 p, glm::vec3 b);
		glm::vec3 normal(glm::vec3 p, int exclude);
		
	public:
		std::vector<Object3D*> children;
		//Controls control;
		Scene();
		void add(Object3D *obj);
		void remove(std::string name);
		void spawn(Camera *camera);
		glm::vec2 rot2D(glm::vec2 p, float angle);
		void update();
	};

	class RaymarchRenderer
	{
		std::string loadSource(char* filename);
		/*const*/ GLuint width, height;
		GLuint VBO, VAO, EBO;
		GLuint shaderProgram;
		GLuint timeLocation, resolutionLocation, rotationLocation, camPosLocation, objCountLocation, warpALoc, warpBLoc, warpCountLoc;
		shaderObject3D *objectLocations;
		void updateUniforms(Scene* scene, Camera* camera);
		
	public:
		RaymarchRenderer(int w, int h);
		~RaymarchRenderer();
		GLFWwindow* window;
		void resize(int x, int y);
		void render(Scene* scene, Camera* camera);
	};

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

	static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos);

}