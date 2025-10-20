#pragma once

#include <GL/glew.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
//#define GLFW_DLL
//#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <unordered_map>

class Shader
{
public:
	GLuint ID;

	Shader(const char* vertexPath, const char* fragmentPath);

	void Use();

	void SetBool(const std::string &name, bool value) const;
	void SetInt(const std::string& name, int value) const;
	void SetFloat(const std::string& name, float value) const;

	void SetVec2(const std::string& name, const glm::vec2& value) const;
	void SetVec2(const std::string& name, float x, float y) const;

	void SetVec3(const std::string& name, const glm::vec3& value) const;
	void SetVec3(const std::string& name, float x, float y, float z) const;

	void SetVec4(const std::string& name, const glm::vec4& value) const;
	void SetVec4(const std::string& name, float x, float y, float z, float w) const;

	void SetMat2(const std::string& name, const glm::mat2& mat) const;
	void SetMat3(const std::string& name, const glm::mat3& mat) const;
	void SetMat4(const std::string& name, const glm::mat4& mat) const;

	void SetTexture(const std::string& name, int location, GLuint texture) const;

	void CacheUniform(const std::string& name);
	void CacheUniforms();

	inline bool ContainsUniform(const std::string& name) { return uniforms.contains(name); }

	std::vector<std::string>& GetActiveSamplerUniforms() { return activeSamplerUniforms; }

private:
	void checkCompileErrors(unsigned int shader, std::string type);
	std::unordered_map<std::string, GLint> uniforms;
	std::vector<std::string> activeSamplerUniforms;
};

