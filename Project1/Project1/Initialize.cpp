#include "Initialize.h"

float rando(){
	return static_cast <float> (rand()) / (static_cast <float> (RAND_MAX / 0.01));
}



int main()
{
	srand(0);

	rme::Scene *scene = new rme::Scene();

	rme::Camera *camera = new rme::Camera(std::string("camera1"));
	camera->position = glm::vec3(0.0, 2.0, -3.0);
	camera->physics = true;
	scene->add(camera);
	/*
	const int maxSpheres = 10;
	int sCount = 0;
	rme::Sphere *spheres[maxSpheres];
	
	for (int i = 0; i < 6; i++)
	{
		spheres[i] = new rme::Sphere("sphere" + std::to_string(i));
		spheres[i]->position = glm::vec3(float(i%3)*4.0-2.0, 0.0, 1.0+float(i)/1.0);
		spheres[i]->material.color = glm::vec3(0.0, 1.0, 0.0);
		spheres[i]->radius = 0.5;
		spheres[i]->charge = 0.15;
		spheres[i]->velocity = glm::vec3(rando(), rando(), rando());
		spheres[i]->physics = true;
		scene->add(spheres[i]);
	}
	*/
	rme::BoxInterior *room = new rme::BoxInterior(std::string("room"));
	room->shape = glm::vec3(30.0, 16.0, 36.0);
	scene->add(room);

	rme::RaymarchRenderer *renderer = new rme::RaymarchRenderer(1200, 720);
	
	int totalFrames = 0;
	int lastFrame = 0;
	float totalTime = 0.0;
	float lastTime = 0.0;
	//glfwSetTime(0.0);

	// Game loop
	
	while (!glfwWindowShouldClose(renderer->window))
	{
		
	//	s1->position.z += 0.002;

		for (int i = 0; i < 5; i++)
		{
			scene->update();
		}

		renderer->render(scene, camera);

		totalFrames++;
		totalTime = float(glfwGetTime());
		float delta = totalTime - lastTime;
		if (delta > 1.0)
		{
			std::printf("FPS: %f\n", float(totalFrames - lastFrame)/delta);
			lastFrame = totalFrames;
			lastTime = totalTime;
		}

	}

	delete renderer;
	return 0;
}
