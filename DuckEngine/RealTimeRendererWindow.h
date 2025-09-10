#pragma once

#include "Window.h"

#include "stb/stb_image.h"

#include "Shader.h"
#include <array>
#include "Camera.h"
#include "Scene.h"

namespace GPUScene
{
	struct VertexAttrib
	{
		GLuint location; //glsl layout location
		GLint size;
		GLenum type; //GL_FLOAT, etc
		unsigned int offset; //byte offset

		bool operator==(const VertexAttrib& other) const noexcept
		{
			return location == other.location && size == other.size && type == other.type && offset == other.offset;
		}
	};

	struct VertexLayoutKey
	{
		GLsizei stride; //stride in bytes
		std::vector<VertexAttrib> attrs;

		bool operator==(const VertexLayoutKey& other) const noexcept
		{
			return stride == other.stride && attrs == other.attrs;
		}
	};

	struct VertexLayoutKeyHash
	{
		size_t operator()(const VertexLayoutKey& k) const noexcept
		{
			size_t h = std::hash<size_t>{}(k.stride);
			for (auto& attr : k.attrs)
			{
				size_t ha = std::hash<GLuint>{}(attr.location) ^
					(std::hash<GLint>{}(attr.size) << 1) ^
					(std::hash<GLenum>{}(attr.type) << 2) ^
					(std::hash<size_t>{}(attr.offset) << 3);
				h ^= ha + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2); // hash combine
			}
			return h;
		}
	};

	struct GPUGeometry
	{
		GLuint VBO = 0;
		//GLuint EBO = 0;     // 0 if not indexed
		//GLsizei indexCount = 0;
		GLsizei vertexCount = 0;
		VertexLayoutKey layout; // for VAO creation
	};

	/*struct GPUTexture
	{
		GLuint id = 0;
	};*/

	enum class MaterialType
	{
		Color, Phong
	};

	struct GPUMaterial
	{
		MaterialType type;
		std::shared_ptr<Shader> shader;

		std::shared_ptr<GLuint> diffuseTexture;
		std::shared_ptr<GLuint> specularTexture;

		glm::vec3 color = glm::vec3(1.0f);
		float shininess = 32.0f;
	};

	struct VAOKey
	{
		GLuint VBO;
		//GLuint EBO;

		VertexLayoutKey layout;
		bool operator==(const VAOKey& other) const noexcept
		{
			return VBO == other.VBO && layout == other.layout;
		}
	};

	struct VAOEntry
	{
		GLuint VAO = 0;
	};

	struct GPUCache
	{
		std::unordered_map<std::shared_ptr<ECS::MeshData>, std::shared_ptr<GPUGeometry>> geomCache;

		//textures keyed by file path
		std::unordered_map<std::string, std::shared_ptr<GLuint>> textureCache;

		std::unordered_map<MaterialType, std::shared_ptr<Shader>> shaderCache;

		std::unordered_map<std::shared_ptr<ECS::MaterialData>, std::shared_ptr<GPUMaterial>> materialCache;

		struct VAOKeyHash
		{
			size_t operator()(const VAOKey& k) const noexcept
			{
				size_t h1 = std::hash<GLuint>{}(k.VBO);
				size_t h2 = VertexLayoutKeyHash{}(k.layout);
				return h1 ^ (h2 << 1);
			}
		};
		std::unordered_map<VAOKey, VAOEntry, VAOKeyHash> vaoCache;
	};

	std::shared_ptr<GPUGeometry> getOrCreateGeometry(GPUCache& c, const std::shared_ptr<ECS::MeshData>& md, const VertexLayoutKey& layout);
	std::shared_ptr<Shader> getOrCreateShader(GPUCache& c, MaterialType type, const char* vertexPath, const char* fragmentPath);
	std::shared_ptr<GLuint> getOrCreateTexture(GPUCache& c, const std::string& path);
	std::shared_ptr<GPUMaterial> getOrCreateMaterial(GPUCache& c, const std::shared_ptr<ECS::MaterialData>& md);
	GLuint buildVAO(const GPUGeometry& g, const Shader& s);
	GLuint getOrCreateVAO(GPUCache& c, const std::shared_ptr<GPUGeometry>& g, const std::shared_ptr<Shader>& s);

	GLuint compileAndLinkShader(std::string vertexPath, std::string fragmentPath);
	void checkShaderCompileErrors(GLuint shader, std::string type);

	void preloadGPU(ECS::ECS& ecs, GPUCache& cache);
};

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
	ECS::ECS ecs;
	GPUScene::GPUCache cache;

	GLuint VBO;
	GLuint cubeVAO;
	GLuint lightVAO;
	//GLuint EBO;
	//GLuint shaderProgram;

	Shader* lightingShader;
	Shader* lightCubeShader;
	
	GLuint diffuseMap;
	GLuint specularMap;

	std::array<glm::vec3, 10> cubePositions = { {
		glm::vec3(0.0f,  0.0f,  0.0f),
		glm::vec3(2.0f,  5.0f, -15.0f),
		glm::vec3(-1.5f, -2.2f, -2.5f),
		glm::vec3(-3.8f, -2.0f, -12.3f),
		glm::vec3(2.4f, -0.4f, -3.5f),
		glm::vec3(-1.7f,  3.0f, -7.5f),
		glm::vec3(1.3f, -2.0f, -2.5f),
		glm::vec3(1.5f,  2.0f, -2.5f),
		glm::vec3(1.5f,  0.2f, -1.5f),
		glm::vec3(-1.3f,  1.0f, -1.5f)
	} };

	std::array<glm::vec3, 4> pointLightPositions = { {
		glm::vec3(0.7f,  0.2f,  2.0f),
		glm::vec3(2.3f, -3.3f, -4.0f),
		glm::vec3(-4.0f,  2.0f, -12.0f),
		glm::vec3(0.0f,  0.0f, -3.0f)
	} };

	glm::vec3 lightPosition = glm::vec3(1.2f, 1.0f, 2.0f);

	Camera camera;

	float movementSpeed = 2.5f;
	float mouseSensitivity = 0.1f;

	float deltaTime = 0.0f;
	float lastTime = 0.0f;

	void initGLObjects();

	GLuint loadTexture(const char* path);

	bool firstMouse = true;
	float lastX;
	float lastY;
	virtual void onCursorPos(double currX, double currY) override;
};

