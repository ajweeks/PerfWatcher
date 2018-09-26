#include "stdafx.hpp"

#include <cstdio>
#include <windows.h>

#include "imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include "Helpers.hpp"

GLFWwindow* g_MainWindow = nullptr;
glm::vec2i g_WindowSize = glm::vec2i(1280, 720);

GLuint g_MainProgram;
GLuint g_FullScreenTriVAO;
GLuint g_FullScreenTriVBO;

void GLFWErrorCallback(i32 error, const char* description);
void GLFWWindowSizeCallback(GLFWwindow* window, i32 width, i32 height);

void CreateWindowAndContext();
void ShutDown();

bool ParseCSV(const char* filePath, std::vector<std::string>& outHeaders, std::vector<std::vector<float>>& outDataRows);

void GenerateFullScreenTri();
void DescribeShaderVertexAttributes();

void DrawFullScreenQuad();

int main()
{
	CreateWindowAndContext();

	std::vector<std::string> headers;
	std::vector<std::vector<float>> dataRows;
	const char* csvFileFilePath = RESOURCE_LOCATION "input/input.csv";
	if (!ParseCSV(csvFileFilePath, headers, dataRows))
	{
		printf("No loaded data!\n");
	}

	GLuint vertShaderID, fragShaderID;
	g_MainProgram = glCreateProgram();
	LoadGLShaders(g_MainProgram, RESOURCE_LOCATION "shaders/vert.v", RESOURCE_LOCATION "shaders/frag.f", vertShaderID, fragShaderID);
	LinkProgram(g_MainProgram);

	glUseProgram(g_MainProgram);

	GenerateFullScreenTri();

	glEnable(GL_DEPTH_TEST);

	GLint viewProjLocation = glGetUniformLocation(g_MainProgram, "viewProj");
	GLint modelLocation = glGetUniformLocation(g_MainProgram, "model");
	float timePrev = (float)glfwGetTime();
	while (!glfwWindowShouldClose(g_MainWindow))
	{
		float timeNow = (float)glfwGetTime();
		float dt = timeNow - timePrev;
		timePrev = timeNow;

		glfwPollEvents();

		// Start the ImGui frame
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		{
			static float f = 0.0f;
			static int counter = 0;
			ImGui::Text("Hello, world!");                           // Display some text (you can use a format string too)
			ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f
			//ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

			//ImGui::Checkbox("Demo Window", &show_demo_window);      // Edit bools storing our windows open/close state
			//ImGui::Checkbox("Another Window", &show_another_window);

			if (ImGui::Button("Button"))                            // Buttons return true when clicked (NB: most widgets return true when edited/activated)
				counter++;
			ImGui::SameLine();
			ImGui::Text("counter = %d", counter);

			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		}

		DrawFullScreenQuad();

		glm::mat4 viewProj = glm::mat4(1.0f);
		glm::mat4 model = glm::mat4(1.0f);



		float FPS = 1.0f / dt;
		std::string windowTitle = "PerfWatcher v0.0.1 - " + FloatToString(dt, 2) + "ms / " + FloatToString(FPS, 0) + " fps";
		glfwSetWindowTitle(g_MainWindow, windowTitle.c_str());


		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


		glfwSwapBuffers(g_MainWindow);
	}


	ShutDown();

	system("PAUSE");
}

void GLFWWindowSizeCallback(GLFWwindow* window, i32 width, i32 height)
{
	UNREFERENCED_PARAMETER(window);

	g_WindowSize = glm::vec2i(width, height);
	glViewport(0, 0, width, height);
}

void GLFWErrorCallback(i32 error, const char* description)
{
	printf("GLFW Error: %i: %s\n", error, description);
}

void CreateWindowAndContext()
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
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

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
	glfwSetWindowSizeCallback(g_MainWindow, GLFWWindowSizeCallback);

	glfwMakeContextCurrent(g_MainWindow);

	bool bEnableVSync = true;
	glfwSwapInterval(bEnableVSync ? 1 : 0);

	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

	ImGui_ImplGlfw_InitForOpenGL(g_MainWindow, true);
	ImGui_ImplOpenGL3_Init();

	// Setup style
	ImGui::StyleColorsDark();

	glClearColor(1, 0, 1, 1);
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

bool ParseCSV(const char* filePath, std::vector<std::string>& outHeaders, std::vector<std::vector<float>>& outDataRows)
{
	std::string csvFileContents;
	if (!ReadFile(filePath, csvFileContents, false))
	{
		printf("Failed to read csv file: %s!\n", filePath);
		return false;
	}

	std::vector<std::string> fileRows = Split(csvFileContents, '\n');

	outHeaders = Split(fileRows[0], ',');
	for (std::string& header : outHeaders)
	{
		TrimWhitespace(header);
	}

	i32 columnCount = (i32)outHeaders.size();

	for (i32 i = 1; i < (i32)fileRows.size(); ++i)
	{
		std::vector<float> row;
		row.reserve(columnCount);

		std::string rowStr = fileRows[i];
		std::vector<std::string> rowEntries = Split(rowStr, ',');
		if ((i32)rowEntries.size() != columnCount)
		{
			printf("Invalidly formatted csv file, different number of data columns than header columns!\n");
			rowEntries.resize(columnCount);
		}

		for (i32 j = 0; j < columnCount; ++j)
		{
			row.push_back(ParseFloat(rowEntries[j]));
		}

		outDataRows.push_back(row);
	}

	printf("Finished parsing csv file: %d rows, %d cols\n", outDataRows.size(), columnCount);

	return true;
}

void GenerateFullScreenTri()
{
	std::vector<glm::vec3> positions = {
		glm::vec3(-1.0f,  -1.0f, 0.0f),
		glm::vec3(-1.0f, 3.0f, 1.0f),
		glm::vec3(3.0f,  -1.0f, 0.0f)
	};

	std::vector<glm::vec4> colours = {
		glm::vec4(0.2f, 0.2f, 0.8f, 1.0f),
		glm::vec4(0.2f, 0.8f, 0.8f, 1.0f),
		glm::vec4(0.8f, 0.2f, 0.2f, 1.0f)
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

	glGenVertexArrays(1, &g_FullScreenTriVAO);
	glBindVertexArray(g_FullScreenTriVAO);

	glGenBuffers(1, &g_FullScreenTriVBO);
	glBindBuffer(GL_ARRAY_BUFFER, g_FullScreenTriVBO);
	glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, vertexBuffer, GL_STATIC_DRAW);


	glBindVertexArray(g_FullScreenTriVBO);

	DescribeShaderVertexAttributes();
}

void DescribeShaderVertexAttributes()
{
	i32 vertexStride = sizeof(glm::vec3) + sizeof(glm::vec4);

	float* currentLocation = (float*)0;
	GLint posLoc = glGetAttribLocation(g_MainProgram, "in_Position");
	GLint colLoc = glGetAttribLocation(g_MainProgram, "in_Colour");
	glEnableVertexAttribArray((GLuint)posLoc);
	glEnableVertexAttribArray((GLuint)colLoc);
	glVertexAttribPointer((GLuint)posLoc, 3, GL_FLOAT, GL_FALSE, vertexStride, currentLocation); currentLocation += 3;
	glVertexAttribPointer((GLuint)colLoc, 4, GL_FLOAT, GL_FALSE, vertexStride, currentLocation); currentLocation += 4;
}

void DrawFullScreenQuad()
{
	glUseProgram(g_MainProgram);

	glBindVertexArray(g_FullScreenTriVAO);
	glBindBuffer(GL_ARRAY_BUFFER, g_FullScreenTriVBO);

	glViewport(0, 0, g_WindowSize.x, g_WindowSize.y);

	glDepthMask(GL_TRUE);
	glDepthFunc(GL_LEQUAL);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_NONE);

	glm::mat4 identity = glm::mat4(1.0f);

	GLint viewProjLocation = glGetUniformLocation(g_MainProgram, "viewProj");
	GLint modelLocation = glGetUniformLocation(g_MainProgram, "model");
	glUniformMatrix4fv(viewProjLocation, 1, GL_FALSE, &identity[0][0]);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &identity[0][0]);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}
