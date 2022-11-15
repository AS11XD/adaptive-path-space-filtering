#pragma once

#include <vector>
#include <glm/glm.hpp>
#include "camera.h"

struct TrajectoryPoint
{
	glm::vec3 position;
	glm::vec3 lookAt;
};

struct CameraTrajectory
{
	std::vector<TrajectoryPoint> points;
};

glm::vec3 interpolateLinear(glm::vec3& v0, glm::vec3& v1, float t);
glm::vec3 interpolateCosine(glm::vec3& v0, glm::vec3& v1, float t);
glm::vec3 interpolateCubic(glm::vec3& v0, glm::vec3& v1, glm::vec3& v2, glm::vec3& v3, float t);
glm::vec3 interpolateHermite(glm::vec3& v0, glm::vec3& v1, glm::vec3& v2, glm::vec3& v3, float t, float tension, float bias);
TrajectoryPoint interpolateLinear(TrajectoryPoint& p1, TrajectoryPoint& p2, float t);
TrajectoryPoint interpolateCosine(TrajectoryPoint& p1, TrajectoryPoint& p2, float t);
TrajectoryPoint interpolateCubic(TrajectoryPoint& p1, TrajectoryPoint& p2, TrajectoryPoint& p3, TrajectoryPoint& p4, float t);
TrajectoryPoint interpolateHermite(TrajectoryPoint& p1, TrajectoryPoint& p2, TrajectoryPoint& p3, TrajectoryPoint& p4, float t, float tension, float bias);

void moveAlongTrajectory(Camera& camera, CameraTrajectory& cameraTrajectory, int trajClamp, int interpolationStrategy, float dt, float speed, float tension, float bias);