
#include "rme.h"
#include "Control.h"

Controls *control = new Controls();

namespace rme
{

	RaymarchRenderer::RaymarchRenderer(int w, int h)
	{
		width = w;
		height = h;

		// Init GLFW
		glfwInit();
		// Set all the required options for GLFW
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
		
		// Create a GLFWwindow object that we can use for GLFW's functions
		window = glfwCreateWindow(width, height, "Time", nullptr, nullptr);
		glfwMakeContextCurrent(window);

		// Disable mouse 
		glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		// Set the required callback functions
		glfwSetKeyCallback(window, key_callback);
		glfwSetCursorPosCallback(window, cursor_pos_callback);
		glfwSetMouseButtonCallback(window, mouse_button_callback);

		// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
		glewExperimental = GL_TRUE;
		// Initialize GLEW to setup the OpenGL Function pointers
		glewInit();
		
		// get version info
		const GLubyte* hardware = glGetString(GL_RENDERER); // get renderer string
		const GLubyte* version = glGetString(GL_VERSION); // version as a string
		printf("Hardware: %s\n", hardware);
		printf("OpenGL version supported %s\n", version);

		// Define the viewport dimensions
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		glViewport(0, 0, width, height);

		// Load shaders
		std::string vert = loadSource("shaders/pass.vert");
		std::string frag = loadSource("shaders/march.frag");
		//std::printf("source: %s\n", vert.c_str());
		const GLchar* vertexShaderSource = (const GLchar *)vert.c_str();
		const GLchar* fragmentShaderSource = (const GLchar *)frag.c_str();

		// Build and compile our shader program
		// Vertex shader
		GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
		glCompileShader(vertexShader);
		// Check for compile time errors
		GLint success;
		GLchar infoLog[512];
		glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
		}
		// Fragment shader
		GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
		glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
		glCompileShader(fragmentShader);
		// Check for compile time errors
		glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
		if (!success)
		{
			glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
		}
		// Link shaders
		shaderProgram = glCreateProgram();
		glAttachShader(shaderProgram, vertexShader);
		glAttachShader(shaderProgram, fragmentShader);
		glLinkProgram(shaderProgram);
		// Check for linking errors
		glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
		if (!success) {
			glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
			std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
		}
		glDeleteShader(vertexShader);
		glDeleteShader(fragmentShader);


		// Set up vertex data (and buffer(s)) and attribute pointers
		//GLfloat vertices[] = {
		//  // First triangle
		//   0.5f,  0.5f,  // Top Right
		//   0.5f, -0.5f,  // Bottom Right
		//  -0.5f,  0.5f,  // Top Left 
		//  // Second triangle
		//   0.5f, -0.5f,  // Bottom Right
		//  -0.5f, -0.5f,  // Bottom Left
		//  -0.5f,  0.5f   // Top Left
		//}; 
		GLfloat vertices[] = {
			1.0f, 1.0f, 0.0f,  // Top Right
			1.0f, -1.0f, 0.0f,  // Bottom Right
			-1.0f, -1.0f, 0.0f,  // Bottom Left
			-1.0f, 1.0f, 0.0f   // Top Left 
		};
		GLuint indices[] = {  // Note that we start from 0!
			0, 1, 3,  // First Triangle
			1, 2, 3   // Second Triangle
		};
		//GLuint VBO, VAO, EBO;
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
		// Bind the Vertex Array Object first, then bind and set vertex buffer(s) and attribute pointer(s).
		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);
		glEnableVertexAttribArray(0);

		glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to 
		// glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind

		glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any 
		// buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO

		timeLocation = glGetUniformLocation(shaderProgram, "time");
		resolutionLocation = glGetUniformLocation(shaderProgram, "resolution");
		objCountLocation = glGetUniformLocation(shaderProgram, "objectCount");
		rotationLocation = glGetUniformLocation(shaderProgram, "cameraRotation");
		camPosLocation = glGetUniformLocation(shaderProgram, "cameraPos");

		warpALoc = glGetUniformLocation(shaderProgram, "warpA");
		warpBLoc = glGetUniformLocation(shaderProgram, "warpB");

		warpCountLoc = glGetUniformLocation(shaderProgram, "warpCount");

		glUseProgram(shaderProgram);

		glUniform2f(resolutionLocation, (GLfloat)width, (GLfloat)height);

		glUniform1i(objCountLocation, 0);

		glUniform2f(rotationLocation, 0.0, 0.0);

		objectLocations = new shaderObject3D[MAX_OBJECTS];

		for (int i = 0; i < MAX_OBJECTS; i++)
		{
		///	shaderObject3D *current = 
			objectLocations[i].position = glGetUniformLocation(shaderProgram, ("objects[" + std::to_string(i) + "].position").c_str());
			objectLocations[i].direction = glGetUniformLocation(shaderProgram, ("objects[" + std::to_string(i) + "].direction").c_str());
			objectLocations[i].radius = glGetUniformLocation(shaderProgram, ("objects[" + std::to_string(i) + "].radius").c_str());
			objectLocations[i].age = glGetUniformLocation(shaderProgram, ("objects[" + std::to_string(i) + "].age").c_str());
			objectLocations[i].shape = glGetUniformLocation(shaderProgram, ("objects[" + std::to_string(i) + "].shape").c_str());
			objectLocations[i].geometry = glGetUniformLocation(shaderProgram, ("objects[" + std::to_string(i) + "].geometry").c_str());
			objectLocations[i].mass = glGetUniformLocation(shaderProgram, ("objects[" + std::to_string(i) + "].mass").c_str());
			objectLocations[i].shininess = glGetUniformLocation(shaderProgram, ("objects[" + std::to_string(i) + "].shiniess").c_str());
			objectLocations[i].luminance = glGetUniformLocation(shaderProgram, ("objects[" + std::to_string(i) + "].luminance").c_str());
			objectLocations[i].color = glGetUniformLocation(shaderProgram, ("objects[" + std::to_string(i) + "].color").c_str());
			objectLocations[i].shading = glGetUniformLocation(shaderProgram, ("objects[" + std::to_string(i) + "].shading").c_str());
		}

		glfwSetTime(0.0);

	}

	void RaymarchRenderer::render(Scene* scene, Camera* camera)
	{
		// Check if any events have been activiated (key pressed, mouse moved etc.) 
		// and call corresponding response functions
		glfwPollEvents();

		// Render
		// Clear the colorbuffer
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
			
		// Update uniforms with Scene
		
		updateUniforms(scene, camera);

		glUniform1f(timeLocation, float(glfwGetTime()));

		// Draw our first triangle
		glUseProgram(shaderProgram);
		glBindVertexArray(VAO);
	//	glDrawArrays(GL_TRIANGLES, 0, 6);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);
		
		// Swap the screen buffers
		glfwSwapBuffers(window);

	}

	void RaymarchRenderer::updateUniforms(Scene* scene, Camera* camera)
	{
		glUniform2f(rotationLocation, control->xRotation, control->yRotation);
		glUniform3f(camPosLocation, camera->position.x, camera->position.y, camera->position.z);
		int size = scene->children.size();
		glUniform1i(objCountLocation, size);
		int warps = 0;
		for (int i = 0; i < size; i++)
		{
			Object3D *currentObj = scene->children[i];
			if (currentObj->geometry == SPHERE)
			{
				if (warps == 0) glUniform3f(warpALoc, currentObj->position.x, currentObj->position.y, currentObj->position.z);
				if (warps == 1) glUniform3f(warpBLoc, currentObj->position.x, currentObj->position.y, currentObj->position.z);
				warps++;
			}
			shaderObject3D currentLoc = objectLocations[i];
			glUniform3f(currentLoc.position, currentObj->position.x, currentObj->position.y, currentObj->position.z);
			glUniform3f(currentLoc.direction, currentObj->direction.x, currentObj->direction.y, currentObj->direction.z);
			glUniform1f(currentLoc.radius, currentObj->radius);
			glUniform1f(currentLoc.age, currentObj->age);
			glUniform3f(currentLoc.shape, currentObj->shape.x, currentObj->shape.y, currentObj->shape.z);
			glUniform1i(currentLoc.geometry, currentObj->geometry);
			glUniform1f(currentLoc.mass, currentObj->mass);
		//	glUniform1f(currentLoc.shininess, currentObj->material->shininess);
		//	glUniform1f(currentLoc.luminance, currentObj->material->luminance);
			glUniform3f(currentLoc.color, currentObj->color.x, currentObj->color.y, currentObj->color.z);
		//	glUniform1i(currentLoc.shading, currentObj->material->shading);
		}
		
		glUniform1i(warpCountLoc, warps);
	}

	std::string RaymarchRenderer::loadSource(char* filename)
	{
		std::ifstream infile{ filename };
		return{ std::istreambuf_iterator<char>(infile), std::istreambuf_iterator<char>() };
	}

	RaymarchRenderer::~RaymarchRenderer()
	{
		// Properly de-allocate all resources once they've outlived their purpose
		glDeleteVertexArrays(1, &VAO);
		glDeleteBuffers(1, &VBO);
		glDeleteBuffers(1, &EBO);
		delete objectLocations;
		// Terminate GLFW, clearing any resources allocated by GLFW.
		glfwTerminate();
	}

	void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
	{
		if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
			glfwSetWindowShouldClose(window, GL_TRUE);
		control->interpretKey(scancode, action);
		std::printf("keypress: %i  scancode: %i  action: %i  mode: %i\n", key, scancode, action, mode);
	}

	void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
	{
		std::cout << "button: " << button << " action : " << action << "\n";
		control->interpretMouseButton(button, action);
	}

	static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
	{
		//std::cout << "xpos: " << xpos << " ypos: " << ypos << "\n";
		control->interpretMouseMove(xpos, ypos);
	}

	Scene::Scene()
	{
		std::vector<Object3D*> children;
		//control();
	}

	void Scene::add(Object3D *obj)
	{
		children.push_back(obj);
	}

	void Scene::remove(std::string name)
	{
		int index = -1;
		int count = 0;
		for (std::vector<Object3D*>::iterator it = children.begin(); it != children.end(); ++it)
		{
			if (children[count]->name == name)
			{
				index = count;
				break;
			}
			count++;
		}
		if (index < 0)
		{
			std::cout << "Object to remove does not exist\n";
		}
		else
		{
			children.erase(children.begin() + count);
		}
	}

	void Scene::spawn(Camera* camera)
	{
		Sphere *sphere = new rme::Sphere("sphere");
		glm::vec2 yRot = rot2D(glm::vec2(0.0, 1.0), control->yRotation);
		glm::vec2 xRot = rot2D(glm::vec2(0.0, yRot.y), control->xRotation);
		glm::vec3 dir = glm::normalize(glm::vec3(xRot.x, yRot.x, xRot.y));

		sphere->color = glm::vec3(0.0, 1.0, 0.0);
		//sphere->material->color = glm::vec3(0.0, 1.0, 0.0);
		sphere->radius = 2.75;
		sphere->position = camera->position + 1.1f*sphere->radius*dir;
		sphere->charge = 0.0;
		sphere->velocity = dir*0.07f;
		sphere->physics = true;
		this->add(sphere);
	}

	void Scene::update()
	{
		// Collision detection and interaction
		for (int i = 0; i < children.size(); i++)
		{
			Object3D *current = children[i];

			current->age += 1.0;

			if (current->physics) {
				current->velocity += glm::vec3(0.0, -0.0005, 0.0);
			}

			// Camera collides but does not feel force
			if (current->geometry == CAMERA)
			{
				// Move player with WASD
				glm::vec2 direction = rot2D(glm::vec2(0.0, 1.0), control->xRotation);
				glm::vec2 directionPerp = rot2D(glm::vec2(1.0, 0.0), control->xRotation);

				glm::vec3 candidate = current->position + current->velocity;
				float testDist = map(candidate, i);
				glm::vec3 camNorm = normal(candidate, i);
				if (testDist < current->radius)
				{

					if (control->w) current->velocity +=  0.002f*glm::vec3(direction.x, 0.0, direction.y);
					if (control->a) current->velocity += -0.002f*glm::vec3(directionPerp.x, 0.0, directionPerp.y);
					if (control->s) current->velocity += -0.002f*glm::vec3(direction.x, 0.0, direction.y);
					if (control->d) current->velocity +=  0.002f*glm::vec3(directionPerp.x, 0.0, directionPerp.y);

				//	if (glm::dot(camNorm, glm::vec3(0.0, 1.0, 0.0)) > 0.0)
				//	{
						current->velocity = 0.97f*(current->velocity - glm::dot(current->velocity, camNorm)*camNorm);
				//	}
					if (control->space && glm::dot(camNorm, glm::vec3(0.0, 1.0, 0.0)) > 0.7)
					{
						current->velocity += glm::vec3(0.0, 0.06, 0.0);
						current->position += current->velocity;
					}

				}

				if (control->lmb)
				{
					spawn((Camera*)current);
					control->lmb = false;
				}

			}
			
			if (current->geometry != SPHERE) continue;
			
			// Electromagnetic/Gravity like force
			for (int j = 0; j < children.size(); j++)
			{
				Object3D *other = children[j];
				if (other->geometry != SPHERE || i == j) continue;
				glm::vec3 diff = current->position - other->position;
				float radius = glm::length(diff);
				float force = current->charge*other->charge / (radius*radius);
				current->velocity += diff * force;
			}
		
			// Collision detection
			float distance = map(current->position, i);
			float delta = distance - current->radius;
			// Collision
			if (delta < 0.0f && current->collisions)
			{
				glm::vec3 norm = glm::vec3(normal(current->position, i));
				current->velocity = 1.0f*glm::reflect(current->velocity, norm);
			//	current->correction += -1.0f*delta*glm::normalize(current->velocity);
			}
		
		}

		// Update positions
		for (int i = 0; i < children.size(); i++)
		{
			Object3D *current = children[i];
			current->velocity *= 0.99;
			current->position += (current->velocity + current->correction);
			current->correction = glm::vec3(0.0);
		}

	}

	float Scene::map(glm::vec3 p, int exclude)
	{
			float dist = 1000000.0;
			for (int i = 0; i < children.size(); i++) {
				if (i == exclude) continue;
				switch (children[i]->geometry) {
				case 4:
					dist = glm::min(dist, sdSphere(p - children[i]->position, children[i]->radius));
					break;
				case 7:
					dist = glm::min(dist, sdBoxInterior(p - children[i]->position, children[i]->shape));
					break;
				default:
					dist = dist;
				}
			}
			return dist;
	}

	glm::vec3 Scene::normal(glm::vec3 p, int exclude)
	{
		glm::vec3 eps = glm::vec3(0.002, 0.0, 0.0);
		return glm::normalize(glm::vec3(
			map(p + glm::vec3(eps.x, eps.y, eps.y), exclude) - map(p - glm::vec3(eps.x, eps.y, eps.y), exclude),
			map(p + glm::vec3(eps.y, eps.x, eps.y), exclude) - map(p - glm::vec3(eps.y, eps.x, eps.y), exclude),
			map(p + glm::vec3(eps.y, eps.y, eps.x), exclude) - map(p - glm::vec3(eps.y, eps.y, eps.x), exclude)));
	}

	glm::vec2 Scene::rot2D(glm::vec2 p, float angle)
	{
		float s = glm::sin(angle);
		float c = glm::cos(angle);
		return p * glm::mat2(c, s, -s, c);
	}

	///// Signed Distance Geometries ////

	float Scene::sdSphere(glm::vec3 p, float s)
	{
		return glm::length(p) - s;
	}

	float Scene::sdTorus(glm::vec3 p, glm::vec2 t)
	{
		glm::vec2 q = glm::vec2(glm::length(glm::vec2(p.x, p.y)) - t.x, p.z);
		return glm::length(q) - t.y;
	}

	float Scene::sdRoundBox(glm::vec3 p, glm::vec3 b, float r)
	{
		glm::vec3 d = glm::abs(p) - b;
		return glm::min(glm::max(d.x, glm::max(d.y, d.z)), 0.0f) +glm::length(glm::max(d, glm::vec3(0.0))) - r;
	}

	float Scene::sdBoxInterior(glm::vec3 p, glm::vec3 b)
	{
		glm::vec3 d = glm::abs(p) - b;
		return -(glm::min(glm::max(d.x, glm::max(d.y, d.z)), 0.0f) + glm::length(glm::max(d, glm::vec3(0.0))));
	}

	Material::Material()
	{
		color = glm::vec3(1.0, 1.0, 1.0);
		shininess = 0.5;
		luminance = 0.0;
		shading = 0;
		//Texture stuff
	}

	Object3D::Object3D(std::string n)
	{
		name = n;
		position = glm::vec3(0.0);
		correction = glm::vec3(0.0);
		velocity = glm::vec3(0.0);
		direction = glm::vec3(0.0, 0.0, 1.0);
		radius = 2.0;
		shape = glm::vec3(0.0, 0.0, 0.0);
		geometry = CONTAINER;
		mass = 1.0;
		charge = 0.1;
		age = 0.0;
		collisions = true;
		physics = false;
		color = glm::vec3(1.0, 1.0, 1.0);
	//	material = new Material();
	}

	Camera::Camera(std::string n) :Object3D(n)
	{
		geometry = CAMERA;
		rotation = glm::vec4(0.0);
		physics = true;
		radius = 4.0;
	}
	
	Sphere::Sphere(std::string n) :Object3D(n) { geometry = SPHERE; }

	BoxInterior::BoxInterior(std::string n) :Object3D(n) { geometry = BOX_INTERIOR; }

}