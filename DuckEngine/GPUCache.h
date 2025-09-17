#pragma once
#include <unordered_map>
#include "ECSRegistry.h"
#include "Shader.h"
#include "stb/stb_image.h"

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

struct ShaderSignature
{
	std::vector<VertexAttrib> required;
	bool operator==(const ShaderSignature& other) const noexcept
	{
		return required == other.required;
	}
};
struct ShaderSignatureHash
{
	size_t operator()(const ShaderSignature& s) const noexcept
	{
		size_t h = 0;
		for (auto& attr : s.required)
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

struct GPUMaterial
{
	enum class Type
	{
		Color, Phong
	};
	Type type;
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

	ShaderSignature signature;//what shader requires
	bool operator==(const VAOKey& other) const noexcept
	{
		return VBO == other.VBO && signature == other.signature;
	}
};
struct VAOKeyHash
{
	size_t operator()(const VAOKey& k) const noexcept
	{
		size_t h1 = std::hash<GLuint>{}(k.VBO);
		size_t h2 = ShaderSignatureHash{}(k.signature);
		return h1 ^ (h2 << 1);
	}
};

//struct VAOEntry
//{
//	GLuint VAO = 0;
//};

class GPUCache
{
public:
	void PreloadGPU(ECS::ECSRegistry& registry);

	std::shared_ptr<GPUGeometry> inline GetGeometry(const std::shared_ptr<ECS::MeshData>& md) { return geomCache[md]; }
	std::shared_ptr<GLuint> inline GetTexture(const std::string& path) { return textureCache[path]; }
	std::shared_ptr<Shader> inline GetShader(GPUMaterial::Type type) { return shaderCache[type]; }
	std::shared_ptr<GPUMaterial> inline GetMaterial(const std::shared_ptr<ECS::MaterialData>& md) { return materialCache[md]; }
	std::shared_ptr<GLuint> inline GetVAO(const GPUGeometry& g, const GPUMaterial& mat) { return vaoCache[{g.VBO, GetRequiredSignature(mat) }]; }
	std::shared_ptr<GLuint> inline GetVAO(const GPUGeometry& g, const ShaderSignature& sig) { return vaoCache[{g.VBO, sig}]; }

	ShaderSignature GetRequiredSignature(const GPUMaterial& mat);

private:
	std::unordered_map<std::shared_ptr<ECS::MeshData>, std::shared_ptr<GPUGeometry>> geomCache;
	//textures keyed by file path
	std::unordered_map<std::string, std::shared_ptr<GLuint>> textureCache;
	std::unordered_map<GPUMaterial::Type, std::shared_ptr<Shader>> shaderCache;
	std::unordered_map<std::shared_ptr<ECS::MaterialData>, std::shared_ptr<GPUMaterial>> materialCache;
	std::unordered_map<VAOKey, std::shared_ptr<GLuint>, VAOKeyHash> vaoCache;
	

	GLuint loadTexture(const char* path);

	std::shared_ptr<GPUGeometry> createGeometry(const std::shared_ptr<ECS::MeshData>& md, const VertexLayoutKey& layout);
	std::shared_ptr<Shader> createShader(GPUMaterial::Type type, const char* vertexPath, const char* fragmentPath);
	std::shared_ptr<GLuint> createTexture(const std::string& path);
	std::shared_ptr<GPUMaterial> createMaterial(const std::shared_ptr<ECS::MaterialData>& md);
	std::shared_ptr<GLuint> createVAO(const GPUGeometry& geom, const VertexLayoutKey& layout, const ShaderSignature& signature);
};

