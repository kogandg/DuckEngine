#include "ECSRegistry.h"

#include "imgui/imgui.h"
#include <iostream>

using namespace ECS;

void ECS::CameraController::init(Transform& transform)
{
	updateRotation(transform);
}

void ECS::CameraController::update(Transform& transform, InputManager& input, float dt)
{
	if (input.IsMouseDown(MouseButton::Right))
	{
		glm::vec2 delta = input.GetMouseDelta() * mouseSensitivity;
		yaw += delta.x;
		pitch += delta.y;
		pitch = glm::clamp(pitch, -89.0f, 89.0f);

		updateRotation(transform);
	}

	glm::vec3 forward = transform.rotation * glm::vec3(0, 0, -1);
	glm::vec3 right = transform.rotation * glm::vec3(1, 0, 0);
	glm::vec3 up = transform.rotation * glm::vec3(0, 1, 0);

	float velocity = movementSpeed * dt;

	if (input.IsKeyDown(Key::W)) transform.position += forward * velocity;
	if (input.IsKeyDown(Key::S)) transform.position -= forward * velocity;
	if (input.IsKeyDown(Key::A)) transform.position -= right * velocity;
	if (input.IsKeyDown(Key::D)) transform.position += right * velocity;
	if (input.IsKeyDown(Key::Space)) transform.position += up * velocity;
	if (input.IsKeyDown(Key::Control)) transform.position -= up * velocity;

	transform.dirty = true;
}

glm::vec3 ECS::CameraController::getFront()
{
	float cy = cos(glm::radians(yaw));
	float sy = sin(glm::radians(yaw));
	float cp = cos(glm::radians(pitch));
	float sp = sin(glm::radians(pitch));

	glm::vec3 trueDirection;
	trueDirection.x = cy * cp;
	trueDirection.y = sp;
	trueDirection.z = sy * cp;
	trueDirection = glm::normalize(trueDirection);

	return trueDirection;
}

void ECS::CameraController::updateRotation(Transform& transform)
{
	float cy = cos(glm::radians(yaw));
	float sy = sin(glm::radians(yaw));
	float cp = cos(glm::radians(pitch));
	float sp = sin(glm::radians(pitch));

	glm::vec3 trueDirection;
	trueDirection.x = cy * cp;
	trueDirection.y = sp;
	trueDirection.z = sy * cp;
	trueDirection = glm::normalize(trueDirection);

	transform.rotation = glm::rotation(glm::vec3(0, 0, -1), trueDirection);
	transform.dirty = true;
}

//void ECS::Material::makeMaterialUnique()
//{
//	auto newData = MaterialData(*data.get());
//	std::shared_ptr<MaterialData> newDataPtr = std::make_shared<MaterialData>(newData);
//	newDataPtr.get()->dirty = true;
//
//	data = newDataPtr;
//}



////importer stuff
//std::shared_ptr<ECS::MeshData> processMesh(aiMesh* mesh, const aiScene* scene)//will need scene later for materials
//{
//	auto meshData = std::make_shared<MeshData>();
//
//	//verts
//	for (int i = 0; i < mesh->mNumVertices; i++)
//	{
//		if (mesh->HasPositions())
//		{
//			meshData->positions.emplace_back(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
//		}
//
//		if (mesh->HasNormals())
//		{
//			meshData->normals.emplace_back(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
//		}
//
//		if (mesh->HasTextureCoords(0))
//		{
//			meshData->textureCoordinates.emplace_back(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
//		}
//		else
//		{
//			meshData->textureCoordinates.emplace_back(0.0f, 0.0f);
//		}
//
//		if (mesh->HasTangentsAndBitangents())
//		{
//			meshData->tangents.emplace_back(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
//			meshData->bitangents.emplace_back(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
//		}
//	}
//
//	//indices
//	for (int i = 0; i < mesh->mNumFaces; i++)
//	{
//		const aiFace& face = mesh->mFaces[i];
//		for (int j = 0; j < face.mNumIndices; j++)
//		{
//			meshData->indices.push_back(face.mIndices[j]);
//		}
//	}
//
//	
//
//	return meshData;
//}
//
//void processNode(aiNode* node, const aiScene* scene, Entity& parentEntity, ECSRegistry& registry, std::unordered_map<int, std::shared_ptr<MaterialData>> materials)
//{
//	for (int i = 0; i < node->mNumMeshes; i++)
//	{
//		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
//
//		Entity meshEntity = registry.CreateEntity();
//		Mesh meshComponent;
//		meshComponent.data = processMesh(mesh, scene);
//		registry.addComponent<Mesh>(meshEntity, meshComponent);
//
//		Material materialComponent;
//		materialComponent.data = materials[mesh->mMaterialIndex];
//		registry.addComponent<Material>(meshEntity, materialComponent);
//
//		Hierarchy hierarchy;
//		hierarchy.parent = parentEntity;
//		registry.addComponent<Hierarchy>(meshEntity, hierarchy);
//
//		auto& parentHierarchy = registry.getComponent<Hierarchy>(parentEntity);
//		parentHierarchy.children.push_back(meshEntity);
//	}
//
//	for (int i = 0; i < node->mNumChildren; i++)
//	{
//		processNode(node->mChildren[i], scene, parentEntity, registry, materials);
//	}
//}
//
//void loadMaterialTypeTextures(aiMaterial* mat, aiTextureType type, std::string typeName, std::shared_ptr<MaterialData> matData, const std::string& directory)
//{
//	for (int i = 0; i < mat->GetTextureCount(type); i++)
//	{
//		aiString str;
//		mat->GetTexture(type, i, &str);
//
//		auto texInfo = TextureInfo();
//		texInfo.path = directory + "/" + str.C_Str();
//
//		matData->textures[typeName + std::to_string(i)] = texInfo;
//	}
//}
//
//std::shared_ptr<MaterialData> processMaterial(aiMaterial* aiMat, const std::string& directory)
//{
//	auto mat = std::make_shared<MaterialData>();
//
//	mat->type = MaterialData::Type::Color;
//
//	aiColor3D color;
//	if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == aiReturn_SUCCESS)
//	{
//		mat->vectors["diffuseColor"] = glm::vec3(color.r, color.g, color.b);
//	}
//	if (aiMat->Get(AI_MATKEY_COLOR_SPECULAR, color) == aiReturn_SUCCESS)
//	{
//		mat->vectors["specularColor"] = glm::vec3(color.r, color.g, color.b);
//	}
//	ai_real shininess = 0.0f;
//	if (aiMat->Get(AI_MATKEY_SHININESS, shininess) == aiReturn_SUCCESS)
//	{
//		mat->scalars["shininess"] = static_cast<float>(shininess);
//	}
//
//	aiString texPath;
//	if (aiMat->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == aiReturn_SUCCESS)
//	{
//		std::cout << "Diffuse: " << texPath.C_Str() << std::endl;
//	}
//	if (aiMat->GetTexture(aiTextureType_SPECULAR, 0, &texPath) == aiReturn_SUCCESS)
//	{
//		std::cout << "Specular: " << texPath.C_Str() << std::endl;
//	}
//	
//
//	loadMaterialTypeTextures(aiMat, aiTextureType_DIFFUSE, "diffuse", mat, directory);
//	loadMaterialTypeTextures(aiMat, aiTextureType_SPECULAR, "specular", mat, directory);
//
//	/*auto loadTexture = [&](aiTextureType type) -> std::optional<TextureInfo>
//		{
//			if (aiMat->GetTextureCount(type) > 0)
//			{
//				aiString str;
//				aiMat->GetTexture(type, 0, &str);
//				return TextureInfo{ directory + "/" + str.C_Str() };
//			}
//			return std::nullopt;
//		};
//
//	auto tex = loadTexture(aiTextureType_DIFFUSE);
//	if (tex.has_value())
//	{
//		mat->textures["diffuse"] = tex.value();
//	}
//	tex = loadTexture(aiTextureType_SPECULAR);
//	if (tex.has_value())
//	{
//		mat->textures["specular"] = tex.value();
//	}
//	tex = loadTexture(aiTextureType_NORMALS);
//	if (tex.has_value())
//	{
//		mat->textures["normal"] = tex.value();
//	}*/
//
//	return mat;
//}
//
//std::unordered_map<int, std::shared_ptr<MaterialData>> loadMaterials(const aiScene* scene)
//{
//	std::unordered_map<int, std::shared_ptr<MaterialData>> materials;
//
//	for (int i = 0; i < scene->mNumMaterials; i++)
//	{
//		aiMaterial* aiMat = scene->mMaterials[i];
//		
//		std::cout << "Material" << i << ": " << aiMat->GetName().C_Str() << std::endl;
//		materials[i] = processMaterial(aiMat, "resources/backpack");
//	}
//
//	return materials;
//}
