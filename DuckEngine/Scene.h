#pragma once

#include <vector>
#include <memory>
#include <embree4/rtcore.h>
#include <iostream>


#include "Intersection.h"
#include "SceneObject.h"
#include "RenderableObject.h"
#include "Camera.h"
#include "EmbreeGeometry.h"
#include <unordered_map>
#include "Light.h"
#include "PointLight.h"
#include "RenderableObject.h"
#include "DirectionalLight.h"


#include <functional>
#include <variant>

//ecs scene
namespace ECS
{
	using Entity = unsigned int;
	constexpr Entity INVALID_ENTITY = 0;

	struct IComponent
	{
		virtual ~IComponent() = default;
	};

	class ECS
	{
	public:
		Entity CreateEntity()
		{
			Entity e = nextEntity;
			nextEntity = Entity{ nextEntity + 1 };
			entities.push_back(e);
			return e;
		}

		template<typename T, typename... Args, typename = std::enable_if_t<std::is_base_of_v<IComponent, T>>>
		T& addComponent(Entity e, Args&&... args)
		{
			auto& storage = getComponentStorage<T>();
			return storage[e] = T(std::forward<Args>(args)...);
		}

		template<typename T, typename = std::enable_if_t<std::is_base_of_v<IComponent, T>>>
		T& addComponent(Entity e, T component)
		{
			auto& storage = getComponentStorage<T>();
			return storage[e] = component;
		}

		template<typename T, typename = std::enable_if_t<std::is_base_of_v<IComponent, T>>>
		bool hasComponent(Entity e)
		{
			auto& storage = getComponentStorage<T>();
			return storage.find(e) != storage.end();
		}

		template<typename T, typename = std::enable_if_t<std::is_base_of_v<IComponent, T>>>
		T& getComponent(Entity e)
		{
			auto& storage = getComponentStorage<T>();
			return storage.at(e);
		}

		template<typename T, typename = std::enable_if_t<std::is_base_of_v<IComponent, T>>>
		void removeComponent(Entity e)
		{
			auto& storage = getComponentStorage<T>();
			storage.erase(e);
		}

		template<typename T, typename = std::enable_if_t<std::is_base_of_v<IComponent, T>>>
		std::unordered_map<Entity, T>& view()
		{
			return getComponentStorage<T>();
		}

		std::vector<Entity>& getEntities()
		{
			return entities;
		}



	private:
		Entity nextEntity = Entity{ 1 };
		std::vector<Entity> entities;
		std::vector<std::function<void(Entity)>> removeFuncs;

		template<typename T>
		std::unordered_map<Entity, T>& getComponentStorage()
		{
			static std::unordered_map<Entity, T> storage;
			static bool registered = false;
			if (!registered)
			{
				storage = std::unordered_map<Entity, T>();
				removeFuncs.push_back([](Entity e) {storage.erase(e); });
				registered = true;
			}
			return storage;
		}
	};

	//components
	struct Transform : public IComponent
	{
		glm::mat4 local = glm::mat4(1.0f);
		glm::mat4 world = glm::mat4(1.0f);
	};

	struct MeshData
	{
		std::vector<float> verts;
	};

	struct Mesh : public IComponent
	{
		std::shared_ptr<MeshData> data;
	};

	enum class MaterialType
	{
		Color,
		Phong
	};

	struct ColorData
	{
		glm::vec3 color;
	};

	struct PhongData
	{
		std::string diffuseTexturePath;
		std::string specularTexturePath;
		float shininess;
	};

	struct MaterialData
	{
		MaterialType materialType;
		std::variant<ColorData, PhongData> data;
	};

	struct Material : public IComponent
	{
		std::shared_ptr<MaterialData> data;
	};

};



// old scene

class Scene
{
public:
	Scene();

	void AddObject(std::shared_ptr<SceneObject> object);

	inline void SetCamera(std::shared_ptr<Camera> camera) { this->camera = camera; }
	inline void InitCamera(int width, int height) { camera.get()->Initialize(width, height); }
	inline void InitCamera(int width, float aspectRatio) { camera.get()->Initialize(width, aspectRatio); }

	void InitScene(RTCDevice device);

	Intersection CastRay(glm::vec3 origin, glm::vec3 direction);
	Intersection CastRay(int x, int y);

	inline std::shared_ptr<Camera> GetCamera() { return camera; }

	std::shared_ptr<RenderableObject> GetRenderable(unsigned int geomID);

	inline std::vector<std::shared_ptr<Light>> GetLights() { return lightSources; }
	inline std::vector<std::shared_ptr<PointLight>> GetPointLights() { return pointLights; }
	inline std::vector<std::shared_ptr<DirectionalLight>> GetDirectionalLights() { return directionalLights; }

private:
	std::vector<std::shared_ptr<SceneObject>> objects;
	std::vector<std::shared_ptr<RenderableObject>> renderableObjects;

	std::vector<std::shared_ptr<Light>> lightSources;
	std::vector<std::shared_ptr<PointLight>> pointLights;
	std::vector<std::shared_ptr<DirectionalLight>> directionalLights;
	//directionalLights

	std::shared_ptr<Camera> camera;

	RTCScene embreeScene;
	//RTCScene embreeSceneInstance;
};

