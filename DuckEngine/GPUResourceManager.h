#pragma once

#include "Shader.h"
#include "AssetManager.h"

struct GPUMesh
{
	GLuint vao = 0;
	GLuint vbo = 0;
	GLuint ebo = 0;
	GLsizei indexCount = 0;
};

struct GPUTexture
{
	GLuint id = 0;
	int width = 0;
	int height = 0;
	int channels = 0;
};


enum class BlendMode : int
{
	Opaque = 0,
	Mask = 1, //alpha test and discard
	Blend = 2 //alpha blend
};

struct GPUMaterial
{
	std::string type; // for shader selection
	std::unordered_map<std::string, float> scalars;
	std::unordered_map<std::string, glm::vec3> vectors;
	std::unordered_map<std::string, std::shared_ptr<GPUTexture>> textures;

	BlendMode blendMode = BlendMode::Opaque;
	float alphaCutoff = 0.01f;

	inline bool IsTransparent() const
	{
		return blendMode == BlendMode::Blend;
	}
};

struct GPUUBO
{
	GLuint buffer = 0;
	GLuint bindingPoint = 0;
	size_t size = 0;
	std::string name;
};


class GPUResourceManager
{
public:
	std::shared_ptr<GPUMesh> UploadMesh(MeshID id, const MeshAsset& mesh);
	std::shared_ptr<GPUMesh> GetMesh(MeshID id) const;

	std::shared_ptr<GPUTexture> UploadTexture(TextureID id, const TextureAsset& texture);
	std::shared_ptr<GPUTexture> GetTexture(TextureID id) const;
	//inline std::shared_ptr<GPUTexture> GetDefaultTexture() const { return GetTexture(INVALID_TEXTURE); }
	inline GLuint GetDefaultTextureID() const { return defaultTextureID; }

	std::shared_ptr<GPUMaterial> CreateMaterial(MaterialID id, const MaterialAsset& material);
	std::shared_ptr<GPUMaterial> GetMaterial(MaterialID id) const;

	void LoadFromAssetManager(const AssetManager& assets);
	void LoadDefaultResources(AssetManager& assets);

	void LoadDefaultShaders();
	bool AddShader(const std::string& type, const std::string& vertexPath, const std::string& fragmentPath);
	std::shared_ptr<Shader> GetShader(const std::string& type) const;

	bool CreateUBO(const std::string& name, size_t size, GLuint bindingPoint);
	bool UpdateUBO(const std::string& name, const void* data, size_t dataSize, size_t offset = 0);
	bool BindUBOToShader(const std::string& name, const std::shared_ptr<Shader>& shader);

private:
	std::unordered_map<MeshID, std::shared_ptr<GPUMesh>> meshCache;
	std::unordered_map<TextureID, std::shared_ptr<GPUTexture>> textureCache;
	std::unordered_map<MaterialID, std::shared_ptr<GPUMaterial>> materialCache;
	std::unordered_map<std::string, std::shared_ptr<Shader>> shaderCache;

	GLuint defaultTextureID = 0;

	std::unordered_map<std::string, GPUUBO> ubos;

	GLuint createVBO(const std::vector<Vertex>& vertices);
	GLuint createEBO(const std::vector<unsigned int>& indices);
	GLuint createVAO(GLuint vbo, GLuint ebo);
	GLuint createTexture2D(const TextureAsset& texture);
};

