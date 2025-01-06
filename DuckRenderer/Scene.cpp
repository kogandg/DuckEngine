#include "Scene.h"
#include "RenderableObject.h"

void Scene::AddObject(std::shared_ptr<SceneObject> object)
{
	objects.push_back(object);
	//add check for if its a light

	if (auto renderable = std::static_pointer_cast<RenderableObject>(object))
	{
		renderableObjects.push_back(std::make_shared<RenderableObject>(*renderable));

		if (renderable->material.get()->emission.length() > 0.0f)
		{
			lightSources.push_back(object);
		}
	}
}

void Scene::InitScene(RTCDevice device)
{
	embreeScene = rtcNewScene(device);

	for (auto renderable : renderableObjects)
	{
		renderable.get()->geometry.get()->InitGeometry(device, embreeScene);
	}

	rtcCommitScene(embreeScene);
}

Intersection Scene::CastRay(glm::vec3 origin, glm::vec3 direction)
{
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
	rayhit.ray.tnear = 0;
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

	//printf("%f, %f, %f: ", origin.x, origin.y, origin.z);
	//if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
	//{
	//	/* Note how geomID and primID identify the geometry we just hit.
	//	 * We could use them here to interpolate geometry information,
	//	 * compute shading, etc.
	//	 * Since there is only a single triangle in this scene, we will
	//	 * get geomID=0 / primID=0 for all hits.
	//	 * There is also instID, used for instancing. See
	//	 * the instancing tutorials for more information */
	//	printf("Found intersection on geometry %d, primitive %d at tfar=%f\n",
	//		rayhit.hit.geomID,
	//		rayhit.hit.primID,
	//		rayhit.ray.tfar);
	//}
	//else
	//	printf("Did not find any intersection.\n");

	Intersection intersection;
	intersection.origin = origin;
	intersection.direction = direction;
	intersection.t = rayhit.ray.tfar;
	intersection.id = rayhit.hit.geomID;

	return intersection;
}

Intersection Scene::CastRay(int x, int y)
{
	auto pair = camera.get()->GetRay(x, y);

	return CastRay(pair.first, pair.second);
}
