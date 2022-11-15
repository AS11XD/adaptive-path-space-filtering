#pragma once

#include <glm/glm.hpp>
#include <string>

enum LIGHT_SOURCE
{
	L_DIRECTIONAL, L_POINT
};

struct Light
{
	glm::vec3 position;
	glm::vec3 color;
	glm::vec3 direction;
	float intensity;
	uint32_t type;
	uint32_t on;
};

struct LightObj
{
	std::string name;
	Light light;
};

class Sun
{
public:
	static glm::vec3 initialPosition;
	static glm::vec3 position;
	static glm::vec3 color;
	static glm::vec3 direction;
	static float intensity;
};

class Moon
{
public:
	static glm::vec3 initialPosition;
	static glm::vec3 position;
	static glm::vec3 color;
	static glm::vec3 direction;
	static float intensity;
};

class Torch
{
public:
	static glm::vec3 color;
	static float intensity;
};