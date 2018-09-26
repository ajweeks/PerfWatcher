#pragma once

#include "Types.hpp"

#include <vector>

bool ReadFile(const char* filePath, std::string& fileContents, bool bBinaryFile);
bool ReadFile(const char* filePath, std::vector<char>& vec, bool bBinaryFile);

std::vector<std::string> Split(const std::string& str, char delim);

float ParseFloat(const std::string& floatStr);
std::string FloatToString(float f, i32 precision);

// Removes leading & trailing whitespace on str in-place
void TrimWhitespace(std::string& str);

bool LoadGLShaders(u32 program, const char* vertShaderPath, const char* fragShaderPath, GLuint& outVertShaderID, GLuint& outFragShaderID);

bool LinkProgram(u32 program);