#pragma once
#include <vector>
#include <glm/glm.hpp>
#include <unordered_map>
#include <string>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <iostream>

#include "stb/stb_image.h"

using MeshID = unsigned int;
constexpr MeshID INVALID_MESH = 0;

using TextureID = unsigned int;
constexpr TextureID INVALID_TEXTURE = 0;

using MaterialID = unsigned int;
constexpr MaterialID INVALID_MATERIAL = 0;

//#define MAX_BONE_INFLUENCE

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoords;
	glm::vec3 tangent;
	glm::vec3 bitangent;

	//int m_boneIDs[MAX_BONE_INFLUENCE];
	//float m_weights[MAX_BONE_INFLUENCE];
};

struct MeshAsset
{
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;
	std::string name;

	//MaterialID materialID = INVALID_MATERIAL;
};

struct TextureAsset
{
	int width = 0;
	int height = 0;
	int channels = 0;
	std::vector<unsigned char> pixels;
	std::string formatHint;
	std::string name;
};

struct MaterialAsset
{
	//shaderPath, baseTexPath, baseColor
	std::string name;
	std::string type; // for shader selection

	std::unordered_map<std::string, float> scalars;
	std::unordered_map<std::string, glm::vec3> vectors;
	std::unordered_map<std::string, TextureID> textures;

	//glm::vec3 baseColor = glm::vec3(1.0f);
	//TextureID baseTexture = INVALID_TEXTURE;
};


//struct MeshData
//{
//	std::vector<glm::vec3> positions;
//	std::vector<glm::vec3> normals;
//	std::vector<glm::vec2> textureCoordinates;
//	std::vector<glm::vec3> tangents;
//	std::vector<glm::vec3> bitangents;
//
//	std::vector<unsigned int> indices;
//};

class AssetManager
{
public:
	std::vector<MeshID> LoadModel(const std::string& path);
	//MeshID AddMesh(MeshData* meshData);

	TextureID LoadTexture(const std::string& path);
	TextureID LoadEmbeddedTexture(const aiTexture* texture, const std::string& name);

	void LoadDefaultTexture();

	MaterialID LoadMaterial(const aiMaterial* material, const aiScene* scene, const std::string& path);

	inline void SetMeshMaterial(MeshID mesh, MaterialID material);// { meshMaterialLinks[mesh] = material; }//maybe add checks for contains
	MaterialID GetMeshMaterial(MeshID mesh);// { return meshMaterialLinks[mesh]; }

	std::shared_ptr<MeshAsset> GetMesh(MeshID id) const;
	std::shared_ptr<TextureAsset> GetTexture(TextureID id) const;
	std::shared_ptr<MaterialAsset> GetMaterial(MaterialID id) const;

	const std::unordered_map<MeshID, std::shared_ptr<MeshAsset>>& GetAllMeshes() const { return meshes; }
	const std::unordered_map<TextureID, std::shared_ptr<TextureAsset>>& GetAllTextures() const { return textures; }
	const std::unordered_map <MaterialID, std::shared_ptr<MaterialAsset>>& GetAllMaterials() const { return materials; }

private:
	MeshID nextMeshID = 1;
	TextureID nextTextureID = 1;
	MaterialID nextMaterialID = 1;

	std::unordered_map<MeshID, std::shared_ptr<MeshAsset>> meshes;
	std::unordered_map<TextureID, std::shared_ptr<TextureAsset>> textures;
	std::unordered_map<MaterialID, std::shared_ptr<MaterialAsset>> materials;

	std::unordered_map<MeshID, MaterialID> meshMaterialLinks;

	std::vector<MeshID> processNode(aiNode* node, const aiScene* scene, const std::vector<MaterialID>& matMap);
	MeshID processMesh(aiMesh* mesh, const aiScene* scene, const std::vector<MaterialID>& matMap);

	std::string getTextureFullPath(const std::string& modelPath, const std::string& textureRelativePath);
};