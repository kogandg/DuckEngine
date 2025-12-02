#include <embree/Sphere.h>

Sphere::Sphere(glm::vec3 center, float radius)
{
	this->center = center;
	this->radius = radius;
}

void Sphere::InitGeometry(RTCDevice device, RTCScene scene, glm::mat4 transform)
{
	RTCScene baseScene = rtcNewScene(device);

	/*RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_USER);

	rtcSetGeometryUserPrimitiveCount(geom, 1);
	rtcSetGeometryUserData(geom, this);
	rtcSetGeometryBoundsFunction(geom, boundsFunc, nullptr);
	rtcSetGeometryIntersectFunction(geom, intersectFunc);
	rtcSetGeometryOccludedFunction(geom, occludedFunc);*/

	RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_SPHERE_POINT);

	glm::vec4* vb = (glm::vec4*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT4, sizeof(glm::vec4), 1);
	vb[0] = glm::vec4(center, radius);

	rtcCommitGeometry(geom);
	rtcAttachGeometry(baseScene, geom);
	rtcReleaseGeometry(geom);
	rtcCommitScene(baseScene);

	RTCGeometry instance = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_INSTANCE);

	rtcSetGeometryInstancedScene(instance, baseScene);

	//transform = glm::translate(glm::mat4(1.0f), glm::vec3(1, 1, 0));
	rtcSetGeometryTransform(instance, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR, &transform[0]);

	rtcCommitGeometry(instance);
	geometryID = rtcAttachGeometry(scene, instance);
	rtcReleaseGeometry(instance);

	// Release the base scene after attaching it
	rtcReleaseScene(baseScene);
}

void Sphere::boundsFunc(const RTCBoundsFunctionArguments* args)
{
	const Sphere* spheres = static_cast<const Sphere*>(args->geometryUserPtr);
	unsigned int primID = args->primID;

	const Sphere& sphere = spheres[primID];

	args->bounds_o->lower_x = sphere.center.x - sphere.radius;
	args->bounds_o->lower_y = sphere.center.y - sphere.radius;
	args->bounds_o->lower_z = sphere.center.z - sphere.radius;
	args->bounds_o->upper_x = sphere.center.x + sphere.radius;
	args->bounds_o->upper_y = sphere.center.y + sphere.radius;
	args->bounds_o->upper_z = sphere.center.z + sphere.radius;
}

void Sphere::intersectFunc(const RTCIntersectFunctionNArguments* args)
{
	const int* valid = args->valid;
	void* geomUserPtr = args->geometryUserPtr;
	RTCRayHit* rayhit = (RTCRayHit*)args->rayhit;
	unsigned int primID = args->primID;

	const Sphere* spheres = static_cast<const Sphere*>(geomUserPtr);
	const Sphere& sphere = spheres[primID];

	for (int i = 0; i < RTC_MAX_INSTANCE_LEVEL_COUNT; i++) {
		if (!valid[i]) continue;

		glm::vec3 center = sphere.center;
		float radius = sphere.radius;

		glm::vec3 origin = glm::vec3(rayhit->ray.org_x, rayhit->ray.org_y, rayhit->ray.org_z);
		glm::vec3 dir = glm::vec3(rayhit->ray.dir_x, rayhit->ray.dir_y, rayhit->ray.dir_z);

		// Compute ray-sphere intersection
		glm::vec3 oc = origin - center;
		float a = glm::dot(dir, dir);
		float b = 2.0f * glm::dot(oc, dir);
		float c = glm::dot(oc, oc) - radius * radius;
		float discriminant = b * b - 4 * a * c;

		if (discriminant > 0)
		{
			float t = (-b - sqrt(discriminant)) / (2.0f * a);
			if (t < rayhit->ray.tfar && t > rayhit->ray.tnear)
			{
				rayhit->ray.tfar = t;
				rayhit->hit.geomID = sphere.geometryID;
				rayhit->hit.primID = primID;
				rayhit->hit.u = 0.0f; // Spherical UV coordinates
				rayhit->hit.v = 0.0f; // can be computed if needed
				rayhit->hit.Ng_x = (origin.x + t * dir.x - center.x) / radius;
				rayhit->hit.Ng_y = (origin.y + t * dir.y - center.y) / radius;
				rayhit->hit.Ng_z = (origin.z + t * dir.z - center.z) / radius;
			}
		}
	}
}

void Sphere::occludedFunc(const RTCOccludedFunctionNArguments* args)
{
	const int* valid = args->valid;
	void* geomUserPtr = args->geometryUserPtr;
	RTCRay* rays = (RTCRay*)args->ray;
	unsigned int primID = args->primID;

	const Sphere* spheres = static_cast<const Sphere*>(geomUserPtr);
	const Sphere& sphere = spheres[primID];

	for (int i = 0; i < RTC_MAX_INSTANCE_LEVEL_COUNT; i++) {
		if (!valid[i]) continue;

		glm::vec3 center = sphere.center;
		float radius = sphere.radius;

		auto ray = rays[i];
		glm::vec3 origin = glm::vec3(ray.org_x, ray.org_y, ray.org_z);
		glm::vec3 dir = glm::vec3(ray.dir_x, ray.dir_y, ray.dir_z);

		// Compute ray-sphere intersection
		glm::vec3 oc = origin - center;
		float a = glm::dot(dir, dir);
		float b = 2.0f * glm::dot(oc, dir);
		float c = glm::dot(oc, oc) - radius * radius;
		float discriminant = b * b - 4 * a * c;

		if (discriminant > 0)
		{
			float t1 = (-b - sqrt(discriminant)) / (2.0f * a);
			float t2 = (-b + sqrt(discriminant)) / (2.0f * a);

			// Check if the intersection is within the ray's bounds
			if ((t1 >= rays[i].tnear && t1 <= rays[i].tfar) || (t2 >= rays[i].tnear && t2 <= rays[i].tfar)) 
			{
				rays[i].tfar = -INFINITY; // Mark ray as occluded
				break; // No need to check further for this ray
			}
		}
	}
}
