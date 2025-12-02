#include <embree/scene/Scene.h>

Scene::Scene()
{
	embreeScene = NULL;
}

void Scene::AddObject(std::shared_ptr<SceneObject> object)
{
	objects.push_back(object);
	//add check for if its a light

	if (auto light = std::dynamic_pointer_cast<Light>(object))
	{
		lightSources.push_back(light);

		if (auto pointLight = std::dynamic_pointer_cast<PointLight>(light))
		{
			pointLights.push_back(pointLight);
		}

		if (auto directionalLight = std::dynamic_pointer_cast<DirectionalLight>(light))
		{
			directionalLights.push_back(directionalLight);
		}
	}

	if (auto renderable = std::dynamic_pointer_cast<RenderableObject>(object))
	{
		renderableObjects.push_back(renderable);

		//add checks for emmision
		/*if (renderable->material.get()->emission.length() > 0.0f)
		{
			lightSources.push_back(object);
		}*/
	}
}

void Scene::InitScene(RTCDevice device)
{
	embreeScene = rtcNewScene(device);

	for (auto renderable : renderableObjects)
	{
		static_cast<EmbreeGeometry*>(renderable.get()->geometry.get())->InitGeometry(device, embreeScene, renderable.get()->GetTransform());
	}

	rtcCommitScene(embreeScene);
}

Intersection Scene::CastRay(glm::vec3 origin, glm::vec3 direction)
{

	direction = glm::normalize(direction);
	/*
   * The ray hit structure holds both the ray and the hit.
   * The user must initialize it properly -- see API documentation
   * for rtcIntersect1() for details.
   */
	struct RTCRayHit rayhit;
	rayhit.ray.org_x = origin.x;
	rayhit.ray.org_y = origin.y;
	rayhit.ray.org_z = origin.z;
	rayhit.ray.dir_x = direction.x;
	rayhit.ray.dir_y = direction.y;
	rayhit.ray.dir_z = direction.z;
	rayhit.ray.tnear = 0.001f;
	rayhit.ray.tfar = std::numeric_limits<float>::infinity();
	rayhit.ray.mask = -1;
	rayhit.ray.flags = 0;
	rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
	rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;

	/*
	 * There are multiple variants of rtcIntersect. This one
	 * intersects a single ray with the scene.
	 */
	rtcIntersect1(embreeScene, &rayhit);

	Intersection intersection;
	intersection.origin = origin;
	intersection.direction = direction;
	intersection.t = rayhit.ray.tfar;
	intersection.id = rayhit.hit.instID[0];//geomid is 0 since its from the base scene, instID is the actual id since its in the used scene. [0] is there can be multiple instances
	
	if (intersection.id != RTC_INVALID_GEOMETRY_ID)
	{
		auto hitObject = GetRenderable(intersection.id).get();
		//auto transform = hitObject->GetTransform();

		intersection.hit = origin + (intersection.t * direction);
		//intersection.hit = transform * glm::vec4(intersection.hit, 0.0f);

		auto embreeNorm = glm::vec3(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z);
		intersection.normal = hitObject->GetInverseTransposeTransform() * glm::vec4(embreeNorm, 0.0f);
		intersection.normal = glm::normalize(intersection.normal);
	}
	return intersection;
}

Intersection Scene::CastRay(int x, int y)
{
	auto pair = camera.get()->GetRay(x, y);

	return CastRay(pair.first, pair.second);
}

std::shared_ptr<RenderableObject> Scene::GetRenderable(unsigned int geomID)
{
	for (auto renderable : renderableObjects)
	{
		if (renderable.get()->geometry.get()->GetGeometryID() == geomID)
		{
			return renderable;
		}
	}
	return nullptr;
}
