#include "OfflineRenderer.h"

void embreeErrorFunction(void* userPtr, enum RTCError error, const char* str)
{
	printf("Embree error %d: %s\n", error, str);
}

void OfflineRenderer::Render(int width, int height)
{
	image = new unsigned char[width * height * 3];
	for (int i = 0; i < width * height * 3; i++)
	{
		image[i] = 0;
	}

	running = true;
	started = true;

	/*for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			int index = (y * width + x) * 3; // 3 channels (RGB) per pixel
			//bool isWhite = ((x / 16) % 2 == (y / 16) % 2); // 16-pixel checkerboard pattern
			//bufferMutex.lock();

			auto r = double(x) / (width - 1);
			auto g = double(y) / (height - 1);
			auto b = 0.0;

			image[index + 0] = int(255.99 * r);  // Red
			image[index + 1] = int(255.99 * g);  // Green
			image[index + 2] = int(255.99 * b);  // Blue
			//bufferMutex.unlock();

			if (cancel)
			{
				return;
			}
		}
		//std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}*/

	/* Initialization. All of this may fail, but we will be notified by
   * our errorFunction. */
	RTCDevice device = initializeDevice();
	scene = initializeScene(device);

	/* This will hit the triangle at t=1. */
	//castRay(0.33f, 0.33f, -1, 0, 0, 1);
	//castRay(glm::vec3(0.33f, 0.33f, -1), glm::vec3(0, 0, 1));

	/* This will not hit anything. */
	//castRay(1.00f, 1.00f, -1, 0, 0, 1);
	//castRay(glm::vec3(1, 1, -1), glm::vec3(0, 0, 1));

	float aspectRatio = (float)width / (float)height;

	float focalLength = 1;
	float viewportHeight = 2;
	float viewportWidth = viewportHeight * aspectRatio;

	glm::vec3 cameraCenter = glm::vec3(0, 0, 0);

	glm::vec3 viewportU = glm::vec3(viewportWidth, 0, 0);
	glm::vec3 viewportV = glm::vec3(0, -viewportHeight, 0);

	glm::vec3 pixelDeltaU = viewportU / (float)width;
	glm::vec3 pixelDeltaV = viewportV / (float)height;

	glm::vec3 viewportUpperLeft = cameraCenter - glm::vec3(0, 0, focalLength) - (viewportU / 2.0f) - (viewportV / 2.0f);
	glm::vec3 pixel0Location = viewportUpperLeft + ((pixelDeltaU + pixelDeltaV) * 0.5f);


	for (int y = 0; y < height; y++)
	{
		for (int x = 0; x < width; x++)
		{
			glm::vec3 pixelCenter = pixel0Location + (pixelDeltaU * (float)x) + (pixelDeltaV * (float)y);
			glm::vec3 rayDirection = pixelCenter - cameraCenter;

			Intersection intersection = castRay(cameraCenter, rayDirection);
			glm::vec3 color = rayColor(intersection);

			writePixel(color, x, y, width);
		}
	}

	/* Though not strictly necessary in this example, you should
	 * always make sure to release resources allocated through Embree. */
	rtcReleaseScene(scene);
	rtcReleaseDevice(device);

	std::cout << "Done Rendering!" << std::endl;

	running = false;
}

RTCDevice OfflineRenderer::initializeDevice()
{
	RTCDevice device = rtcNewDevice(NULL);

	if (!device)
	{
		printf("error %d: cannot create device\n", rtcGetDeviceError(NULL));
	}

	rtcSetDeviceErrorFunction(device, embreeErrorFunction, NULL);
	return device;
}

RTCScene OfflineRenderer::initializeScene(RTCDevice device)
{
	RTCScene scene = rtcNewScene(device);

	/*
	 * Create a triangle mesh geometry, and initialize a single triangle.
	 * You can look up geometry types in the API documentation to
	 * find out which type expects which buffers.
	 *
	 * We create buffers directly on the device, but you can also use
	 * shared buffers. For shared buffers, special care must be taken
	 * to ensure proper alignment and padding. This is described in
	 * more detail in the API documentation.
	 */
	RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
	float* vertices = (float*)rtcSetNewGeometryBuffer(geom,
		RTC_BUFFER_TYPE_VERTEX,
		0,
		RTC_FORMAT_FLOAT3,
		3 * sizeof(float),
		3);

	unsigned* indices = (unsigned*)rtcSetNewGeometryBuffer(geom,
		RTC_BUFFER_TYPE_INDEX,
		0,
		RTC_FORMAT_UINT3,
		3 * sizeof(unsigned),
		1);

	if (vertices && indices)
	{
		vertices[0] = 0.f; vertices[1] = 0.f; vertices[2] = -1.0f;
		vertices[3] = 1.f; vertices[4] = 0.f; vertices[5] = -1.0f;
		vertices[6] = 0.f; vertices[7] = 1.f; vertices[8] = -1.0f;

		indices[0] = 0; indices[1] = 1; indices[2] = 2;
	}

	/*
	 * You must commit geometry objects when you are done setting them up,
	 * or you will not get any intersections.
	 */
	rtcCommitGeometry(geom);

	/*
	 * In rtcAttachGeometry(...), the scene takes ownership of the geom
	 * by increasing its reference count. This means that we don't have
	 * to hold on to the geom handle, and may release it. The geom object
	 * will be released automatically when the scene is destroyed.
	 *
	 * rtcAttachGeometry() returns a geometry ID. We could use this to
	 * identify intersected objects later on.
	 */
	rtcAttachGeometry(scene, geom);
	rtcReleaseGeometry(geom);

	/*
	 * Like geometry objects, scenes must be committed. This lets
	 * Embree know that it may start building an acceleration structure.
	 */
	rtcCommitScene(scene);

	return scene;
}

Intersection OfflineRenderer::castRay(glm::vec3 origin, glm::vec3 direction)
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
	rtcIntersect1(scene, &rayhit);

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

glm::vec3 OfflineRenderer::rayColor(Intersection intersection)
{
	if (intersection.id == RTC_INVALID_GEOMETRY_ID)
	{
		glm::vec3 unitDirection = glm::normalize(intersection.direction);
		float a = 0.5 * (unitDirection.y + 1.0);
		return (1.0f-a)*glm::vec3(1, 1, 1) + a*glm::vec3(0.5, 0.7, 1);
	}
	
	return glm::vec3(1, 0, 0);
}

void OfflineRenderer::writePixel(glm::vec3 color, int x, int y, int width)
{
	int index = (y * width + x) * 3; // 3 channels (RGB) per pixel

	image[index + 0] = int(255.99 * color.r);  // Red
	image[index + 1] = int(255.99 * color.g);  // Green
	image[index + 2] = int(255.99 * color.b);  // Blue
}

