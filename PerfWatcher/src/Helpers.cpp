#include "stdafx.hpp"

#include <fstream>
#include <sstream>
#include <iomanip> // For setprecision

#include "Helpers.hpp"
#include "OrbitCamera.hpp"

extern OrbitCam g_OrbitCam;
extern GLFWwindow* g_MainWindow;

bool ReadFile(const char* filePath, std::string& fileContents, bool bBinaryFile)
{
	int fileMode = std::ios::in | std::ios::ate;
	if (bBinaryFile)
	{
		fileMode |= std::ios::binary;
	}
	std::ifstream file(filePath, fileMode);

	if (!file)
	{
		printf("Unable to read file: %s\n", filePath);
		return false;
	}

	std::streampos length = file.tellg();

	fileContents.resize((size_t)length);

	file.seekg(0, std::ios::beg);
	file.read(&fileContents[0], length);
	file.close();

	// Remove extra null terminators caused by Windows line endings
	for (u32 charIndex = 0; charIndex < fileContents.size() - 1; ++charIndex)
	{
		if (fileContents[charIndex] == '\0')
		{
			fileContents = fileContents.substr(0, charIndex);
		}
	}

	return true;
}

bool ReadFile(const char* filePath, std::vector<char>& vec, bool bBinaryFile)
{
	i32 fileMode = std::ios::in | std::ios::ate;
	if (bBinaryFile)
	{
		fileMode |= std::ios::binary;
	}
	std::ifstream file(filePath, fileMode);

	if (!file)
	{
		printf("Unable to read file: %s\n", filePath);
		return false;
	}

	std::streampos length = file.tellg();

	vec.resize((size_t)length);

	file.seekg(0, std::ios::beg);
	file.read(vec.data(), length);
	file.close();

	return true;
}

std::vector<std::string> Split(const std::string& str, char delim)
{
	std::vector<std::string> result;
	size_t i = 0;

	size_t strLen = str.size();
	while (i != strLen)
	{
		while (i != strLen && str[i] == delim)
		{
			++i;
		}

		size_t j = i;
		while (j != strLen && str[j] != delim)
		{
			++j;
		}

		if (i != j)
		{
			result.push_back(str.substr(i, j - i));
			i = j;
		}
	}

	return result;
}

void TrimWhitespace(std::string& str)
{
	auto startIter = str.begin();
	while (isspace(*startIter))
	{
		startIter = str.erase(startIter);
	}

	auto revIter = str.rbegin();
	while (isspace(*revIter))
	{
		str.pop_back();
		revIter = str.rbegin();
	}
}

float ParseFloat(const std::string& floatStr)
{
	if (floatStr.empty())
	{
		printf("Invalid float string (empty)\n");
		return -1.0f;
	}

	return (float)std::atof(floatStr.c_str());
}

std::string FloatToString(float f, i32 precision)
{
	std::stringstream stream;

	stream << std::fixed << std::setprecision(precision) << f;

	return stream.str();
}

bool LoadGLShaders(u32 program, const char* vertShaderPath, const char* fragShaderPath, GLuint& outVertShaderID, GLuint& outFragShaderID)
{
	bool bSuccess = true;

	outVertShaderID = glCreateShader(GL_VERTEX_SHADER);
	outFragShaderID = glCreateShader(GL_FRAGMENT_SHADER);

	{
		printf("Loading shaders %s & %s\n", vertShaderPath, fragShaderPath);
	}

	std::vector<char> vertShaderCode;
	if (!ReadFile(vertShaderPath, vertShaderCode, false))
	{
		printf("Could not load vert shader from %s\n", vertShaderPath);
	}
	vertShaderCode.push_back('\0'); // Signal end of string with terminator character

	std::vector<char> fragShaderCode;
	if (!ReadFile(fragShaderPath, fragShaderCode, false))
	{
		printf("Could not load frag shader from %s\n", fragShaderPath);
	}
	fragShaderCode.push_back('\0'); // Signal end of string with terminator character

	GLint result = GL_FALSE;
	i32 infoLogLength;

	// Compile vertex shader
	char const* vertexSourcepointer = vertShaderCode.data(); // TODO: Test
	glShaderSource(outVertShaderID, 1, &vertexSourcepointer, NULL);
	glCompileShader(outVertShaderID);

	glGetShaderiv(outVertShaderID, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE)
	{
		glGetShaderiv(outVertShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
		std::string vertexShaderErrorMessage;
		vertexShaderErrorMessage.resize((size_t)infoLogLength);
		glGetShaderInfoLog(outVertShaderID, infoLogLength, NULL, (GLchar*)vertexShaderErrorMessage.data());
		printf("%s\n", vertexShaderErrorMessage.c_str());
		bSuccess = false;
	}

	// Compile fragment shader
	char const* fragmentSourcePointer = fragShaderCode.data();
	glShaderSource(outFragShaderID, 1, &fragmentSourcePointer, NULL);
	glCompileShader(outFragShaderID);

	glGetShaderiv(outFragShaderID, GL_COMPILE_STATUS, &result);
	if (result == GL_FALSE)
	{
		glGetShaderiv(outFragShaderID, GL_INFO_LOG_LENGTH, &infoLogLength);
		std::string fragmentShaderErrorMessage;
		fragmentShaderErrorMessage.resize((size_t)infoLogLength);
		glGetShaderInfoLog(outFragShaderID, infoLogLength, NULL, (GLchar*)fragmentShaderErrorMessage.data());
		printf("%s\n", fragmentShaderErrorMessage.c_str());
		bSuccess = false;
	}

	glAttachShader(program, outVertShaderID);
	glAttachShader(program, outFragShaderID);

	return bSuccess;
}

bool LinkProgram(u32 program)
{
	glLinkProgram(program);

	GLint result = GL_FALSE;
	glGetProgramiv(program, GL_LINK_STATUS, &result);
	if (result == GL_FALSE)
	{
		return false;
	}

	return true;
}

void GenerateDirectionRayFromScreenPos(i32 x, i32 y, glm::vec3& rayO, glm::vec3& rayDir)
{
	i32 frameBufferWidth;
	i32 frameBufferHeight;
	glfwGetWindowSize(g_MainWindow, &frameBufferWidth, &frameBufferHeight);
	float aspectRatio = (float)frameBufferWidth / frameBufferHeight;

	float tanFov = tanf(0.5f * g_OrbitCam.FOV);

	float pixelScreenX = 2.0f * ((x + 0.5f) / (float)frameBufferWidth) - 1.0f;
	float pixelScreenY = 1.0f - 2.0f * ((y + 0.5f) / (float)frameBufferHeight);

	float pixelCameraX = pixelScreenX * aspectRatio * tanFov;
	float pixelCameraY = pixelScreenY * tanFov;


	glm::mat4 cameraView = glm::inverse(g_OrbitCam.GetView());

	rayO = cameraView * glm::vec4(rayO, 1.0f);

	glm::vec3 rayPWorld = cameraView * glm::vec4(pixelCameraX, pixelCameraY, -1.0f, 1.0f);
	rayDir = glm::normalize(rayPWorld - rayO);
}

bool RaySphereIntersection(const glm::vec3& rayO, const glm::vec3& rayDir, const glm::vec3& sphereCenter, float sphereRadius)
{
	float t_min = 0.1f;
	float t_max = 1000.0f;
	glm::vec3 oc = rayO - sphereCenter;
	float a = dot(rayDir, rayDir);
	float b = dot(oc, rayDir);
	float c = dot(oc, oc) - sphereRadius * sphereRadius;
	float discriminant = b * b - a * c;
	if (discriminant > 0)
	{
		float temp = (-b - sqrt(discriminant)) / a;
		if (temp < t_max && temp > t_min)
		{
			return true;
		}

		temp = (-b + sqrt(discriminant)) / a;
		if (temp < t_max && temp > t_min)
		{
			return true;
		}
	}

	return false;
}