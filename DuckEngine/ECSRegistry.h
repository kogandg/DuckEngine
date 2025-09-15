#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

#include <functional>
#include <variant>
#include <memory>
#include <string>
#include "InputManager.h"

namespace ECS
{
	using Entity = unsigned int;
	constexpr Entity INVALID_ENTITY = 0;

	struct IComponent
	{
		virtual ~IComponent() = default;
	};

	template<typename T>
	struct ComponentDependencies
	{
		using type = std::tuple<>;
	};

	class ECSRegistry
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
			T component = T(std::forward<Args>(args)...);
			return addComponent<T>(e, component);
		}

		template<typename T, typename = std::enable_if_t<std::is_base_of_v<IComponent, T>>>
		T& addComponent(Entity e, T component)
		{
			addDependencies(e, typename ComponentDependencies<T>::type{});//ensure deps exist

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

		template<typename... Deps>
		void addDependencies(Entity e, std::tuple<Deps...>)
		{
			((hasComponent<Deps>(e) ? void() : (void)addComponent<Deps>(e, Deps{})), ...);
		}
	};

	//components
	struct Transform : public IComponent
	{
		glm::vec3 position = glm::vec3(0.0f);
		glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f); // maybe should be vec3 rotation rather than quaternion
		glm::vec3 scale = glm::vec3(1.0f);

		glm::mat4 local = glm::mat4(1.0f);
		glm::mat4 world = glm::mat4(1.0f);

		bool dirty = true;
	};

	struct MeshData
	{
		std::vector<float> verts;
	};

	struct Mesh : public IComponent
	{
		std::shared_ptr<MeshData> data;
	};
	template<>
	struct ComponentDependencies<Mesh>
	{
		using type = std::tuple<Transform>;
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
		enum class Type
		{
			Color,
			Phong
		};
		Type type;

		std::variant<ColorData, PhongData> data;
	};

	struct Material : public IComponent
	{
		std::shared_ptr<MaterialData> data;
	};

	struct PointData
	{
		float constant;
		float linear;
		float quadratic;
	};

	struct SpotData
	{
		glm::vec3 position;
		glm::vec3 direction;
		float cutOff;
		float outerCutOff;

		float constant;
		float linear;
		float quadratic;
	};


	struct Light : public IComponent
	{
		enum class Type
		{
			Directional,
			Point,
			Spot
		};

		Type type;

		glm::vec3 ambient;
		glm::vec3 diffuse;
		glm::vec3 specular;

		std::variant<PointData, SpotData> data;
	};
	template<>
	struct ComponentDependencies<Light>
	{
		using type = std::tuple<Transform>;
	};


	struct Hierarchy : public IComponent
	{
		Entity parent = INVALID_ENTITY;
		std::vector<Entity> children;
	};

	struct Camera : public IComponent
	{
		float fovY = 45.0f;
		float aspectRatio = 8.0f / 6.0f;
		float nearPlane = 0.1f;
		float farPlane = 1000.0f;


		//cached info
		//glm::vec3 forward;
		//glm::vec3 right;
		//glm::vec3 up;

		glm::mat4 view = glm::mat4(1.0f);
		glm::mat4 projection = glm::mat4(1.0f);
		//maybe keep view proj

		//bool primary = false;
	};
	template<>
	struct ComponentDependencies<Camera>
	{
		using type = std::tuple<Transform>;
	};

	struct TransformSystem
	{
		void update(ECSRegistry& registry)
		{
			for (auto& [entity, transform] : registry.view<Transform>())
			{
				if (registry.hasComponent<Hierarchy>(entity))
				{
					updateTransformsHierarchy(registry, transform, registry.getComponent<Hierarchy>(entity));
				}
				else
				{
					updateTransforms(transform);
				}
			}
		}

	private:

		void updateTransforms(Transform& t)
		{
			if (!t.dirty) return;

			glm::mat4 T = glm::translate(glm::mat4(1.0f), t.position);
			glm::mat4 R = glm::mat4_cast(t.rotation);
			glm::mat4 S = glm::scale(glm::mat4(1.0f), t.scale);
			t.local = T * R * S;

			t.world = t.local;
			t.dirty = false;
		}

		void updateTransformsHierarchy(ECSRegistry& registry, Transform& t, Hierarchy& h)
		{
			updateTransforms(t);
			if (h.parent != INVALID_ENTITY)
			{
				auto& parentTransform = registry.getComponent<Transform>(h.parent);
				t.world = parentTransform.world * t.local;
			}

			for (Entity child : h.children)
			{
				updateTransformsHierarchy(registry, registry.getComponent<Transform>(child), registry.getComponent<Hierarchy>(child));
			}
		}
	};

	struct CameraSystem
	{
		void update(ECSRegistry& registry)
		{
			for (auto& [entity, camera] : registry.view<Camera>())
			{
				auto& transform = registry.getComponent<Transform>(entity);
				camera.projection = glm::perspective(glm::radians(camera.fovY), camera.aspectRatio, camera.nearPlane, camera.farPlane);
				camera.view = glm::inverse(transform.world);
			}
		}
	};

	struct CameraController
	{
		float pitch = 0.0f;
		float yaw = 0.0f;
		float movementSpeed = 2.5f;
		float mouseSensitivity = 0.1f;

		void init(Transform& transform);
		void update(Transform& transform, InputManager& input, float dt);

	private:
		void updateRotation(Transform& transform);
	};

	struct DebugSystem
	{
		void update(ECSRegistry& registry);

	private:
		void drawEntityNode(ECSRegistry& registry, Entity e);
		void drawTransform(Transform& t);
		void drawMaterial(Material& t);
		void drawLight(Light& l);
		void drawCamera(Camera& c);
	};
};

