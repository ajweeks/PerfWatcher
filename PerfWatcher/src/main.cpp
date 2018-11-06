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
#include "OrbitCamera.hpp"

extern bool g_MouseJustPressed[5];

std::vector<std::string> g_LoadedFiles;

GLFWwindow* g_MainWindow = nullptr;
glm::vec2i g_WindowSize = glm::vec2i(1280, 720);
float g_DT = 0.0f;
bool g_LMBDown = false;
bool g_MMBDown = false;
glm::vec2i g_CursorPos;

float g_DataPointSize = 1.0f;

glm::vec3 g_AreaSize(100.0f, 60.0f, 200.0f);

float g_xScale = 1.0f;
float g_yScale = 1.0f;
EaseValue<float> g_yScaleMult(EaseType::CUBIC_IN_OUT, 0.0f, 1.0f, 2.0f);
float g_zScale = 1.0f;

GLuint g_MainProgram;
float* g_FullScreenTriVertexBuffer;
GLuint g_FullScreenTriVAO;
GLuint g_FullScreenTriVBO;

std::vector<float*> g_DataPlotVertexBuffers;
std::vector<glm::mat4> g_DataPlotModelMats;
std::vector<GLuint> g_DataVAOs;
std::vector<GLuint> g_DataVBOs;

glm::vec4 g_HoveredDataPointColorMult(4.0f, 4.0f, 4.0f, 1.0f);

struct DataPointRenderData
{
	DataPointRenderData(const glm::mat4& model, const glm::vec4& colour) :
		model(model), colour(colour) {}

	glm::mat4 model;
	glm::vec4 colour;
};

std::vector<float*> g_DataPointVertexBuffers;
std::vector<DataPointRenderData> g_DataPointRenderData;
std::vector<GLuint> g_DataPointVAOs;
std::vector<GLuint> g_DataPointVBOs;

i32 g_HAxisCount = 10;
i32 g_VAxisCount = 10;
OrbitCam g_OrbitCam;

std::vector<float*> g_AxisVertexBuffers;
std::vector<i32> g_AxisVertexBufferCounts;
std::vector<glm::mat4> g_AxisModelMats;
std::vector<GLuint> g_AxisVAOs;
std::vector<GLuint> g_AxisVBOs;

i32 g_ColorMultiplierLoc = -1;

void GLFWErrorCallback(i32 error, const char* description);
void GLFWWindowSizeCallback(GLFWwindow* window, int width, int height);
void GLFWCursorPosCallback(GLFWwindow* window, double x, double y);
void GLFWMouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
const char* GLFWGetClipboardText(void* user_data);
void GLFWSetClipboardText(void* user_data, const char* text);
void GLFWScrollCallback(GLFWwindow* window, double xOffset, double yOffset);
void GLFWKeyCallback(GLFWwindow* window, int key, int, int action, int mods);
void GLFWCharCallback(GLFWwindow* window, unsigned int c);
void GLFWDropCallback(GLFWwindow* window, int count, const char** paths);


bool ParseCSV(const char* filePath, std::vector<std::string>& outHeaders, std::vector<std::vector<float>>& outDataRows, glm::vec2& outMinmMaxValues, i32 maxRowCount = -1);

void GenerateFullScreenTri();
void GenerateVertexBufferFromData(const std::vector<float>& data, GLuint& VAO, GLuint& VBO);
void GenerateCubeVertexBuffer(float size, GLuint& VAO, GLuint& VBO);
void GenerateAxesVertexBuffer(i32 verticalCount, i32 horizCount, GLuint& VAO, GLuint& VBO);
void DescribeShaderVertexAttributes();

void DrawFullScreenQuad();

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
		glfwSetDropCallback(g_MainWindow, GLFWDropCallback);

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

		// ...

		GLuint vertShaderID, fragShaderID;
		g_MainProgram = glCreateProgram();
		LoadGLShaders(g_MainProgram, RESOURCE_LOCATION "shaders/vert.v", RESOURCE_LOCATION "shaders/frag.f", vertShaderID, fragShaderID);
		LinkProgram(g_MainProgram);

		glUseProgram(g_MainProgram);

		g_ColorMultiplierLoc = glGetUniformLocation(g_MainProgram, "ColourMultiplier");


		GenerateFullScreenTri();

		i32 plotCount = (i32)dataCols.size();
		for (i32 i = 0; i < plotCount; ++i)
		{
			const std::vector<float>& col = dataCols[i];

			GLuint VAO, VBO;

			GenerateVertexBufferFromData(col, VAO, VBO);

			g_DataVAOs.push_back(VAO);
			g_DataVBOs.push_back(VBO);

			glm::vec3 plotTranslation = glm::vec3(g_xScale * ((float)i / (plotCount - 1) - 0.5f), 0.0f, 0.0f);
			glm::mat4 model = glm::translate(glm::mat4(1.0f), plotTranslation);
			g_DataPlotModelMats.push_back(model);

			for (i32 j = 0; j < (i32)col.size(); ++j)
			{
				GLuint pointVAO, pointVBO;
				float percent = (float)j / col.size();

				GenerateCubeVertexBuffer(g_DataPointSize, pointVAO, pointVBO);

				glm::vec3 dataPointTranslation =  glm::vec3(0.0f, col[j], g_zScale * percent);
				glm::mat4 dataPointModel = glm::translate(glm::mat4(1.0f), dataPointTranslation);
				g_DataPointRenderData.emplace_back(dataPointModel, glm::vec4(1.0f));

				g_DataPointVAOs.push_back(pointVAO);
				g_DataPointVBOs.push_back(pointVBO);
			}
		}

		GLuint axesVAO, axesVBO;
		GenerateAxesVertexBuffer(g_HAxisCount, g_VAxisCount, axesVAO, axesVBO);
		g_AxisVAOs.push_back(axesVAO);
		g_AxisVBOs.push_back(axesVBO);

		glEnable(GL_DEPTH_TEST);
	}

	void LoadFile(const std::string& filePath)
	{
		std::vector<std::string> headers;
		glm::vec2 minMaxValues;
		std::vector<std::vector<float>> dataRows;
		if (!ParseCSV(filePath.c_str(), headers, dataRows, minMaxValues, 30))
		{
			printf("Failed to load data from %s!\n", filePath.c_str());
		}
		else
		{
			i32 rowCount = (i32)dataRows.size();
			i32 colCount = (i32)dataRows[0].size();
			dataCols.reserve(colCount);
			for (i32 i = 0; i < colCount; i++)
			{
				std::vector<float> col;
				col.reserve(rowCount);

				for (i32 j = 0; j < rowCount; j++)
				{
					col.push_back(dataRows[j][i]);
				}

				dataCols.push_back(col);
			}
		}

		g_xScale = g_AreaSize.x;
		g_yScale = g_AreaSize.y / (minMaxValues.y - minMaxValues.x);
		g_zScale = g_AreaSize.z;

		g_yScaleMult.Reset();


		glDeleteBuffers(g_DataVAOs.size(), g_DataVAOs.data());
		glDeleteBuffers(g_DataVBOs.size(), g_DataVBOs.data());
		g_DataVAOs.clear();
		g_DataVBOs.clear();

		i32 plotCount = (i32)dataCols.size();
		for (i32 i = 0; i < plotCount; ++i)
		{
			const std::vector<float>& col = dataCols[i];

			GLuint VAO, VBO;

			GenerateVertexBufferFromData(col, VAO, VBO);

			g_DataVAOs.push_back(VAO);
			g_DataVBOs.push_back(VBO);

			glm::vec3 plotTranslation = glm::vec3(g_xScale * ((float)i / (plotCount - 1) - 0.5f), 0.0f, 0.0f);
			glm::mat4 model = glm::translate(glm::mat4(1.0f), plotTranslation);
			g_DataPlotModelMats.push_back(model);

			for (i32 j = 0; j < (i32)col.size(); ++j)
			{
				GLuint pointVAO, pointVBO;
				float percent = (float)j / col.size();

				GenerateCubeVertexBuffer(g_DataPointSize, pointVAO, pointVBO);

				glm::vec3 dataPointTranslation = glm::vec3(0.0f, col[j], g_zScale * percent);
				glm::mat4 dataPointModel = glm::translate(glm::mat4(1.0f), dataPointTranslation);
				g_DataPointRenderData.emplace_back(dataPointModel, glm::vec4(1.0f));

				g_DataPointVAOs.push_back(pointVAO);
				g_DataPointVBOs.push_back(pointVBO);
			}
		}
	}

	void Tick()
	{
		if (ImGui::IsKeyDown(GLFW_KEY_T))
		{
			g_yScaleMult.Reset();
		}

		g_OrbitCam.Tick();
		g_yScaleMult.Tick();

		{
			glm::vec3 rayO(0.0f);
			glm::vec3 rayDir;
			GenerateDirectionRayFromScreenPos(g_CursorPos.x, g_CursorPos.y, rayO, rayDir);
			for (DataPointRenderData& dataPointRenderData : g_DataPointRenderData)
			{
				glm::vec4 pos(0.0f, 0.0f, 0.0f, 1.0f);
				pos = dataPointRenderData.model * pos;

				if (RaySphereIntersection(rayO, rayDir, (glm::vec3)pos, g_DataPointSize))
				{
					dataPointRenderData.colour = g_HoveredDataPointColorMult;
				}
				else
				{
					dataPointRenderData.colour = VEC4_UNIT;
				}
			}
		}
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

			// Start the ImGui frame
			ImGui_ImplOpenGL3_NewFrame();
			ImGui_ImplGlfw_NewFrame();
			ImGui::NewFrame();

			static const float windowUpdateRate = 0.2f;
			static float windowTitleTimer = windowUpdateRate;
			windowTitleTimer += g_DT;
			if (windowTitleTimer >= windowUpdateRate)
			{
				windowTitleTimer -= windowUpdateRate;
				float FPS = 1.0f / g_DT;
				std::string windowTitle = "PerfWatcher v0.0.2 - " + FloatToString(g_DT, 2) + "ms / " + FloatToString(FPS, 0) + " fps";
				glfwSetWindowTitle(g_MainWindow, windowTitle.c_str());
			}

			Tick();

			DoImGuiItems();

			glDepthMask(GL_TRUE);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

			glUniform4f(g_ColorMultiplierLoc, 1.0f, 1.0f, 1.0f, 1.0f);

			DrawFullScreenQuad();

			glm::mat4 viewProj = g_OrbitCam.GetViewProj();

			glDepthMask(GL_TRUE);
			glDepthFunc(GL_LEQUAL);

			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

				glUniform4f(g_ColorMultiplierLoc, 1.0f, 1.0f, 1.0f, 1.0f);

				glDrawArrays(GL_LINE_STRIP, 0, dataCols[i].size());

				i32 dataElements = (i32)dataCols[i].size();
				for (i32 j = 0; j < dataElements; ++j)
				{
					i32 idx = i * dataElements + j;

					glBindVertexArray(g_DataPointVAOs[i]);
					glBindBuffer(GL_ARRAY_BUFFER, g_DataPointVBOs[i]);

					glEnable(GL_CULL_FACE);

					glUniform4f(g_ColorMultiplierLoc,
						g_DataPointRenderData[idx].colour.x,
						g_DataPointRenderData[idx].colour.y,
						g_DataPointRenderData[idx].colour.z,
						g_DataPointRenderData[idx].colour.w);

					glm::vec3 plotTranslation = glm::vec3(
						g_xScale * ((float)i / (plotCount - 1) - 0.5f) - g_DataPointSize / 2.0f,
						g_yScale * g_yScaleMult.current * dataCols[i][j] - g_DataPointSize / 2.0f,
						g_zScale * ((float)j / dataElements - 0.5f) - g_DataPointSize / 2.0f);
					glm::mat4 dataPointModel = glm::translate(glm::mat4(1.0f), plotTranslation);
					dataPointModel = glm::rotate(dataPointModel, glm::radians(45.0f), VEC_RIGHT);
					dataPointModel = glm::rotate(dataPointModel, glm::radians(45.0f), VEC_UP);
					g_DataPointRenderData[idx].model = dataPointModel;
					glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &dataPointModel[0][0]);

					glDrawArrays(GL_TRIANGLES, 0, 36);
				}
			}

			glm::vec3 camFor = glm::normalize(g_OrbitCam.offset - g_OrbitCam.center);
			camFor.y = 0.0f;
			camFor = glm::normalize(camFor);
			float FoF = glm::dot(camFor, VEC_FORWARD);
			float FoR = glm::dot(camFor, VEC_RIGHT);
			const float dotThreshold = 0.9f;

			float fDirectionality = (glm::max(FoF, 0.0f) - dotThreshold) / (1.0f - dotThreshold);
			//printf("%.2f, %.2f, %.2f\n", FoF, FoR, fDirectionality);

			glUniform4f(g_ColorMultiplierLoc, 1.0f, 1.0f, 1.0f, 1.0f);

			for (i32 i = 0; i < (i32)g_AxisVAOs.size(); ++i)
			{
				glBindVertexArray(g_AxisVAOs[i]);
				glBindBuffer(GL_ARRAY_BUFFER, g_AxisVBOs[i]);

				glm::vec3 scaleVec = glm::vec3(g_xScale, 0.0f, g_zScale) * 0.55f;
				scaleVec.y = g_yScale * g_yScaleMult.current * 100.0f;
				glm::mat4 axesModel = glm::scale(glm::mat4(1.0f), scaleVec);
				glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &axesModel[0][0]);

				for (i32 j = 0; j < g_VAxisCount; ++j)
				{
					{
						float a = FoR < 0.0f ?  (glm::abs(FoF) - dotThreshold) / (1.0f - dotThreshold) : 1.0;
						glUniform4f(g_ColorMultiplierLoc, 1.0f, 1.0f, 1.0f, a);
						glDrawArrays(GL_LINES, j * 8, 2);
					}
					{
						float a = FoF < 0.0f ?  (glm::abs(FoR) - dotThreshold) / (1.0f - dotThreshold) : 1.0;
						glUniform4f(g_ColorMultiplierLoc, 1.0f, 1.0f, 1.0f, a);
						glDrawArrays(GL_LINES, j * 8 + 2, 2);
					}
					{
						float a = FoR > 0.0f ?  (glm::abs(FoF) - dotThreshold) / (1.0f - dotThreshold) : 1.0;
						glUniform4f(g_ColorMultiplierLoc, 1.0f, 1.0f, 1.0f, a);
						glDrawArrays(GL_LINES, j * 8 + 4, 2);
					}
					{
						float a = FoF > 0.0f ?  (glm::abs(FoR) - dotThreshold) / (1.0f - dotThreshold) : 1.0;
						glUniform4f(g_ColorMultiplierLoc, 1.0f, 1.0f, 1.0f, a);
						glDrawArrays(GL_LINES, j * 8 + 6, 2);
					}
				}
			}

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
		ImGui::SetNextWindowPos(ImVec2(1158, 22), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("##camera-control-window", &bShowCamControlWindow, ImVec2(120, 60), -1.0f,
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

		for (auto buffer : g_AxisVertexBuffers)
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

		if (ImGui::GetIO().WantCaptureMouse)
		{
			dMousePos = glm::vec2i(0);
		}

		if (g_LMBDown)
		{
			float scale = g_DT * g_OrbitCam.orbitSpeed;
			g_OrbitCam.Orbit(-dMousePos.x * scale, dMousePos.y * scale);
		}
		else if (g_MMBDown)
		{
			g_OrbitCam.Pan(g_OrbitCam.right * (float)dMousePos.x + g_OrbitCam.forward * (float)dMousePos.y);
		}
	}

	void ScrollCallback(float xOffset, float yOffset)
	{
		g_OrbitCam.Zoom(-yOffset);
	}

private:
	std::vector<std::vector<float>> dataCols;
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
	if (button >= 0 && button < ARRAY_LENGTH(g_MouseJustPressed))
	{
		if (action == GLFW_PRESS)
		{
			g_MouseJustPressed[button] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			g_MouseJustPressed[button] = false;
		}
	}
	ImGuiIO& io = ImGui::GetIO();
	if (action == GLFW_PRESS && !io.WantCaptureMouse)
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			g_LMBDown = true;
		}
		else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
		{
			g_MMBDown = true;
		}
	}
	else
	{
		if (button == GLFW_MOUSE_BUTTON_LEFT)
		{
			g_LMBDown = false;
		}
		else if (button == GLFW_MOUSE_BUTTON_MIDDLE)
		{
			g_MMBDown = false;
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

void GLFWDropCallback(GLFWwindow* window, int count, const char** paths)
{
	i32 filesAdded = 0;
	for (i32 i = 0; i < count; ++i)
	{
		if (Find(g_LoadedFiles, std::string(paths[i])) == g_LoadedFiles.end())
		{
			g_LoadedFiles.emplace_back(paths[i]);
			++filesAdded;
		}
	}

	if (filesAdded > 0)
	{
		printf("%d file(s) added, %d total\n", filesAdded, g_LoadedFiles.size());
		g_Application.LoadFile(*(g_LoadedFiles.end() - 1));
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
		glm::vec4(0.12f, 0.12f, 0.48f, 1.0f),
		glm::vec4(0.12f, 0.48f, 0.48f, 1.0f),
		glm::vec4(0.48f, 0.12f, 0.12f, 1.0f)
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
	positions.emplace_back(g_DataPointSize, g_DataPointSize, g_DataPointSize);
	positions.emplace_back(g_DataPointSize, 0.0f, g_DataPointSize);

	// Bottom
	positions.emplace_back(g_DataPointSize, 0.0f, g_DataPointSize);
	positions.emplace_back(0.0f, 0.0f, 0.0f);
	positions.emplace_back(0.0f, 0.0f, g_DataPointSize);
	positions.emplace_back(g_DataPointSize, 0.0f, g_DataPointSize);
	positions.emplace_back(g_DataPointSize, 0.0f, 0.0f);
	positions.emplace_back(0.0f, 0.0f, 0.0f);

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

void GenerateAxesVertexBuffer(i32 verticalCount, i32 horizCount, GLuint& VAO, GLuint& VBO)
{
	i32 verticalLineVertCount = (8 * verticalCount);
	i32 horizLineVertCount = (8 * horizCount);
	i32 vertexCount = verticalLineVertCount;// +horizLineVertCount;

	std::vector<glm::vec3> positions;
	positions.reserve(vertexCount);

	std::vector<glm::vec4> colours;
	colours.reserve(vertexCount);

	float currentZ = 0.0f;
	for (i32 i = 0; i < verticalCount; ++i)
	{
		float percent = (float)i / (float)(verticalCount - 1);
		positions.emplace_back(-1.0f, percent, -1.0f);
		positions.emplace_back(-1.0f, percent, 1.0f);

		positions.emplace_back(-1.0f, percent, -1.0f);
		positions.emplace_back(1.0f, percent, -1.0f);

		positions.emplace_back(1.0f, percent, 1.0f);
		positions.emplace_back(1.0f, percent, -1.0f);

		positions.emplace_back(1.0f, percent, 1.0f);
		positions.emplace_back(-1.0f, percent, 1.0f);

		colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);
		colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);

		colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);
		colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);

		colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);
		colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);

		colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);
		colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);
	}

	//for (i32 i = 0; i < horizCount; ++i)
	//{
	//	float percent = (float)i / (float)(horizCount - 1);
	//	positions.emplace_back(-1.0f, percent, -1.0f);
	//	positions.emplace_back(-1.0f, percent, 1.0f);

	//	positions.emplace_back(-1.0f, percent, -1.0f);
	//	positions.emplace_back(1.0f, percent, -1.0f);

	//	positions.emplace_back(1.0f, percent, 1.0f);
	//	positions.emplace_back(1.0f, percent, -1.0f);

	//	positions.emplace_back(1.0f, percent, 1.0f);
	//	positions.emplace_back(-1.0f, percent, 1.0f);

	//	colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);
	//	colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);

	//	colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);
	//	colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);

	//	colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);
	//	colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);

	//	colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);
	//	colours.emplace_back(0.9f, 0.9f, 0.9f, 1.0f);
	//}

	i32 vertexStride = sizeof(glm::vec3) + sizeof(glm::vec4);
	i32 vertexBufferSize = vertexCount * vertexStride;
	void* vertexBuffer = malloc(vertexBufferSize);
	if (!vertexBuffer)
	{
		printf("Failed to allocate memory for vertex buffer!\n");
		return;
	}

	g_AxisVertexBuffers.push_back((float*)vertexBuffer);
	g_AxisVertexBufferCounts.push_back(vertexCount);

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

	float currentZ = -g_zScale / 2.0f;
	for (i32 i = 0; i < vertexCount; ++i)
	{
		float percent = (float)i / vertexCount;
		positions.emplace_back(0.0f, data[i], currentZ);
		float heightPercent = data[i] / (g_yScale * g_AreaSize.y);
		colours.emplace_back(heightPercent, 1.0f - heightPercent, 0.1f, 1.0f);

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
