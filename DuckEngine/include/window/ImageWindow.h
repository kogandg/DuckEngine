#pragma once
#include "Window.h"

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <vector>
#include <map>


class ImageWindow : public Window
{
public:
	ImageWindow(const char* title, int width, int height);

	virtual void Init() override;
	virtual void CleanUp() override;

	virtual void InitObjects(const char* texturePath);
	virtual void InitObjects(const unsigned char* data, int width, int height);

	virtual void IdleCallback() override;
	void IdleCallback(const unsigned char* data, int width, int height);
	virtual void DisplayCallback();

	inline void SetSaveEnable(bool saveEnable) { this->saveEnable = saveEnable; }

protected:

	GLuint shaderProgram;
	GLuint VAO;
	GLuint VBO;
	GLuint EBO;

	int aspectWidth;
	int aspectHeight;
	GLuint texture;

	enum FileSaveType
	{
		PNG,
		BMP
	};

	std::vector<const char*> fileSaveTypeOptions = {"png", "bmp"};
	FileSaveType currentFileSaveType = FileSaveType::PNG;
	bool saveEnable = false;
	bool savePressed = false;
	std::string fileSaveName = "output";

	const float imguiMaxWidth = 1000.0f;

	GLuint compileShader(unsigned int type, const char* source);
	void createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource);
	void initGLObjects();

	void loadTexture(const char* path);
	void loadTexture(const unsigned char* data, int width, int height);

	unsigned char* flipImage(const unsigned char* data, int width, int height);

	void drawQuad();
	void imguiSaveDialog();
	void saveImage(const unsigned char* data, int width, int height);

	ImVec2 mapImagePosToWindowPos(ImVec2 imagePos, int imageWidth, int imageHeight);
};

