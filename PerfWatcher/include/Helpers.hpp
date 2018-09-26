#pragma once

#include "Types.hpp"

#include <vector>

bool ReadFile(const char* filePath, std::vector<char>& vec, bool bBinaryFile);

bool LoadGLShaders(u32 program, const char* vertShaderPath, const char* fragShaderPath, GLuint& outVertShaderID, GLuint& outFragShaderID);

bool LinkProgram(u32 program);