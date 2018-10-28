#pragma once


struct OrbitCam
{
	OrbitCam();

	void Tick();
	glm::mat4 GetView();
	glm::mat4 GetProj();
	glm::mat4 GetViewProj();
	void Orbit(float horizontal, float vertical);
	void Pan(const glm::vec3& o);
	void Zoom(float amount);
	void CalculateBasis();
	void SetPerspective(bool bNewPersective);
	glm::vec3 GetPos();

	float FOV;
	float aspectRatio;
	float nearPlane;
	float farPlane;
	float distFromCenter = 200.0f;

	bool bPerspective;

	glm::vec3 center;
	glm::vec3 offset;
	glm::vec2 offsetVel;

	glm::vec3 right;
	glm::vec3 up;
	glm::vec3 forward;

	float orbitSpeed = 10.0f;
	float zoomSpeed = 4.0f;
	float panSpeed = 0.1f;

	float minDist = 1.0f;
	float maxDist = 500.0f;
};
