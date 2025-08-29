#include "RealTimeRendererWindow.h"


RealTimeRendererWindow::RealTimeRendererWindow(const char* title, int width, int height) : Window(title, width, height)
{
}

void RealTimeRendererWindow::Init()
{
	Window::Init();
	// Setup input callbacks
	glfwSetWindowUserPointer(glfwWindow, this);

	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS); // depth-testing interprets a smaller value as "closer"
}

void RealTimeRendererWindow::CleanUp()
{
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	//glDeleteProgram(shaderProgram);

	Window::CleanUp();
}

void RealTimeRendererWindow::IdleCallback()
{
	Window::IdleCallback();
}

void RealTimeRendererWindow::DisplayCallback()
{
	if (!Window::Begin()) return;

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);

	//float timeValue = glfwGetTime();
	//float greenValue = (sin(timeValue) / 2.0f) + 0.5f;
	//int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");

	//glUseProgram(shaderProgram);
	//glUniform4f(vertexColorLocation, 0.0f, greenValue, 0.0f, 1.0f);

	shader->Use();
	//shader->SetFloat("offSet", 1.0f);

	glBindVertexArray(VAO);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glDrawArrays(GL_TRIANGLES, 0, 3);

	Window::End();
}

void RealTimeRendererWindow::InitObjects()
{
	initGLObjects();
}

void RealTimeRendererWindow::initGLObjects()
{
	//shaders

	shader = new Shader("vertexShader.vert", "fragShader.frag");

	///*const char* vertexShaderSource = "#version 330 core\n"
	//	"layout (location = 0) in vec3 aPos;\n"
	//	"void main()\n"
	//	"{\n"
	//	"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
	//	"}\0";*/

	//const char* vertexShaderSource = "#version 330 core\n"
	//	"layout (location = 0) in vec3 aPos;\n"
	//	"layout (location = 1) in vec3 aColor;\n"
	//	"out vec3 ourColor;\n"
	//	"void main()\n"
	//	"{\n"
	//	"   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
	//	"	ourColor = aColor;\n"
	//	"}\0";

	///*const char* vertexShaderSource = "#version 330 core\n"
	//	"layout(location = 0) in vec3 aPos; // the position variable has attribute position 0\n"
	//	"out vec4 vertexColor; // specify a color output to the fragment shader\n"
	//	"void main()\n"
	//	"{\n"
	//	"gl_Position = vec4(aPos, 1.0); // see how we directly give a vec3 to vec4's constructor\n"
	//	"vertexColor = vec4(0.5, 0.0, 0.0, 1.0); // set the output variable to a dark-red color\n"
	//	"}\0";*/

	//GLuint vertexShader;
	//vertexShader = glCreateShader(GL_VERTEX_SHADER);
	//glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	//glCompileShader(vertexShader);

	//GLint  success;
	//char infoLog[512];
	//glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	//if (!success)
	//{
	//	glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
	//	std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	//}


	///*const char* fragmentShaderSource = "#version 330 core\n"
	//	"out vec4 FragColor;\n"
	//	"void main()\n"
	//	"{\n"
	//	"FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);\n"
	//	"}\0";*/

	///*const char* fragmentShaderSource = "#version 330 core\n"
	//	"out vec4 FragColor;\n"
	//	"uniform vec4 ourColor;\n"
	//	"void main()\n"
	//	"{\n"
	//	"FragColor = ourColor;\n"
	//	"}\0";*/

	//const char* fragmentShaderSource = "#version 330 core\n"
	//	"out vec4 FragColor;\n"
	//	"in vec3 ourColor;\n"
	//	"void main()\n"
	//	"{\n"
	//	"FragColor = vec4(ourColor, 1.0);\n"
	//	"}\0";

	///*const char* fragmentShaderSource = "#version 330 core\n"
	//	"out vec4 FragColor;\n"
	//	"in vec4 vertexColor; // the input variable from the vertex shader (same name and same type)\n"
	//	"void main()\n"
	//	"{\n"
	//	"FragColor = vertexColor;\n"
	//	"}\0";*/

	//GLuint fragmentShader;
	//fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	//glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	//glCompileShader(fragmentShader);

	//glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	//if (!success)
	//{
	//	glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
	//	std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	//}

	//shaderProgram = glCreateProgram();

	//glAttachShader(shaderProgram, vertexShader);
	//glAttachShader(shaderProgram, fragmentShader);
	//glLinkProgram(shaderProgram);

	//glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	//if (!success) {
	//	glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
	//	std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	//}
	//glDeleteShader(vertexShader);
	//glDeleteShader(fragmentShader);

	////glUseProgram(shaderProgram);


	//verts

	//float vertices[] = {
	//	 0.5f,  0.5f, 0.0f,  // top right
	//	 0.5f, -0.5f, 0.0f,  // bottom right
	//	-0.5f, -0.5f, 0.0f,  // bottom left
	//	-0.5f,  0.5f, 0.0f   // top left 
	//};
	//unsigned int indices[] = {  // note that we start from 0!
	//	0, 1, 3,  // first Triangle
	//	1, 2, 3   // second Triangle
	//};

	//float vertices[] = {
	//	-0.5f, -0.5f, 0.0f, // left  
	//	 0.5f, -0.5f, 0.0f, // right 
	//	 0.0f,  0.5f, 0.0f  // top   
	//};

	float vertices[] = {
		// positions         // colors
		 0.5f, -0.5f, 0.0f,  1.0f, 0.0f, 0.0f,   // bottom right
		-0.5f, -0.5f, 0.0f,  0.0f, 1.0f, 0.0f,   // bottom left
		 0.0f,  0.5f, 0.0f,  0.0f, 0.0f, 1.0f    // top 
	};

	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	//glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	//position attrib
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 2 * 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	//color attrib
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 2 * 3 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);

	//glBindVertexArray(0);

}


