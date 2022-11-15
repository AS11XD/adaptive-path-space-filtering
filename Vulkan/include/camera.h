#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>

class Camera
{
public:
	glm::vec3 position;
	glm::mat4 projection;
	glm::mat4 view;
	float trajectoryIdx = 0.0f;
	bool trajectoryReverse = false;
	float width, height;

	const glm::vec2 halton_sequence[8] = {
		{ 1.0 / 2.0, 1.0 / 3.0 },
		{ 1.0 / 4.0, 2.0 / 3.0 },
		{ 3.0 / 4.0, 1.0 / 9.0 },
		{ 1.0 / 8.0, 4.0 / 9.0 },
		{ 5.0 / 8.0, 7.0 / 9.0 },
		{ 3.0 / 8.0, 2.0 / 9.0 },
		{ 7.0 / 8.0, 5.0 / 9.0 },
		{ 1.0 / 16.0, 8.0 / 9.0 },
	};

	Camera()
	{

	}

	Camera(float fov, float _width, float _height) : width(_width), height(_height), fov(fov), yaw(90.0f), pitch(0.0f), _near(0.1f), _far(1000.0f)
	{
		projection = glm::perspective(glm::radians(fov), width / height, _near, _far);
		position = glm::vec3(0.0f, 0.5f, -3.0f);
		up = glm::vec3(0.0f, -1.0f, 0.0f);
		onMouseMove(0.0f, 0.0f);
		update();
	}

	glm::mat4 getVP(bool jitter, uint32_t frameCount)
	{
		if (!jitter)
			return vp;

		glm::vec2 jt = (halton_sequence[frameCount % 8] - glm::vec2(0.5f)) * (glm::vec2(2.0f) / glm::vec2(width, height));

		glm::mat4 jitterMat = glm::mat4(
			1.0f, 0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.0f, 0.0f,
			jt.x, jt.y, 0.0f, 1.0f);

		return jitterMat * vp;
	}

	glm::mat4 getV()
	{
		return view;
	}

	glm::mat4 getP()
	{
		return projection;
	}

	void translate(glm::vec3 v)
	{
		position += v;
		update();
	}

	void setLookAt(glm::vec3 l)
	{
		look_at = l;
		update();
	}

	glm::vec3 getSide()
	{
		return glm::normalize(glm::cross(look_at, up));
	}

	void setPosition(glm::vec3 p)
	{
		position = p;
		update();
	}

	void onMouseMove(float xRel, float yRel)
	{
		yaw += xRel * mouse_sensitivity;
		pitch += yRel * mouse_sensitivity;
		if (pitch > 89.0f)
		{
			pitch = 89.0f;
		}
		if (pitch < -89.0f)
		{
			pitch = -89.0f;
		}
		glm::vec3 front;
		front.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
		front.y = sin(glm::radians(pitch));
		front.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
		look_at = glm::normalize(front);
		update();
	}

	void moveFront(float amount)
	{
		translate(glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f) * look_at) * amount);
		update();
	}

	void moveSideways(float amount)
	{
		translate(glm::normalize(glm::cross(look_at, up)) * amount);
		update();
	}

	void moveDown(float amount)
	{
		translate(up * amount);
	}

	void updateScreenSize(float _width, float _height)
	{
		if (_width != 0 && _height != 0)
		{
			width = _width;
			height = _height;
			projection = glm::perspective(glm::radians(fov), width / height, _near, _far);
			update();
		}
	}

	glm::vec3 getPosition()
	{
		return position;
	}


	glm::vec3 getLookAt()
	{
		return look_at;
	}

	float getNear()
	{
		return _near;
	}

	float getFar()
	{
		return _far;
	}

private:
	glm::mat4 vp;
	float _near;
	float _far;
	float yaw;
	float pitch;
	const float mouse_sensitivity = 0.25f;
	glm::vec3 look_at;
	glm::vec3 up;
	float fov;

	void update()
	{
		view = glm::lookAt(position, position + look_at, up);
		vp = projection * view;
	}
};