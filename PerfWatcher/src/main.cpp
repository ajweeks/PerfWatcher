#include "stdafx.hpp"

#include <cstdio>
#include <windows.h>

#include <glm/glm.hpp>
#include <imgui.h>

int main()
{
	glm::vec3 a(1.0f, 2.0f, 3.0f);
	ImVec2 v(4.0f, 5.0f);

	printf("hello, world! %.1f, %.1f, %.1f, %.1f, %.1f\n", a.x, a.y, a.z, v.x, v.y);

	system("PAUSE");
}