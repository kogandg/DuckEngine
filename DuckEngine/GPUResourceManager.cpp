#include "GPUResourceManager.h"

GLuint GPUResourceManager::createVBO(const std::vector<Vertex>& vertices)
{
	GLuint vbo;
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);
	return vbo;
}

GLuint GPUResourceManager::createEBO(const std::vector<unsigned int>& indices)
{
	GLuint ebo;
	glGenBuffers(1, &ebo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);
	return ebo;
}

GLuint GPUResourceManager::createVAO(GLuint vbo, GLuint ebo)
{
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);

	glEnableVertexAttribArray(0); // position
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
	glEnableVertexAttribArray(1); // normal
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
	glEnableVertexAttribArray(2); // texCoords
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoords));
	glEnableVertexAttribArray(3); // tangent
	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, tangent));
	glEnableVertexAttribArray(4); // bitangent
	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, bitangent));
	glBindVertexArray(0);
	return vao;
}

GLuint GPUResourceManager::createTexture2D(const TextureAsset& texture)
{
	GLuint texID;
	glGenTextures(1, &texID);
	glBindTexture(GL_TEXTURE_2D, texID);

	GLenum format;
	if (texture.channels == 1) format = GL_RED;
	else if (texture.channels == 3) format = GL_RGB;
	else if (texture.channels == 4) format = GL_RGBA;
	else
	{
		std::cout << "Unsupported texture format with " << texture.channels << " channels." << std::endl;
		return 0;
	}

	glTexImage2D(GL_TEXTURE_2D, 0, format, texture.width, texture.height, 0, format, GL_UNSIGNED_BYTE, texture.pixels.data());
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	return texID;
}

std::shared_ptr<GPUMesh> GPUResourceManager::UploadMesh(MeshID id, const MeshAsset& mesh)
{
	auto gpuMesh = std::make_shared<GPUMesh>();
	gpuMesh->vbo = createVBO(mesh.vertices);
	gpuMesh->ebo = createEBO(mesh.indices);
	gpuMesh->vao = createVAO(gpuMesh->vbo, gpuMesh->ebo);
	gpuMesh->indexCount = static_cast<GLsizei>(mesh.indices.size());
	meshCache[id] = gpuMesh;
	return gpuMesh;
}

std::shared_ptr<GPUMesh> GPUResourceManager::GetMesh(MeshID id) const
{
	auto it = meshCache.find(id);
	return it != meshCache.end() ? it->second : nullptr;
}

std::shared_ptr<GPUTexture> GPUResourceManager::UploadTexture(TextureID id, const TextureAsset& texture)
{
	auto gpuTex = std::make_shared<GPUTexture>();
	gpuTex->id = createTexture2D(texture);
	gpuTex->width = texture.width;
	gpuTex->height = texture.height;
	gpuTex->channels = texture.channels;
	textureCache[id] = gpuTex;
	return gpuTex;
}

std::shared_ptr<GPUTexture> GPUResourceManager::GetTexture(TextureID id) const
{
	auto it = textureCache.find(id);
	return it != textureCache.end() ? it->second : nullptr;
}

std::shared_ptr<GPUMaterial> GPUResourceManager::CreateMaterial(MaterialID id, const MaterialAsset& material)
{
	auto gpuMat = std::make_shared<GPUMaterial>();

	gpuMat->type = material.type;
	//gpuMat->shader = std::make_shared<Shader>("shaders/default.vert", "shaders/default.frag");

	gpuMat->scalars = material.scalars;
	gpuMat->vectors = material.vectors;
	
	for (auto textureID : material.textures)
	{
		auto tex = GetTexture(textureID.second);
		if (tex) gpuMat->textures[textureID.first] = tex;
	}

	materialCache[id] = gpuMat;
	return gpuMat;
}

std::shared_ptr<GPUMaterial> GPUResourceManager::GetMaterial(MaterialID id) const
{
	auto it = materialCache.find(id);
	return it != materialCache.end() ? it->second : nullptr;
}

void GPUResourceManager::LoadFromAssetManager(const AssetManager& assets)
{
	for (const auto& [id, mesh] : assets.GetAllMeshes()) 
	{
		UploadMesh(id, *mesh);
	}
	for (const auto& [id, tex] : assets.GetAllTextures()) 
	{
		UploadTexture(id, *tex);
	}
	for (const auto& [id, mat] : assets.GetAllMaterials()) 
	{
		CreateMaterial(id, *mat);
	}
}

void GPUResourceManager::LoadDefaultResources(AssetManager& assets)
{
	TextureID defaultTexID = assets.GenerateDefaultTexture(glm::ivec4(255, 255, 255, 255));
	auto defaultTex = assets.GetTexture(defaultTexID);
	auto gpuTex = UploadTexture(defaultTexID, *defaultTex);
	defaultTextureID = gpuTex->id;
}

void GPUResourceManager::LoadDefaultShaders()
{
	shaderCache["default"] = std::make_shared<Shader>("shaders/default.vert", "shaders/default.frag");
	shaderCache["unlit"] = std::make_shared<Shader>("shaders/unlit.vert", "shaders/unlit.frag");
	shaderCache["phong"] = std::make_shared<Shader>("shaders/phong.vert", "shaders/phong.frag");

	for (const auto& [name, shader] : shaderCache)
	{
		shader->CacheUniforms();
	}
}

bool GPUResourceManager::AddShader(const std::string& type, const std::string& vertexPath, const std::string& fragmentPath)
{
	if (shaderCache.find(type) != shaderCache.end())
	{
		return false;
	}

	auto shader = std::make_shared<Shader>(vertexPath.c_str(), fragmentPath.c_str());
	shaderCache[type] = shader;
	return true;
}

std::shared_ptr<Shader> GPUResourceManager::GetShader(const std::string& type) const
{
	auto it = shaderCache.find(type);
	return it != shaderCache.end() ? it->second : nullptr;
}

bool GPUResourceManager::CreateUBO(const std::string& name, size_t size, GLuint bindingPoint)
{
	if (ubos.find(name) != ubos.end()) return false;

	GPUUBO ubo;
	ubo.bindingPoint = bindingPoint;
	ubo.size = size;
	ubo.name = name;

	glGenBuffers(1, &ubo.buffer);
	glBindBuffer(GL_UNIFORM_BUFFER, ubo.buffer);
	glBufferData(GL_UNIFORM_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, ubo.buffer);

	ubos[name] = ubo;
	return true;
}

bool GPUResourceManager::UpdateUBO(const std::string& name, const void* data, size_t dataSize, size_t offset)
{
	auto it = ubos.find(name);
	if (it == ubos.end() || (offset + dataSize > it->second.size)) return false;

	glBindBuffer(GL_UNIFORM_BUFFER, it->second.buffer);
	glBufferSubData(GL_UNIFORM_BUFFER, offset, dataSize, data);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	return true;
}

bool GPUResourceManager::BindUBOToShader(const std::string& name, const std::shared_ptr<Shader>& shader)
{
	if (ubos.find(name) != ubos.end()) return false;

	GLuint blockIndex = glGetUniformBlockIndex(shader->ID, name.c_str());
	if (blockIndex == GL_INVALID_INDEX) return false;

	glUniformBlockBinding(shader->ID, blockIndex, ubos[name].bindingPoint);
	return true;
}