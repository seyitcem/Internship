#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <string>
#include <vector>
#include <stack>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "graphics/shader.h"
#include "graphics/texture.h"
#include "graphics/model.h"
#include "graphics/light.h"

#include "graphics/models/cube.hpp"
#include "graphics/models/lamp.hpp"
#include "graphics/models/gun.hpp"
#include "graphics/models/sphere.hpp"
#include "graphics/models/box.hpp"

#include "physics/environment.h"

#include "io/keyboard.h"
#include "io/mouse.h"
#include "io/camera.h"

#include "algorithms/states.hpp"

#include "scene.h"

Scene scene;

void processInput(double dt);

Camera cam;

double dt = 0.0f;
double lastFrame = 0.0f;

SphereArray launchObjects;

int main() {
	scene = Scene(3, 3, "OpenGL Tutorial", 1000, 700);
	if (!scene.init()) {
		std::cout << "Could not open window" << std::endl;
		glfwTerminate();
		return -1;
	}

	scene.cameras.push_back(&cam);
	scene.activeCamera = 0;

	// Shaders
	Shader shader("assets/object.vs", "assets/object.fs");
	Shader lampShader("assets/instanced/instanced.vs", "assets/lamp.fs");
	Shader launchShader("assets/instanced/instanced.vs", "assets/object.fs");
	Shader boxShader("assets/instanced/box.vs", "assets/instanced/box.fs");

	// Models
	launchObjects.init();

	Box box;
	box.init();

	Model m(BoundTypes::AABB, glm::vec3(0.0f), glm::vec3(0.05f));
	m.loadModel("assets/models/orc/scene.gltf");

	Model car(BoundTypes::AABB, glm::vec3(2.0f), glm::vec3(0.2f));
	car.loadModel("assets/models/car/scene.gltf");

	// Lights
	DirLight dirLight = { glm::vec3(-0.2f, -1.0f, -0.3f), glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), glm::vec4(0.4f, 0.4f, 0.4f, 1.0f), glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) };
	scene.dirLight = &dirLight;

	glm::vec3 pointLightPositions[] = {
		glm::vec3(0.7f,  0.2f,  2.0f),
		glm::vec3(2.3f, -3.3f, -4.0f),
		glm::vec3(-4.0f,  2.0f, -12.0f),
		glm::vec3(0.0f,  0.0f, -3.0f)
	};

	glm::vec4 ambient = glm::vec4(0.05f, 0.05f, 0.05f, 1.0f);
	glm::vec4 diffuse = glm::vec4(0.8f, 0.8f, 0.8f, 1.0f);
	glm::vec4 specular = glm::vec4(1.0f);
	float k0 = 1.0f;
	float k1 = 0.09f;
	float k2 = 0.032f;

	PointLight pointLights[4];

	LampArray lamps;
	lamps.init();
	for (unsigned int i = 0; i < 4; i++) {
		pointLights[i] = {
			pointLightPositions[i],
			k0, k1, k2,
			ambient, diffuse, specular
		};
		lamps.lightInstances.push_back(pointLights[i]);
		scene.pointLights.push_back(&pointLights[i]);
		States::activate(&scene.activePointLights, i);
	}

	SpotLight spotLight = {
		cam.cameraPos, cam.cameraFront,
		glm::cos(glm::radians(12.5f)), glm::cos(glm::radians(20.0f)),
		1.0f, 0.07f, 0.032f,
		glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f), glm::vec4(1.0f)
	};
	scene.spotLights.push_back(&spotLight);
	scene.activeSpotLights = 1;	// 0b00000001

	while (!scene.shouldClose()) {
		box.positions.clear();
		box.sizes.clear();

		double currentTime = glfwGetTime();
		dt = currentTime - lastFrame;
		lastFrame = currentTime;

		processInput(dt);

		scene.update();

		scene.render(shader);
		m.render(shader, dt, &box);
		car.render(shader, dt, &box);

		std::stack<int> removeObjects;
		for (int i = 0; i < launchObjects.instances.size(); i++) {
			if (glm::length(scene.getActiveCamera()->cameraPos - launchObjects.instances[i].pos) > 50.0f) {
				removeObjects.push(i);
				continue;
			}
		}
		for (int i = 0; i < removeObjects.size(); i++) {
			launchObjects.instances.erase(launchObjects.instances.begin() + removeObjects.top());
			removeObjects.pop();
		}

		if (launchObjects.instances.size() > 0) {
			scene.render(launchShader);
			launchObjects.render(launchShader, dt, &box);
		}

		scene.render(lampShader, false);
		lamps.render(lampShader, dt, &box);


		if (box.positions.size() > 0) {
			scene.render(boxShader, false);
			box.render(boxShader);
		}
		scene.newFrame();
	}

	lamps.cleanup();
	box.cleanup();
	launchObjects.cleanup();
	m.cleanup();

	scene.cleanup();
	return 0;
}

void launchItem(float dt) {
	RigidBody rb(1.0f, scene.getActiveCamera()->cameraPos);
	rb.transferEnergy(100.0f, scene.getActiveCamera()->cameraFront);
	rb.applyAcceleration(Environment::gravitationalAcceleration);
	launchObjects.instances.push_back(rb);
}

void processInput(double dt) {
	scene.processInput(dt);

	if (States::isActive(&scene.activeSpotLights, 0)) {
		scene.spotLights[0]->position = scene.getActiveCamera()->cameraPos;
		scene.spotLights[0]->direction = scene.getActiveCamera()->cameraFront;
	}

	if (Keyboard::key(GLFW_KEY_ESCAPE)) {
		scene.setShouldClose(true);
	}

	if (Keyboard::keyWentDown(GLFW_KEY_L)) {
		States::toggle(&scene.activeSpotLights, 0);
	}

	if (Keyboard::keyWentDown(GLFW_KEY_F)) {
		launchItem(dt);
	}

	for (int i = 0; i < 4; i++) {
		if (Keyboard::keyWentDown(GLFW_KEY_1 + i)) {
			States::toggle(&scene.activePointLights, i);
		}
	}
}