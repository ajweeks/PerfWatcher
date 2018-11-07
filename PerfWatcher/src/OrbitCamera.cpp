#include "stdafx.hpp"

#include "OrbitCamera.hpp"

#pragma warning(push, 0)
#include <glm/vec3.hpp>
#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)

#include "Helpers.hpp"

extern glm::vec2i g_WindowSize;
extern float g_DT;
extern float g_xScale;
extern float g_yScale;
extern float g_zScale;

OrbitCam::OrbitCam()
{
	Reset();
}

void OrbitCam::Tick()
{
	offsetVel *= (1.0f - glm::clamp(g_DT * 20.0f, 0.0f, 0.99f));
	offset += right * offsetVel.x + up * offsetVel.y;

	CalculateBasis();
}

glm::mat4 OrbitCam::GetView()
{
	glm::mat4 view = glm::lookAt(center + offset, center, VEC_UP);

	return view;
}

glm::mat4 OrbitCam::GetProj()
{
	aspectRatio = g_WindowSize.x / (float)g_WindowSize.y;

	float orthoSize = glm::length(offset);
	glm::mat4 projection = bPerspective ?
		glm::perspective(FOV, aspectRatio, nearPlane, farPlane) :
		glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);

	return projection;
}

glm::mat4 OrbitCam::GetViewProj()
{
	aspectRatio = g_WindowSize.x / (float)g_WindowSize.y;

	float orthoSize = glm::length(offset);
	glm::mat4 projection = bPerspective ?
		glm::perspective(FOV, aspectRatio, nearPlane, farPlane) :
		glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
	glm::mat4 view = glm::lookAt(center + offset, center, VEC_UP);
	glm::mat4 viewProj = projection * view;

	return viewProj;
}

void OrbitCam::Orbit(float horizontal, float vertical)
{
	offset += right * horizontal + up * vertical;
	offset = glm::normalize(offset) * distFromCenter;
	glm::vec2 offsetXY(glm::dot(offset, right), glm::dot(offset, up));
	offsetVel += offsetXY;

	CalculateBasis();
}

void OrbitCam::Pan(const glm::vec3& o)
{
	// TODO: Implement!
	center += o * panSpeed;
	////offset += o * panSpeed;

	CalculateBasis();
}

void OrbitCam::Zoom(float amount)
{
	distFromCenter += amount * zoomSpeed;
	distFromCenter = glm::clamp(distFromCenter, minDist, maxDist);
	offset = glm::normalize(offset) * distFromCenter;
}

void OrbitCam::CalculateBasis()
{
	glm::mat4 view = glm::lookAt(offset + center, center, VEC_UP);
	view = glm::transpose(view);
	right = view[0];
	up = view[1];
	forward = view[2];
}

void OrbitCam::SetPerspective(bool bNewPersective)
{
	bPerspective = bNewPersective;
}

glm::vec3 OrbitCam::GetPos()
{
	return center + offset;
}

void OrbitCam::Reset()
{
	FOV = glm::radians(80.0f);
	aspectRatio = (float)g_WindowSize.x / (float)g_WindowSize.y;
	nearPlane = 0.1f;
	farPlane = 1000.0f;

	center = glm::vec3(0.5f, 0.0f, 0.5f);
	offset = glm::vec3(100.0f, 100.0f, 100.0f);
	offset = glm::normalize(offset) * distFromCenter;

	offsetVel = glm::vec2(0.0f);

	bPerspective = true;

	CalculateBasis();
}

void OrbitCam::ParseUserConfigFile(const std::vector<std::string>& fileContents)
{
	i32 row = 0;
	while (row < fileContents.size())
	{
		if (fileContents[row].compare("#camera-persp:") == 0)
		{
			bPerspective = (ParseInt(fileContents[row + 1]) == 1);
		}

		row++;
	}
}
