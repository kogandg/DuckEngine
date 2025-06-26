#include "ImageWindow.h"


ImageWindow::ImageWindow(const char* title, int width, int height) : Window(title, width, height)
{
}

void ImageWindow::Init()
{
	Window::Init();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	io.Fonts->AddFontFromFileTTF("fonts/DroidSans.ttf", 22);

	// Setup input callbacks
	glfwSetWindowUserPointer(glfwWindow, this);

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"	
}

void ImageWindow::CleanUp()
{
	// Cleanup
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
	glDeleteProgram(shaderProgram);

	Window::CleanUp();
}

void ImageWindow::InitObjects(const char* texturePath)
{
	initGLObjects();

	loadTexture(texturePath);
	glfwSetWindowAspectRatio(glfwWindow, aspectWidth, aspectHeight);
}

void ImageWindow::InitObjects(const unsigned char* data, int width, int height)
{
	initGLObjects();

	loadTexture(data, width, height);
	//glfwSetWindowAspectRatio(glfwWindow, aspectWidth, aspectHeight);
}

void ImageWindow::IdleCallback()
{
	Window::IdleCallback();
}

void ImageWindow::IdleCallback(const unsigned char* data, int width, int height)
{
	if (!initialized) return;

	//Window::IdleCallback();

	loadTexture(data, width, height);

	if (savePressed)
	{
		saveImage(data, width, height);
	}
}

void ImageWindow::DisplayCallback()
{
	if (!Window::Begin()) return;
	savePressed = false;

	drawQuad();


	// Create window-specific ImGui content
	ImGui::Begin("Render info: ", nullptr, 0);

	ImGui::Text("Window width: %d", width);
	ImGui::Text("Window height: %d", height);

	if (saveEnable)
	{
		imguiSaveDialog();
	}

	ImGui::End();

	Window::End();
}



GLuint ImageWindow::compileShader(unsigned int type, const char* source)
{
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		glGetShaderInfoLog(shader, 512, nullptr, infoLog);
		std::cerr << "Shader Compilation Error: " << infoLog << std::endl;
	}

	return shader;
}

void ImageWindow::createShaderProgram(const char* vertexShaderSource, const char* fragmentShaderSource)
{
	GLuint vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
	GLuint fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	GLint success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLchar infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, nullptr, infoLog);
		std::cerr << "Program Linking Error: " << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);
}

void ImageWindow::initGLObjects()
{
	float quadVertices[] = {
		// Positions   // Texture Coords
		-1.0f,  1.0f,  0.0f, 1.0f,  // Top-left
		-1.0f, -1.0f,  0.0f, 0.0f,  // Bottom-left
		 1.0f, -1.0f,  1.0f, 0.0f,  // Bottom-right
		 1.0f,  1.0f,  1.0f, 1.0f   // Top-right
	};

	unsigned int quadIndices[] = {
		0, 1, 2,  // First triangle
		0, 2, 3   // Second triangle
	};

	// Create VAO, VBO, EBO
	//vertex array object, vertex buffer object, element buffer object;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	// Bind and fill VBO
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	// Bind and fill EBO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(quadIndices), quadIndices, GL_STATIC_DRAW);

	// Set vertex attributes
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Vertex shader source
	const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    TexCoord = aTexCoord;
}
)";
	// Fragment shader source
	const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D texture1;

void main() {
    FragColor = texture(texture1, TexCoord);
}
)";

	createShaderProgram(vertexShaderSource, fragmentShaderSource);


	//test this location
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Set texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void ImageWindow::loadTexture(const char* path)
{
	//only needs to be done once for each texture
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	// Texture parameters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//end of only once
	//leaving here as this is more likely to be called once to load an image from a file
	//removed from other loadTexture as it is reloaded every frame with new image, but texture is already bound in initialized 

	int width;
	int height;
	int nChannels;
	stbi_set_flip_vertically_on_load(true); // Flip the image vertically
	unsigned char* data = stbi_load(path, &width, &height, &nChannels, 0);
	if (data)
	{
		GLenum format = nChannels == 4 ? GL_RGBA : GL_RGB;
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		aspectWidth = width;
		aspectHeight = height;
	}
	else
	{
		std::cerr << "Failed to load texture: " << path << std::endl;
	}
	stbi_image_free(data);
}

void ImageWindow::loadTexture(const unsigned char* data, int width, int height)
{
	//flip image
	auto flipped = flipImage(data, width, height);

	// Upload the texture data to GPU
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, flipped);
	glGenerateMipmap(GL_TEXTURE_2D);

	aspectWidth = width;
	aspectHeight = height;

	delete flipped;
}

unsigned char* ImageWindow::flipImage(const unsigned char* data, int width, int height)
{
	unsigned char* flippedImage = new unsigned char[width * height * 3];

	int stride = width * 3;//3 is number of channels //number of bytes per row

	for (int y = 0; y < height; y++)
	{
		const unsigned char* srcRow = data + (y * stride);
		unsigned char* destRow = flippedImage + ((height - y - 1) * stride);

		// Copy the row from the original image to the flipped image
		std::memcpy(destRow, srcRow, stride);
	}

	return flippedImage;
}

void ImageWindow::drawQuad()
{
	// Draw the quad
	glUseProgram(shaderProgram);
	glBindTexture(GL_TEXTURE_2D, texture);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); // Use EBO for indices
}

void ImageWindow::imguiSaveDialog()
{
	ImGui::Text("Save image as: ");
	ImGui::SameLine();

	// Calculate available width
	float availableWidth = ImGui::GetContentRegionAvail().x; // Remaining space in the window
	// Clamp the width to the maximum size
	float inputWidth = (availableWidth > imguiMaxWidth) ? imguiMaxWidth : availableWidth;

	ImGui::SetNextItemWidth(inputWidth);
	ImGui::InputText("##fileSaveNameInput", &fileSaveName);

	int currentIndex = static_cast<int>(currentFileSaveType);
	ImGui::Text("Save image as type: ");
	ImGui::SameLine();

	availableWidth = ImGui::GetContentRegionAvail().x;
	float dropdownWidth = (availableWidth > imguiMaxWidth) ? imguiMaxWidth : availableWidth;

	ImGui::SetNextItemWidth(dropdownWidth);
	ImGui::Combo("##fileSaveType", &currentIndex, fileSaveTypeOptions.data(), fileSaveTypeOptions.size());
	currentFileSaveType = static_cast<FileSaveType>(currentIndex);

	if (ImGui::Button("Save Image"))
	{
		savePressed = true;
	}
}

void ImageWindow::saveImage(const unsigned char* data, int width, int height)
{
	// Save the image as a PNG file
		//const char* filename = "checkerboard.png";
	if (fileSaveName == "")
	{
		fileSaveName = "output";
	}
	std::string fullFileName = fileSaveName + "." + fileSaveTypeOptions[currentFileSaveType];

	bool saveSuccess = false;
	switch (currentFileSaveType)
	{
	case ImageWindow::PNG:
		saveSuccess = stbi_write_png(fullFileName.c_str(), width, height, 3, data, width * 3);
		break;

	case ImageWindow::BMP:
		saveSuccess = stbi_write_bmp(fullFileName.c_str(), width, height, 3, data);
		break;

	default:
		break;
	}

	if (saveSuccess)
	{
		std::cout << "Image saved to " << fullFileName << "\n";
	}
	else
	{
		std::cerr << "Failed to save image\n";
	}
}

ImVec2 ImageWindow::mapImagePosToWindowPos(ImVec2 imagePos, int imageWidth, int imageHeight)
{
	float scaleX = (float)width / imageWidth;
	float scaleY = (float)height / imageHeight;
	return ImVec2(imagePos.x * scaleX, imagePos.y * scaleY);
}
