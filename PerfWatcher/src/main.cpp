#include "stdafx.hpp"

#include <cstdio>
#include <windows.h>

#pragma warning(push, 0)
#include "imgui.h"
#include "imgui/imgui_impl_glfw.h"
#include "imgui/imgui_impl_opengl3.h"

#include <glm/gtc/matrix_transform.hpp>
#pragma warning(pop)

#include "Helpers.hpp"

GLFWwindow* g_MainWindow = nullptr;
glm::vec2i g_WindowSize = glm::vec2i(1280, 720);
float g_DT = 0.0f;
bool g_LMBDown = false;
glm::vec2i g_CursorPos;

float g_DataPointSize = 1.0f;

float g_xScale = 100.0f;
float g_yScale = 100.0f;
EaseValue<float> g_yScaleMult(EaseType::CUBIC_IN_OUT, 0.0f, 1.0f, 2.0f);
float g_zScale = 100.0f;

GLuint g_MainProgram;
float* g_FullScreenTriVertexBuffer;
GLuint g_FullScreenTriVAO;
GLuint g_FullScreenTriVBO;

std::vector<glm::mat4> g_DataPlotModelMats;
std::vector<float*> g_DataPlotVertexBuffers;
std::vector<GLuint> g_DataVAOs;
std::vector<GLuint> g_DataVBOs;

std::vector<float*> g_DataPointVertexBuffers;
std::vector<glm::mat4> g_DataPointModelMats;
std::vector<GLuint> g_DataPointVAOs;
std::vector<GLuint> g_DataPointVBOs;

extern bool g_MouseJustPressed[5];


void GLFWErrorCallback(i32 error, const char* description);
void GLFWWindowSizeCallback(GLFWwindow* window, int width, int height);
void GLFWCursorPosCallback(GLFWwindow* window, double x, double y);
void GLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
const char* GLFWGetClipboardText(void* user_data);
void GLFWSetClipboardText(void* user_data, const char* text);
void GLFWScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
void GLFWKeyCallback(GLFWwindow* window, int key, int, int action, int mods);
void GLFWCharCallback(GLFWwindow* window, unsigned int c);


bool ParseCSV(const char* filePath, std::vector<std::string>& outHeaders, std::vector<std::vector<float>>& outDataRows, glm::vec2& outMinmMaxValues, i32 maxRowCount = -1);

void GenerateFullScreenTri();
void GenerateVertexBufferFromData(const std::vector<float>& data, GLuint& VAO, GLuint& VBO);
void GenerateCubeVertexBuffer(float size, GLuint& VAO, GLuint& VBO);
void DescribeShaderVertexAttributes();

void DrawFullScreenQuad();

struct OrbitCam
{
	OrbitCam()
	{
		FOV = glm::radians(80.0f);
		aspectRatio = g_WindowSize.x / (float)g_WindowSize.y;
		nearPlane = 0.1f;
		farPlane = 1000.0f;

		center = glm::vec3(0.0f);
		offset = glm::vec3(100.0f, 100.0f, 100.0f);
		offset = glm::normalize(offset) * distFromCenter;

		offsetVel = glm::vec2(0.0f, 0.0f);

		CalculateBasis();
	}

	void Tick()
	{
		offsetVel *= (1.0f - glm::clamp(g_DT * 20.0f, 0.0f, 0.99f));
		offset += right * offsetVel.x + up * offsetVel.y;
		//printf("offset vel: %.1f, %.1f\n", offsetVel.x, offsetVel.y);

		CalculateBasis();
	}

	glm::mat4 GetViewProj()
	{
		aspectRatio = g_WindowSize.x / (float)g_WindowSize.y;

		float orthoSize = glm::length(offset);
		glm::mat4 projection = bPerspective ?
			glm::perspective(FOV, aspectRatio, nearPlane, farPlane) :
			glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, nearPlane, farPlane);
		glm::mat4 view = glm::lookAt(offset, center, glm::vec3(0.0f, 1.0f, 0.0f));
		glm::mat4 viewProj = projection * view;

		return viewProj;
	}

	void Orbit(float horizontal, float vertical)
	{
		offset += right * horizontal + up * vertical;
		offset = glm::normalize(offset) * distFromCenter;
		glm::vec2 offsetXY(glm::dot(offset, right), glm::dot(offset, up));
		offsetVel += offsetXY;// Lerp(offsetVel, offsetXY, 0.1f);

		CalculateBasis();
	}

	void Zoom(float amount)
	{
		distFromCenter += amount * zoomSpeed;
		offset = glm::normalize(offset) * distFromCenter;
	}

	void CalculateBasis()
	{
		glm::mat4 view = glm::lookAt(offset, center, glm::vec3(0.0f, 1.0f, 0.0f));
		view = glm::transpose(view);
		right = view[0];
		up = view[1];
		forward = view[2];

		printf("Basis: (%.1f, %.1f, %.1f), (%.1f, %.1f, %.1f), (%.1f, %.1f, %.1f)\n", right.x, right.y, right.z, up.x, up.y, up.z, forward.x, forward.y, forward.z);
	}

	void SetPerspective(bool bNewPersective)
	{
		bPerspective = bNewPersective;
	}

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
};

OrbitCam g_OrbitCam;

class PerfWatcher
{
public:
	void Init()
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

		glfwSetWindowUserPointer(g_MainWindow, this);
		glfwSetWindowSizeCallback(g_MainWindow, GLFWWindowSizeCallback);
		glfwSetCursorPosCallback(g_MainWindow, GLFWCursorPosCallback);
		glfwSetMouseButtonCallback(g_MainWindow, GLFWMouseButtonCallback);
		glfwSetScrollCallback(g_MainWindow, GLFWScrollCallback);
		glfwSetKeyCallback(g_MainWindow, GLFWKeyCallback);
		glfwSetCharCallback(g_MainWindow, GLFWCharCallback);

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

		io.SetClipboardTextFn = GLFWSetClipboardText;
		io.GetClipboardTextFn = GLFWGetClipboardText;

#if _DEBUG
		glEnable(GL_DEBUG_OUTPUT);
#endif

		glClearColor(1, 0, 1, 1);



		g_OrbitCam = OrbitCam();

		std::vector<std::string> headers;
		const char* csvFileFilePath = RESOURCE_LOCATION "input/input-01.csv";
		glm::vec2 minMaxValues;
		if (!ParseCSV(csvFileFilePath, headers, dataRows, minMaxValues, 10))
		{
			printf("No loaded data!\n");
		}

		g_yScale = 100.0f / (minMaxValues.y - minMaxValues.x);

		GLuint vertShaderID, fragShaderID;
		g_MainProgram = glCreateProgram();
		LoadGLShaders(g_MainProgram, RESOURCE_LOCATION "shaders/vert.v", RESOURCE_LOCATION "shaders/frag.f", vertShaderID, fragShaderID);
		LinkProgram(g_MainProgram);

		glUseProgram(g_MainProgram);

		GenerateFullScreenTri();

		i32 plotCount = (i32)dataRows.size();
		for (i32 i = 0; i < plotCount; ++i)
		{
			const std::vector<float>& row = dataRows[i];

			GLuint VAO, VBO;

			GenerateVertexBufferFromData(row, VAO, VBO);

			g_DataVAOs.push_back(VAO);
			g_DataVBOs.push_back(VBO);

			glm::vec3 plotTranslation = glm::vec3(g_xScale * (float)i / (plotCount - 1) - 0.5f, 0.0f, 0.0f);
			glm::mat4 model = glm::translate(glm::mat4(1.0f), plotTranslation);
			g_DataPlotModelMats.push_back(model);

			for (i32 j = 0; j < (i32)row.size(); ++j)
			{
				GLuint pointVAO, pointVBO;
				float percent = (float)j / row.size();

				GenerateCubeVertexBuffer(g_DataPointSize, pointVAO, pointVBO);

				glm::vec3 dataPointTranslation =  glm::vec3(0.0f, row[j], g_zScale * percent);
				glm::mat4 dataPointModel = glm::translate(glm::mat4(1.0f), dataPointTranslation);
				g_DataPointModelMats.push_back(dataPointModel);

				g_DataPointVAOs.push_back(pointVAO);
				g_DataPointVBOs.push_back(pointVBO);
			}
		}


		glEnable(GL_DEPTH_TEST);
	}

	void Loop()
	{
		float timePrev = (float)glfwGetTime();
		while (bRunning && !glfwWindowShouldClose(g_MainWindow))
		{
			float timeNow = (float)glfwGetTime();
			g_DT = timeNow - timePrev;
			timePrev = timeNow;

			glfwPollEvents();

			if (ImGui::IsKeyDown(GLFW_KEY_T))
			{
				g_yScaleMult.Reset();
			}

			g_OrbitCam.Tick();
			g_yScaleMult.Tick();

			printf("%.2f\n", g_yScaleMult.current);

			// Start the ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			DoImGuiItems();

			glDepthMask(GL_TRUE);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			DrawFullScreenQuad();

			glm::mat4 viewProj = g_OrbitCam.GetViewProj();

			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LEQUAL);

			glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, g_yScale * g_yScaleMult.current, 1.0f));

			GLint viewProjLocation = glGetUniformLocation(g_MainProgram, "viewProj");
			GLint modelLocation = glGetUniformLocation(g_MainProgram, "model");

			i32 plotCount = (i32)g_DataVAOs.size();
			for (i32 i = 0; i < plotCount; ++i)
			{
				glBindVertexArray(g_DataVAOs[i]);
				glBindBuffer(GL_ARRAY_BUFFER, g_DataVBOs[i]);

				glUniformMatrix4fv(viewProjLocation, 1, GL_FALSE, &viewProj[0][0]);
				glm::mat4 plotModel = g_DataPlotModelMats[i] * scaleMat;
				glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &plotModel[0][0]);

				glDrawArrays(GL_LINE_STRIP, 0, dataRows[i].size());

				i32 dataElements = (i32)dataRows[i].size();
				for (i32 j = 0; j < dataElements; ++j)
				{
					glBindVertexArray(g_DataPointVAOs[i]);
					glBindBuffer(GL_ARRAY_BUFFER, g_DataPointVBOs[i]);


					glm::vec3 plotTranslation = glm::vec3(
						g_xScale * (float)i / (plotCount - 1) - 0.5f - 0.5f,
						g_yScale * g_yScaleMult.current * dataRows[i][j] - 0.5f,
						g_zScale * (float)j / dataElements - 0.5f);
					glm::mat4 dataPointModel = glm::translate(glm::mat4(1.0f), plotTranslation);
					dataPointModel = glm::rotate(dataPointModel, glm::radians(45.0f), glm::vec3(1.0f, 0.0f, 0.0f));
					dataPointModel = glm::rotate(dataPointModel, glm::radians(45.0f), glm::vec3(0.0f, 1.0f, 0.0f));
					glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &dataPointModel[0][0]);

					glDrawArrays(GL_TRIANGLES, 0, 36);
				}
			}

			float FPS = 1.0f / g_DT;
			std::string windowTitle = "PerfWatcher v0.0.1 - " + FloatToString(g_DT, 2) + "ms / " + FloatToString(FPS, 0) + " fps";
			glfwSetWindowTitle(g_MainWindow, windowTitle.c_str());


			ImGui::Render();
			ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());


			glfwSwapBuffers(g_MainWindow);
		}
	}

	void DoImGuiItems()
	{
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::Selectable("Import"))
				{
					printf("import\n");
				}

				if (ImGui::Selectable("Quit"))
				{
					bRunning = false;
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		static bool bShowCamControlWindow = true;
		if (ImGui::Begin("##camera-control-window", &bShowCamControlWindow, ImVec2(400, 150), -1.0f,
			ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoTitleBar))
		{
			if (ImGui::Button("Orthographic"))
			{
				g_OrbitCam.SetPerspective(false);
			}

			if (ImGui::Button("Perspective"))
			{
				g_OrbitCam.SetPerspective(true);
			}

			ImGui::End();
		}
	}

	void Destroy()
	{
		if (g_MainWindow)
		{
			glfwDestroyWindow(g_MainWindow);
			g_MainWindow = nullptr;
		}

		free(g_FullScreenTriVertexBuffer);

		for (auto buffer : g_DataPointVertexBuffers)
		{
			free(buffer);
		}

		for (auto buffer : g_DataPlotVertexBuffers)
		{
			free(buffer);
		}

		printf("goodbye, world\n");
	}

	void CursorPosCallback(float x, float y)
	{
		glm::vec2i newPos = glm::vec2i((i32)x, (i32)y);
		glm::vec2i dMousePos = newPos - g_CursorPos;
		g_CursorPos = newPos;

		if (g_LMBDown)
		{
			float scale = g_DT * g_OrbitCam.orbitSpeed;
			g_OrbitCam.Orbit(-dMousePos.x * scale, dMousePos.y * scale);
		}
	}

	void ScrollCallback(float xOffset, float yOffset)
	{
		g_OrbitCam.Zoom(-yOffset);
	}

private:
	std::vector<std::vector<float>> dataRows;
	bool bRunning = true;

};

PerfWatcher g_Application;

int main()
{
	g_Application = PerfWatcher();

	g_Application.Init();
	g_Application.Loop();
	g_Application.Destroy();

	system("PAUSE");
}

void GLFWErrorCallback(i32 error, const char* description)
{
	printf("GLFW Error: %i: %s\n", error, description);
}

void GLFWWindowSizeCallback(GLFWwindow* window, int width, int height)
{
	UNREFERENCED_PARAMETER(window);

	g_WindowSize = glm::vec2i(width, height);
	glViewport(0, 0, width, height);
}

void GLFWCursorPosCallback(GLFWwindow* window, double x, double y)
{
	PerfWatcher* application = (PerfWatcher*)glfwGetWindowUserPointer(window);
	application->CursorPosCallback((float)x, (float)y);
}

void GLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	if (action == GLFW_PRESS && button >= 0 && button < ARRAY_LENGTH(g_MouseJustPressed))
	{
		g_MouseJustPressed[button] = true;
	}

	if (action == GLFW_PRESS)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			g_LMBDown = true;
		}
	}
	else
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			g_LMBDown = false;
		}
	}
}

const char* GLFWGetClipboardText(void* user_data)
{
	return glfwGetClipboardString((GLFWwindow*)user_data);
}

void GLFWSetClipboardText(void* user_data, const char* text)
{
	glfwSetClipboardString((GLFWwindow*)user_data, text);
}

void GLFWScrollCallback(GLFWwindow* window, double xOffset, double yOffset)
{
	ImGuiIO& io = ImGui::GetIO();
	io.MouseWheelH += (float)xOffset;
	io.MouseWheel += (float)yOffset;

	PerfWatcher* application = (PerfWatcher*)glfwGetWindowUserPointer(window);
	application->ScrollCallback((float)xOffset, (float)yOffset);
}

void GLFWKeyCallback(GLFWwindow*, int key, int, int action, int mods)
{
	ImGuiIO& io = ImGui::GetIO();
	if (action == GLFW_PRESS)
	{
		io.KeysDown[key] = true;
	}
	if (action == GLFW_RELEASE)
	{
		io.KeysDown[key] = false;
	}

	(void)mods; // Modifiers are not reliable across systems
	io.KeyCtrl = io.KeysDown[GLFW_KEY_LEFT_CONTROL] || io.KeysDown[GLFW_KEY_RIGHT_CONTROL];
	io.KeyShift = io.KeysDown[GLFW_KEY_LEFT_SHIFT] || io.KeysDown[GLFW_KEY_RIGHT_SHIFT];
	io.KeyAlt = io.KeysDown[GLFW_KEY_LEFT_ALT] || io.KeysDown[GLFW_KEY_RIGHT_ALT];
	io.KeySuper = io.KeysDown[GLFW_KEY_LEFT_SUPER] || io.KeysDown[GLFW_KEY_RIGHT_SUPER];
}

void GLFWCharCallback(GLFWwindow*, unsigned int c)
{
	ImGuiIO& io = ImGui::GetIO();
	if (c > 0 && c < 0x10000)
	{
		io.AddInputCharacter((unsigned short)c);
	}
}

bool ParseCSV(const char* filePath, std::vector<std::string>& outHeaders, std::vector<std::vector<float>>& outDataRows, glm::vec2& outMinmMaxValues, i32 maxRowCount /* = -1 */)
{
	float minValue = FLT_MAX;
	float maxValue = FLT_MIN;

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
			float dataPoint = ParseFloat(rowEntries[j]);
			if (dataPoint > maxValue)
			{
				maxValue = dataPoint;
			}
			if (dataPoint < minValue)
			{
				minValue = dataPoint;
			}
			row.push_back(dataPoint);
		}

		bool bRowContainsData = false;
		for (float dataPoint : row)
		{
			if (dataPoint != 0.0f)
			{
				bRowContainsData = true;
				break;
			}
		}

		if (bRowContainsData)
		{
			outDataRows.push_back(row);

			if (maxRowCount != -1 && (i32)outDataRows.size() >= maxRowCount)
			{
				break;
			}
		}
	}
	outMinmMaxValues.x = minValue;
	outMinmMaxValues.y = maxValue;

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
		return;
	}

	g_FullScreenTriVertexBuffer = (float*)vertexBuffer;

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

	DescribeShaderVertexAttributes();
}

void GenerateCubeVertexBuffer(float size, GLuint& VAO, GLuint& VBO)
{
	constexpr i32 vertexCount = 6 * 6; // two tris per face

	std::vector<glm::vec3> positions;
	positions.reserve(vertexCount);

	std::vector<glm::vec4> colours;
	colours.reserve(vertexCount);

	glm::vec4 colour(0.1f, 0.1f, 0.1f, 1.0f);

	// Front
	positions.emplace_back(0.0f, g_DataPointSize, 0.0f);
	positions.emplace_back(0.0f, 0.0f, 0.0f);
	positions.emplace_back(g_DataPointSize, 0.0f, 0.0f);
	positions.emplace_back(0.0f, g_DataPointSize, 0.0f);
	positions.emplace_back(g_DataPointSize, 0.0f, 0.0f);
	positions.emplace_back(g_DataPointSize, g_DataPointSize, 0.0f);

	// Top
	positions.emplace_back(0.0f, g_DataPointSize, g_DataPointSize);
	positions.emplace_back(0.0f, g_DataPointSize, 0.0f);
	positions.emplace_back(g_DataPointSize, g_DataPointSize, 0.0f);
	positions.emplace_back(0.0f, g_DataPointSize, g_DataPointSize);
	positions.emplace_back(g_DataPointSize, g_DataPointSize, 0.0f);
	positions.emplace_back(g_DataPointSize, g_DataPointSize, g_DataPointSize);

	// Back
	positions.emplace_back(0.0f, g_DataPointSize, g_DataPointSize);
	positions.emplace_back(g_DataPointSize, 0.0f, g_DataPointSize);
	positions.emplace_back(0.0f, 0.0f, g_DataPointSize);
	positions.emplace_back(0.0f, g_DataPointSize, g_DataPointSize);
	positions.emplace_back(0.0f, 0.0f, g_DataPointSize);
	positions.emplace_back(0.0f, g_DataPointSize, g_DataPointSize);

	// Bottom
	positions.emplace_back(g_DataPointSize, 0.0f, g_DataPointSize);
	positions.emplace_back(0.0f, 0.0f, g_DataPointSize);
	positions.emplace_back(0.0f, 0.0f, 0.0f);
	positions.emplace_back(0.0f, 0.0f, g_DataPointSize);
	positions.emplace_back(g_DataPointSize, 0.0f, g_DataPointSize);
	positions.emplace_back(g_DataPointSize, 0.0f, 0.0f);

	// Right
	positions.emplace_back(g_DataPointSize, g_DataPointSize, g_DataPointSize);
	positions.emplace_back(g_DataPointSize, g_DataPointSize, 0.0f);
	positions.emplace_back(g_DataPointSize, 0.0f, 0.0f);
	positions.emplace_back(g_DataPointSize, g_DataPointSize, g_DataPointSize);
	positions.emplace_back(g_DataPointSize, 0.0f, 0.0f);
	positions.emplace_back(g_DataPointSize, 0.0f, g_DataPointSize);

	// Left
	positions.emplace_back(0.0f, g_DataPointSize, g_DataPointSize);
	positions.emplace_back(0.0f, 0.0f, 0.0f);
	positions.emplace_back(0.0f, g_DataPointSize, 0.0f);
	positions.emplace_back(0.0f, g_DataPointSize, g_DataPointSize);
	positions.emplace_back(0.0f, 0.0f, g_DataPointSize);
	positions.emplace_back(0.0f, 0.0f, 0.0f);

	assert((i32)positions.size() == vertexCount);

	for (i32 i = 0; i < vertexCount; ++i)
	{
		colours.emplace_back(colour);
	}

	i32 vertexStride = sizeof(glm::vec3) + sizeof(glm::vec4);
	i32 vertexBufferSize = vertexCount * vertexStride;
	void* vertexBuffer = malloc(vertexBufferSize);
	if (!vertexBuffer)
	{
		printf("Failed to allocate memory for vertex buffer!\n");
		return;
	}

	g_DataPointVertexBuffers.push_back((float*)vertexBuffer);

	float* currentBufferPos = (float*)vertexBuffer;
	for (i32 i = 0; i < vertexCount; ++i)
	{
		memcpy(currentBufferPos, &positions[i].x, sizeof(glm::vec3));
		currentBufferPos += 3;

		memcpy(currentBufferPos, &colours[i].x, sizeof(glm::vec4));
		currentBufferPos += 4;
	}

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, vertexBuffer, GL_STATIC_DRAW);

	DescribeShaderVertexAttributes();
}

void GenerateVertexBufferFromData(const std::vector<float>& data, GLuint& VAO, GLuint& VBO)
{
	i32 vertexCount = (i32)data.size();

	std::vector<glm::vec3> positions;
	positions.reserve(vertexCount);

	std::vector<glm::vec4> colours;
	colours.reserve(vertexCount);

	float currentZ = 0.0f;
	for (i32 i = 0; i < vertexCount; ++i)
	{
		float percent = (float)i / vertexCount;
		positions.emplace_back(0.0f, data[i], currentZ);
		colours.emplace_back(percent, 1.0f - percent, 0.5f, 1.0f);

		currentZ += g_zScale * (1.0f / vertexCount);
	}

	i32 vertexStride = sizeof(glm::vec3) + sizeof(glm::vec4);
	i32 vertexBufferSize = vertexCount * vertexStride;
	void* vertexBuffer = malloc(vertexBufferSize);
	if (!vertexBuffer)
	{
		printf("Failed to allocate memory for vertex buffer!\n");
		return;
	}

	g_DataPlotVertexBuffers.push_back((float*)vertexBuffer);

	float* currentBufferPos = (float*)vertexBuffer;
	for (i32 i = 0; i < vertexCount; ++i)
	{
		memcpy(currentBufferPos, &positions[i].x, sizeof(glm::vec3));
		currentBufferPos += 3;

		memcpy(currentBufferPos, &colours[i].x, sizeof(glm::vec4));
		currentBufferPos += 4;
	}

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);

	glGenBuffers(1, &VBO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, vertexBuffer, GL_STATIC_DRAW);

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

	glDepthMask(GL_FALSE);
	glDisable(GL_CULL_FACE);

	glm::mat4 identity = glm::mat4(1.0f);

	GLint viewProjLocation = glGetUniformLocation(g_MainProgram, "viewProj");
	GLint modelLocation = glGetUniformLocation(g_MainProgram, "model");
	glUniformMatrix4fv(viewProjLocation, 1, GL_FALSE, &identity[0][0]);
	glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &identity[0][0]);

	glDrawArrays(GL_TRIANGLES, 0, 3);
}
