#pragma once

#include <GL/glew.h>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
//#define GLFW_DLL
//#include <GLFW/glfw3.h>

class Shader
{
public:
	GLuint ID;

	Shader(const char* vertexPath, const char* fragmentPath);

	void Use();

	void SetBool(const std::string &name, bool value) const;
	void SetInt(const std::string& name, int value) const;
	void SetFloat(const std::string& name, float value) const;

private:
	void checkCompileErrors(unsigned int shader, std::string type);
};

