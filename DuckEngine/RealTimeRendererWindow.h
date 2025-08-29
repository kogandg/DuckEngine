#pragma once
#include "Window.h"
#include "Shader.h"

class RealTimeRendererWindow : public Window
{
public:
	RealTimeRendererWindow(const char* title, int width, int height);

	virtual void Init() override;
	virtual void CleanUp() override;

	virtual void IdleCallback() override;
	virtual void DisplayCallback() override;

	virtual void InitObjects();

private:
	GLuint VBO;
	GLuint VAO;
	GLuint EBO;
	//GLuint shaderProgram;
	Shader* shader;

	void initGLObjects();
};

