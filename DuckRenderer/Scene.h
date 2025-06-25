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

