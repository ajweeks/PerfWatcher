#include "stdafx.hpp"

#include <fstream>

#include "Helpers.hpp"

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
