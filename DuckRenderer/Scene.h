#pragma once

#include <vector>
#include <memory>
#include <embree4/rtcore.h>

#include "Intersection.h"
#include "SceneObject.h"
#include "RenderableObject.h"
#include "Camera.h"


class Scene
{
public:

	void AddObject(std::shared_ptr<SceneObject> object);

	inline void SetCamera(std::shared_ptr<Camera> camera) { this->camera = camera; }
	inline void InitCamera(int width, int height) { camera.get()->Initialize(width, height); }
	inline void InitCamera(int width, float aspectRatio) { camera.get()->Initialize(width, aspectRatio); }

	void InitScene(RTCDevice device);

	Intersection CastRay(glm::vec3 origin, glm::vec3 direction);
	Intersection CastRay(int x, int y);

	inline std::shared_ptr<Camera> GetCamera() { return camera; }

private:
	std::vector<std::shared_ptr<SceneObject>> objects;
	std::vector<std::shared_ptr<RenderableObject>> renderableObjects;
	std::vector<std::shared_ptr<SceneObject>> lightSources;

	std::shared_ptr<Camera> camera;

	RTCScene embreeScene;
};

