#include "light.h"

glm::vec3 Sun::initialPosition = glm::vec3(0.0f, 480.0f, 0.0f);
glm::vec3 Sun::position = Sun::initialPosition;
glm::vec3 Sun::color = glm::normalize(glm::vec3(0.9f, 0.9f, 0.7f));
glm::vec3 Sun::direction = glm::normalize(Sun::position);
float Sun::intensity = 8.0f;

glm::vec3 Moon::initialPosition = glm::vec3(0.0f, -480.0f, 0.0f);
glm::vec3 Moon::position = Moon::initialPosition;
glm::vec3 Moon::color = glm::normalize(glm::vec3(0.6f, 0.7f, 0.9f));
glm::vec3 Moon::direction = glm::normalize(Moon::position);
float Moon::intensity = 6.0f;

glm::vec3 Torch::color = glm::normalize(glm::vec3(0.9f, 0.9f, 0.7f));
float Torch::intensity = 8.0f;