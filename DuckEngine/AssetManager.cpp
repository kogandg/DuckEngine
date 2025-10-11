#include "AssetManager.h"

//Helper to convert aiColor3D to glm::vec3
static glm::vec3 ToVec3(const aiColor3D& color)
{
	return glm::vec3(color.r, color.g, color.b);
}

std::vector<MeshID> AssetManager::LoadModel(const std::string& path)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace);

	std::vector<MeshID> meshIDs;
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode || !scene->HasMeshes())
	{
		std::cout << "Failed to load model: " << path << std::endl;//maybe should just output to log, should make logger
		return meshIDs;
	}

	std::vector<MaterialID> localToGlobalMaterialMap(scene->mNumMaterials, INVALID_MATERIAL);
	for (unsigned int i = 0; i < scene->mNumMaterials; i++)
	{
		MaterialID globalID = LoadMaterial(scene->mMaterials[i], scene);
		localToGlobalMaterialMap[i] = globalID;
	}

	meshIDs = processNode(scene->mRootNode, scene, localToGlobalMaterialMap);
	return meshIDs;
}

TextureID AssetManager::LoadTexture(const std::string& path)
{
	int width;
	int height;
	int channels;
	unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
	if (!data)
	{
		std::cout << "Failed to load texture: " << path << std::endl;
		return INVALID_TEXTURE;
	}

	TextureID id = nextTextureID++;
	auto tex = std::make_shared<TextureAsset>();
	tex->width = width;
	tex->height = height;
	tex->channels = channels;
	tex->pixels.assign(data, data + (width * height * channels));
	tex->name = path;
	textures[id] = tex;
	stbi_image_free(data);
	return id;
}

TextureID AssetManager::LoadEmbeddedTexture(const aiTexture* texture, const std::string& name)
{
	if (!texture) return INVALID_TEXTURE;

	TextureID id = nextTextureID++;
	auto tex = std::make_shared<TextureAsset>();
	tex->name = name;
	tex->formatHint = texture->achFormatHint;

	if (texture->mHeight == 0)
	{
		// Compressed texture
		tex->width = texture->mWidth; // Size in bytes
		tex->height = 0;
		tex->channels = 0; // Unknown for compressed textures
		tex->pixels.resize(texture->mWidth);
		std::memcpy(tex->pixels.data(), texture->pcData, texture->mWidth);
	}
	else
	{
		// Uncompressed texture (ARGB8888)
		tex->width = texture->mWidth;
		tex->height = texture->mHeight;
		tex->channels = 4; // ARGB8888 has 4 channels
		tex->pixels.resize(tex->width * tex->height * 4);
		std::memcpy(tex->pixels.data(), texture->pcData, tex->pixels.size());
	}
	textures[id] = tex;
	return id;
}

MaterialID AssetManager::LoadMaterial(const aiMaterial* material, const aiScene* scene)
{
	if (!material) return INVALID_MATERIAL;

	MaterialID id = nextMaterialID++;
	auto mat = std::make_shared<MaterialAsset>();
	mat->name = material->GetName().C_Str();

	mat->type = "default";
	int shadingModel = 0;
	if (material->Get(AI_MATKEY_SHADING_MODEL, shadingModel) == aiReturn_SUCCESS)
	{
		if (shadingModel == aiShadingMode_NoShading)
		{
			mat->type = "unlit";
		}
		if (shadingModel == aiShadingMode_Phong)
		{
			mat->type = "phong";
		}
	}

	aiColor3D color(1.0f, 1.0f, 1.0f);
	if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn_SUCCESS)
	{
		mat->baseColor = ToVec3(color);
	}

	//load diffuse texture/base color texture
	if (material->GetTextureCount(aiTextureType_DIFFUSE) > 0)
	{
		aiString texPath;
		material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath);
		std::string texName = texPath.C_Str();

		TextureID texID = INVALID_TEXTURE;
		if (texName.length() > 0)
		{
			if (texName[0] == '*')
			{
				int texIndex = std::stoi(texName.c_str() + 1);
				if (texIndex > 0 && texIndex < scene->mNumTextures)
				{
					texID = LoadEmbeddedTexture(scene->mTextures[texIndex], texName);
				}
			}
			else
			{
				texID = LoadTexture(texName);
			}
		}
		mat->baseTexture = texID;
	}

	materials[id] = mat;
	return id;
}

inline void AssetManager::SetMeshMaterial(MeshID mesh, MaterialID material)
{
	meshMaterialLinks[mesh] = material;
}

MaterialID AssetManager::GetMeshMaterial(MeshID mesh)
{
	auto it = meshMaterialLinks.find(mesh);
	return it != meshMaterialLinks.end() ? it->second : INVALID_MATERIAL;
}

std::shared_ptr<MeshAsset> AssetManager::GetMesh(MeshID id) const
{
	auto it = meshes.find(id);
	return it != meshes.end() ? it->second : nullptr;
}

std::shared_ptr<TextureAsset> AssetManager::GetTexture(TextureID id) const
{
	auto it = textures.find(id);
	return it != textures.end() ? it->second : nullptr;
}

std::shared_ptr<MaterialAsset> AssetManager::GetMaterial(MaterialID id) const
{
	auto it = materials.find(id);
	return it != materials.end() ? it->second : nullptr;
}

std::vector<MeshID> AssetManager::processNode(aiNode* node, const aiScene* scene, const std::vector<MaterialID>& matMap)
{
	std::vector<MeshID> meshIDs;
	if (!node || !scene) return meshIDs;

	for (unsigned int i = 0; i < node->mNumMeshes; i++) 
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		MeshID id = processMesh(mesh, scene, matMap);
		if (id != INVALID_MESH) meshIDs.push_back(id);
	}
	for (unsigned int i = 0; i < node->mNumChildren; ++i) 
	{
		auto childIDs = processNode(node->mChildren[i], scene, matMap);
		meshIDs.insert(meshIDs.end(), childIDs.begin(), childIDs.end());
	}
	return meshIDs;
}

MeshID AssetManager::processMesh(aiMesh* mesh, const aiScene* scene, const std::vector<MaterialID>& matMap)
{
	if (!mesh) return INVALID_MESH;

	MeshID id = nextMeshID++;
	auto asset = std::make_shared<MeshAsset>();
	asset->name = mesh->mName.C_Str();

	//vertices
	for (unsigned int i = 0; i < mesh->mNumVertices; i++)
	{
		Vertex v;
		v.position = mesh->HasPositions() ? glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z) : glm::vec3(0.0f);
		v.normal = mesh->HasNormals() ? glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z) : glm::vec3(0.0f);
		v.texCoords = mesh->HasTextureCoords(0) ? glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y) : glm::vec2(0.0f);
		v.tangent = mesh->HasTangentsAndBitangents() ? glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z) : glm::vec3(0.0f);
		v.bitangent = mesh->HasTangentsAndBitangents() ? glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z) : glm::vec3(0.0f);
		asset->vertices.push_back(v);
	}

	//indices
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		const aiFace& face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
		{
			asset->indices.push_back(face.mIndices[j]);
		}
	}

	meshes[id] = asset;

	MaterialID matID = INVALID_MATERIAL;
	if (mesh->mMaterialIndex < matMap.size())
	{
		matID = matMap[mesh->mMaterialIndex];
	}
	meshMaterialLinks[id] = matID;
	
	return id;
}
