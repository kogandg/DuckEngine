#include "OfflineRenderer.h"

void embreeErrorFunction(void* userPtr, enum RTCError error, const char* str)
{
	printf("Embree error %d: %s\n", error, str);
}

OfflineRenderer::OfflineRenderer(int maxDepth)
{
	this->maxDepth = maxDepth;
}

void OfflineRenderer::Render(int imageWidth, int imageHeight)
{
	/* Initialization. All of this may fail, but we will be notified by
   * our errorFunction. */
	RTCDevice device = initializeDevice();
	//scene = initializeScene(device);
	initializeScene(device);

	scene.InitCamera(imageWidth, imageHeight);


	image = new unsigned char[imageWidth * imageHeight * 3];
	for (int i = 0; i < imageWidth * imageHeight * 3; i++)
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

	

	/* This will hit the triangle at t=1. */
	//castRay(0.33f, 0.33f, -1, 0, 0, 1);
	//castRay(glm::vec3(0.33f, 0.33f, -1), glm::vec3(0, 0, 1));

	/* This will not hit anything. */
	//castRay(1.00f, 1.00f, -1, 0, 0, 1);
	//castRay(glm::vec3(1, 1, -1), glm::vec3(0, 0, 1));

	for (int y = 0; y < imageHeight; y++)
	{
		for (int x = 0; x < imageWidth; x++)
		{
			if (cancel)
			{
				std::cout << "Done canceling" << std::endl;
				return;
			}

			Intersection intersection = scene.CastRay(x, y);
			glm::vec3 color = rayColor(intersection);

			writePixel(color, x, y, imageWidth);
		}

		//std::this_thread::sleep_for(std::chrono::milliseconds(1000));
	}

	/* Though not strictly necessary in this example, you should
	 * always make sure to release resources allocated through Embree. */
	//rtcReleaseScene(scene);
	//rtcReleaseDevice(device);

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

void OfflineRenderer::initializeScene(RTCDevice device)
{
	scene.SetCamera(std::make_shared<Camera>(Camera(glm::vec3(0, 0, 0), glm::vec3(0, 0, -1), glm::vec3(0, 1, 0))));
	
	auto triGeom = std::make_shared<Triangle>(Triangle(glm::vec3(0, 0, -1), glm::vec3(1, 0, -1), glm::vec3(0, 1, -1)));
	
	Material material;
	material.diffuse = glm::vec3(1, 0, 0);
	material.emission = glm::vec3(0.0f);
	material.roughness = 0;
	material.shininess = 0;
	material.specular = glm::vec3(1, 0, 0);
	auto mat = std::make_shared<Material>(material);

	tri = std::make_shared<RenderableObject>(RenderableObject("", triGeom, mat));

	scene.AddObject(tri);

	scene.InitScene(device);

	//tri.InitGeometry(device, scene);

	///*
	// * Like geometry objects, scenes must be committed. This lets
	// * Embree know that it may start building an acceleration structure.
	// */
	//rtcCommitScene(scene);

	//return scene;
}

//Intersection OfflineRenderer::castRay(glm::vec3 origin, glm::vec3 direction)
//{
//	/*
//   * The ray hit structure holds both the ray and the hit.
//   * The user must initialize it properly -- see API documentation
//   * for rtcIntersect1() for details.
//   */
//	struct RTCRayHit rayhit;
//	rayhit.ray.org_x = origin.x;
//	rayhit.ray.org_y = origin.y;
//	rayhit.ray.org_z = origin.z;
//	rayhit.ray.dir_x = direction.x;
//	rayhit.ray.dir_y = direction.y;
//	rayhit.ray.dir_z = direction.z;
//	rayhit.ray.tnear = 0;
//	rayhit.ray.tfar = std::numeric_limits<float>::infinity();
//	rayhit.ray.mask = -1;
//	rayhit.ray.flags = 0;
//	rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
//	rayhit.hit.instID[0] = RTC_INVALID_GEOMETRY_ID;
//
//	/*
//	 * There are multiple variants of rtcIntersect. This one
//	 * intersects a single ray with the scene.
//	 */
//	rtcIntersect1(scene, &rayhit);
//
//	//printf("%f, %f, %f: ", origin.x, origin.y, origin.z);
//	//if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID)
//	//{
//	//	/* Note how geomID and primID identify the geometry we just hit.
//	//	 * We could use them here to interpolate geometry information,
//	//	 * compute shading, etc.
//	//	 * Since there is only a single triangle in this scene, we will
//	//	 * get geomID=0 / primID=0 for all hits.
//	//	 * There is also instID, used for instancing. See
//	//	 * the instancing tutorials for more information */
//	//	printf("Found intersection on geometry %d, primitive %d at tfar=%f\n",
//	//		rayhit.hit.geomID,
//	//		rayhit.hit.primID,
//	//		rayhit.ray.tfar);
//	//}
//	//else
//	//	printf("Did not find any intersection.\n");
//
//	Intersection intersection;
//	intersection.origin = origin;
//	intersection.direction = direction;
//	intersection.t = rayhit.ray.tfar;
//	intersection.id = rayhit.hit.geomID;
//
//	return intersection;
//}

glm::vec3 OfflineRenderer::rayColor(Intersection intersection)
{
	if (intersection.id == RTC_INVALID_GEOMETRY_ID)
	{
		glm::vec3 unitDirection = glm::normalize(intersection.direction);
		float a = 0.5 * (unitDirection.y + 1.0);
		return (1.0f-a)*glm::vec3(1, 1, 1) + a*glm::vec3(0.5, 0.7, 1);
	}
	
	if (intersection.id == tri.get()->geometry.get()->GetGeometryID())
	{
		return tri.get()->material.get()->diffuse;
	}

	return glm::vec3(0, 1, 0);
}

void OfflineRenderer::writePixel(glm::vec3 color, int x, int y, int width)
{
	int index = (y * width + x) * 3; // 3 channels (RGB) per pixel

	image[index + 0] = int(255.99 * color.r);  // Red
	image[index + 1] = int(255.99 * color.g);  // Green
	image[index + 2] = int(255.99 * color.b);  // Blue
}

