#include "stdafx.hpp"

#include <cstdio>
#include <windows.h>

#include "Helpers.hpp"

GLFWwindow* g_MainWindow = nullptr;
glm::vec2i g_WindowSize = glm::vec2i(1280, 720);

void GLFWErrorCallback(i32 error, const char* description);

void Startup()
{
	glm::vec3 a(1.0f, 2.0f, 3.0f);
	ImVec2 v(4.0f, 5.0f);

	glfwSetErrorCallback(GLFWErrorCallback);

	if (glfwInit() == GLFW_FALSE)
	{
		printf("Failed to initialize glfw!\n");
		exit(EXIT_FAILURE);
	}

#if _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif // _DEBUG

	// Don't hide window when losing focus in Windowed Fullscreen
	glfwWindowHint(GLFW_AUTO_ICONIFY, GLFW_FALSE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

	bool bMaximized = false;
	if (bMaximized)
	{
		glfwWindowHint(GLFW_MAXIMIZED, GL_TRUE);
	}

	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	const char* titleString = "PerfWatcher v0.0.1";
	g_MainWindow = glfwCreateWindow(g_WindowSize.x, g_WindowSize.y, titleString, NULL, NULL);
	if (!g_MainWindow)
	{
		printf("Failed to create glfw Window! Exiting...\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	//glfwSetWindowUserPointer(g_MainWindow, this);


	glfwMakeContextCurrent(g_MainWindow);

	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);


	printf("hello, world! %.1f, %.1f, %.1f, %.1f, %.1f\n", a.x, a.y, a.z, v.x, v.y);
}

void ShutDown()
{
	if (g_MainWindow)
	{
		glfwDestroyWindow(g_MainWindow);
		g_MainWindow = nullptr;
	}

	printf("goodbye, world\n");
}

int main()
{
	Startup();

	glClearColor(1, 0, 1, 1);

	GLuint vertShaderID, fragShaderID;
	GLuint program = glCreateProgram();
	LoadGLShaders(program, RESOURCE_LOCATION "shaders/vert.v", RESOURCE_LOCATION "shaders/frag.f", vertShaderID, fragShaderID);
	LinkProgram(program);

	glUseProgram(program);

	std::vector<glm::vec3> positions = {
		glm::vec3(-1.0f,  -1.0f, 0.0f),
		glm::vec3(-1.0f, 3.0f, 1.0f),
		glm::vec3(3.0f,  -1.0f, 0.0f)
	};

	std::vector<glm::vec4> colours = {
		glm::vec4(0.0f, 0.0f, 1.0f, 1.0f),
		glm::vec4(0.0f, 1.0f, 1.0f, 1.0f),
		glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)
	};

	i32 vertexCount = (i32)positions.size();
	i32 vertexStride = sizeof(glm::vec3) + sizeof(glm::vec4);
	i32 vertexBufferSize = vertexCount * vertexStride;
	void* vertexBuffer = malloc(vertexBufferSize);
	if (!vertexBuffer)
	{
		printf("Failed to allocate memory for vertex buffer!\n");
	}

	float* currentBufferPos = (float*)vertexBuffer;
	for (i32 i = 0; i < vertexCount; ++i)
	{
		memcpy(currentBufferPos, &positions[i].x, sizeof(glm::vec3));
		currentBufferPos += 3;

		memcpy(currentBufferPos, &colours[i].x, sizeof(glm::vec4));
		currentBufferPos += 4;
	}

	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	GLuint VBO;
	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, vertexBuffer, GL_STATIC_DRAW);


	glBindVertexArray(VBO);

	float* currentLocation = (float*)0;
	GLint posLoc = glGetAttribLocation(program, "in_Position");
	GLint colLoc = glGetAttribLocation(program, "in_Colour");
	glEnableVertexAttribArray((GLuint)posLoc);
	glEnableVertexAttribArray((GLuint)colLoc);
	glVertexAttribPointer((GLuint)posLoc, 3, GL_FLOAT, GL_FALSE, vertexStride, currentLocation); currentLocation += 3;
	glVertexAttribPointer((GLuint)colLoc, 4, GL_FLOAT, GL_FALSE, vertexStride, currentLocation); currentLocation += 4;

	glEnable(GL_DEPTH_TEST);

	GLint viewProjLocation = glGetUniformLocation(program, "viewProj");
	GLint modelLocation = glGetUniformLocation(program, "model");
	while (!glfwWindowShouldClose(g_MainWindow))
	{
		glfwPollEvents();

		glBindVertexArray(VBO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);

		glViewport(0, 0, g_WindowSize.x, g_WindowSize.y);

		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LEQUAL);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glCullFace(GL_NONE);

		glm::mat4 viewProj = glm::mat4(1.0f);
		glm::mat4 model = glm::mat4(1.0f);

		glUniformMatrix4fv(viewProjLocation, 1, GL_FALSE, &viewProj[0][0]);
		glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);

		glDrawArrays(GL_TRIANGLES, 0, vertexCount);

		glfwSwapBuffers(g_MainWindow);
	}


	ShutDown();

	system("PAUSE");
}

void GLFWErrorCallback(i32 error, const char* description)
{
	printf("GLFW Error: %i: %s\n", error, description);
}
