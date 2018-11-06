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

void GenerateDirectionRayFromScreenPos(i32 x, i32 y, glm::vec3& rayO, glm::vec3& rayDir);

bool RaySphereIntersection(const glm::vec3& rayO, const glm::vec3& rayDir, const glm::vec3& sphereCenter, float sphereRadius);

template<class T>
inline typename std::vector<T>::const_iterator Find(const std::vector<T>& vec, const T& t)
{
	for (std::vector<T>::const_iterator iter = vec.begin(); iter != vec.end(); ++iter)
	{
		if (*iter == t)
		{
			return iter;
		}
	}

	return vec.end();
}

template<class T>
T Lerp(T a, T b, float t)
{
	return (a * (1.0f - t)) + (t * b);
}

enum class EaseType
{
	CUBIC_IN_OUT,
	QUADRATIC_IN_OUT,
	ELASTIC_OUT
};

template<class T>
T Ease_CubicInOut(T a, T c, float t, float duration)
{
	t /= duration / 2.0f;
	if (t < 1.0f)
	{
		return c / 2.0f * t * t * t + a;
	}
	t -= 2.0f;
	return c / 2.0f * (t * t * t + 2.0f) + a;
}

template<class T>
T Ease_QuadInOut(T a, T c, float t, float duration)
{
	t /= duration / 2.0f;
	if (t < 1.0f)
	{
		return c / 2.0f * t * t + a;
	}
	t--;
	return -c / 2.0f * (t * (t - 2.0f) - 1.0f) + a;
}

template<class T>
T Ease_ElasticOut(T a, T c, float t, float duration)
{
	if (t == 0)
	{
		return a;
	}
	if ((t /= duration) == 1.0f)
	{
		return a + c;
	}

	float p = duration * 0.3f;
	float s = p / 4.0f;
	return (c * pow(2.0f, -10.0f * t) *
		sin((t * duration - s) * (2.0f * glm::pi<float>()) / p) + c + a);
}

template<class T>
struct EaseValue
{
	EaseValue(EaseType type, T start, T end, float duration) :
		type(type), start(start), current(start), change(end - start), duration(duration), elapsed(0.0f)
	{
	}

	void Tick()
	{
		elapsed += g_DT;

		if (elapsed < duration)
		{
			switch (type)
			{
			case EaseType::CUBIC_IN_OUT:
			{
				current = Ease_CubicInOut(start, change, elapsed, duration);
			} break;
			case EaseType::QUADRATIC_IN_OUT:
			{
				current = Ease_QuadInOut(start, change, elapsed, duration);
			} break;
			case EaseType::ELASTIC_OUT:
			{
				current = Ease_ElasticOut(start, change, elapsed, duration);
			} break;
			}
		}
		else
		{
			current = start + change;
		}
	}

	void Reset()
	{
		elapsed = 0.0f;
		current = start;
	}

	EaseType type;
	T start;
	T change;
	T current;
	float duration;
	float elapsed;
};

#define ARRAY_LENGTH(arr) ((int)(sizeof(arr)/sizeof(*arr)))

static const glm::vec3 VEC_RIGHT(1.0f, 0.0f, 0.0f);
static const glm::vec3 VEC_UP(0.0f, 1.0f, 0.0f);
static const glm::vec3 VEC_FORWARD(0.0f, 0.0f, 1.0f);

static const glm::vec3 VEC_UNIT(1.0f);
static const glm::vec3 VEC_ZERO(0.0f);
static const glm::vec4 VEC4_UNIT(1.0f);
static const glm::vec4 VEC4_ZERO(0.0f);
