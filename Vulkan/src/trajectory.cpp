#include "trajectory.h"
#include "guiControl.h"

glm::vec3 interpolateLinear(glm::vec3& v0, glm::vec3& v1, float t)
{
	float t2 = (1.0f - cosf(t * M_PI)) / 2.0f;
	return (1.0f - t2) * v0 + t2 * v1;
}

glm::vec3 interpolateCosine(glm::vec3& v0, glm::vec3& v1, float t)
{
	return (1.0f - t) * v0 + t * v1;
}

glm::vec3 interpolateCubic(glm::vec3& v0, glm::vec3& v1, glm::vec3& v2, glm::vec3& v3, float t)
{
	float t2 = t * t;
	glm::vec3 a0 = v3 - v2 - v0 + v1;
	glm::vec3 a1 = v0 - v1 - a0;
	glm::vec3 a2 = v2 - v0;
	glm::vec3 a3 = v1;
	return a0 * t * t2 + a1 * t2 + a2 * t + a3;
}

glm::vec3 interpolateHermite(glm::vec3& v0, glm::vec3& v1, glm::vec3& v2, glm::vec3& v3, float t, float tension, float bias)
{
	glm::vec3 m0, m1;
	float a0, a1, a2, a3;

	float t2 = t * t;
	float t3 = t2 * t;

	m0 = (v1 - v0) * (1.0f + bias) * (1.0f - tension) / 2.0f;
	m0 += (v2 - v1) * (1.0f - bias) * (1.0f - tension) / 2.0f;
	m1 = (v2 - v1) * (1.0f + bias) * (1.0f - tension) / 2.0f;
	m1 += (v3 - v2) * (1.0f - bias) * (1.0f - tension) / 2.0f;
	a0 = 2.0f * t3 - 3.0f * t2 + 1.0f;
	a1 = t3 - 2.0f * t2 + t;
	a2 = t3 - t2;
	a3 = -2.0f * t3 + 3.0f * t2;

	return(a0 * v1 + a1 * m0 + a2 * m1 + a3 * v2);
}

TrajectoryPoint interpolateLinear(TrajectoryPoint& p1, TrajectoryPoint& p2, float t)
{
	glm::vec3 lookAt1 = p1.lookAt;
	glm::vec3 position1 = p1.position;
	glm::vec3 lookAt2 = p2.lookAt;
	glm::vec3 position2 = p2.position;
	TrajectoryPoint ntp;
	ntp.position = interpolateLinear(position1, position2, t);
	ntp.lookAt = interpolateLinear(lookAt1, lookAt2, t);
	return ntp;
}


TrajectoryPoint interpolateCosine(TrajectoryPoint& p1, TrajectoryPoint& p2, float t)
{
	glm::vec3 lookAt1 = p1.lookAt;
	glm::vec3 position1 = p1.position;
	glm::vec3 lookAt2 = p2.lookAt;
	glm::vec3 position2 = p2.position;
	TrajectoryPoint ntp;
	float t2 = (1.0f - cosf(t * M_PI)) / 2.0f;
	ntp.position = interpolateCosine(position1, position2, t);
	ntp.lookAt = interpolateCosine(lookAt1, lookAt2, t);
	return ntp;
}

TrajectoryPoint interpolateCubic(TrajectoryPoint& p1, TrajectoryPoint& p2, TrajectoryPoint& p3, TrajectoryPoint& p4, float t)
{
	glm::vec3 lookAt1 = p1.lookAt;
	glm::vec3 position1 = p1.position;
	glm::vec3 lookAt2 = p2.lookAt;
	glm::vec3 position2 = p2.position;
	glm::vec3 lookAt3 = p3.lookAt;
	glm::vec3 position3 = p3.position;
	glm::vec3 lookAt4 = p4.lookAt;
	glm::vec3 position4 = p4.position;
	TrajectoryPoint ntp;
	ntp.position = interpolateCubic(position1, position2, position3, position4, t);
	ntp.lookAt = interpolateCubic(lookAt1, lookAt2, lookAt3, lookAt4, t);
	return ntp;
}

TrajectoryPoint interpolateHermite(TrajectoryPoint& p1, TrajectoryPoint& p2, TrajectoryPoint& p3, TrajectoryPoint& p4, float t, float tension, float bias)
{
	glm::vec3 lookAt1 = p1.lookAt;
	glm::vec3 position1 = p1.position;
	glm::vec3 lookAt2 = p2.lookAt;
	glm::vec3 position2 = p2.position;
	glm::vec3 lookAt3 = p3.lookAt;
	glm::vec3 position3 = p3.position;
	glm::vec3 lookAt4 = p4.lookAt;
	glm::vec3 position4 = p4.position;
	TrajectoryPoint ntp;

	ntp.position = interpolateHermite(position1, position2, position3, position4, t, tension, bias);
	ntp.lookAt = interpolateHermite(lookAt1, lookAt2, lookAt3, lookAt4, t, tension, bias);
	return ntp;
}

void moveAlongTrajectory(Camera& camera, CameraTrajectory& cameraTrajectory, int trajClamp, int interpolationStrategy, float dt, float speed, float tension, float bias)
{
	uint32_t idx1 = (uint32_t)std::floorf(camera.trajectoryIdx);
	uint32_t idx0 = idx1 - 1;
	uint32_t idx2 = idx1 + 1;
	uint32_t idx3 = idx2 + 1;

	switch (trajClamp)
	{
		case TRAJ_CLAMP::TRAJ_REVERT:
			idx0 = std::clamp(idx0, (uint32_t)0, (uint32_t)cameraTrajectory.points.size() - 1);
			idx3 = std::clamp(idx3, (uint32_t)0, (uint32_t)cameraTrajectory.points.size() - 1);
			break;

		case TRAJ_CLAMP::TRAJ_LOOP:
			idx0 %= cameraTrajectory.points.size();
			idx1 %= cameraTrajectory.points.size();
			idx2 %= cameraTrajectory.points.size();
			idx3 %= cameraTrajectory.points.size();
			break;

		default:
			break;
	}

	TrajectoryPoint& tp0 = cameraTrajectory.points[idx0];
	TrajectoryPoint& tp1 = cameraTrajectory.points[idx1];
	TrajectoryPoint& tp2 = cameraTrajectory.points[idx2];
	TrajectoryPoint& tp3 = cameraTrajectory.points[idx3];

	float t = camera.trajectoryIdx - idx1;
	TrajectoryPoint ntp;

	switch (interpolationStrategy)
	{
		case TRAJ_INTERPOLATION::TRAJ_LINEAR:
			ntp = interpolateLinear(tp1, tp2, t);
			break;
		case TRAJ_INTERPOLATION::TRAJ_COSINE:
			ntp = interpolateCosine(tp1, tp2, t);
			break;
		case TRAJ_INTERPOLATION::TRAJ_CUBIC:
			ntp = interpolateCubic(tp0, tp1, tp2, tp3, t);
			break;
		case TRAJ_INTERPOLATION::TRAJ_HERMITE:
			ntp = interpolateHermite(tp0, tp1, tp2, tp3, t, tension, bias);
			break;
		default:
			break;
	}

	camera.setPosition(ntp.position);
	camera.setLookAt(ntp.lookAt);

	float ntidx = 0.0f;

	switch (trajClamp)
	{
		case TRAJ_CLAMP::TRAJ_REVERT:
			ntidx = camera.trajectoryIdx;
			ntidx += (camera.trajectoryReverse ? -1.0f : 1.0f) * speed * dt;
			if (ntidx >= (float)(cameraTrajectory.points.size() - 1) || ntidx < 0.0f)
			{
				camera.trajectoryReverse = !camera.trajectoryReverse;
				camera.trajectoryIdx += (camera.trajectoryReverse ? -1.0f : 1.0f) * speed * dt;
			}
			else
			{
				camera.trajectoryIdx = ntidx;
			}
			break;

		case TRAJ_CLAMP::TRAJ_LOOP:
			camera.trajectoryIdx += speed * dt;
			if (camera.trajectoryIdx >= (float)cameraTrajectory.points.size())
			{
				camera.trajectoryIdx -= (float)cameraTrajectory.points.size();
			}
			break;

		default:
			break;
	}
}
