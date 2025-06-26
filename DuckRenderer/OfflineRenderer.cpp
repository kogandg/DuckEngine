#include "OfflineRenderer.h"
#include "ThreadPool.h"

void embreeErrorFunction(void* userPtr, enum RTCError error, const char* str)
{
	printf("Embree error %d: %s\n", error, str);
}


OfflineRenderer::OfflineRenderer(std::shared_ptr<Integrator> integrator, std::shared_ptr<Scene> scene)
{
	this->integrator = integrator;
	this->scene = scene;
}

void OfflineRenderer::Initialize()
{
	//init device
	embreeDevice = rtcNewDevice(NULL);

	if (!embreeDevice)
	{
		printf("error %d: cannot create device\n", rtcGetDeviceError(NULL));
	}

	rtcSetDeviceErrorFunction(embreeDevice, embreeErrorFunction, NULL);

	//init scene
	scene.get()->InitScene(embreeDevice);
}

void OfflineRenderer::Render(std::shared_ptr<RenderTarget> target)
{
	scene.get()->InitCamera(target.get()->GetWidth(), target.get()->GetHeight());

	progressState = ProgressState::Running;

	//int tileSize = 16;

	genTiles(target, 64);

	ThreadPool pool(8);

	for (int i = 0; i < tiles.size(); i++)
	{
		auto tile = tiles[i];
		pool.Enqueue([tile, target, this]() {OfflineRenderer::renderTile(tile, target); });
	}

	bool allTilesDone = false;
	while (!allTilesDone)
	{
		allTilesDone = true;
		for (auto tile : tiles)
		{
			if (tile->state != TileState::Complete)
			{
				allTilesDone = false;
			}
		}

		if (progressState == ProgressState::Canceled)
		{
			pool.Shutdown();
			break;
		}
	}

	/*
	for (int y = 0; y < target.get()->GetHeight(); y++)
	{
		for (int x = 0; x < target.get()->GetWidth(); x++)
		{
			currentPixel.x = x;
			currentPixel.y = y;
			
			std::unique_lock<std::mutex> lock(pauseMutex);
			pauseCV.wait(lock, [this] { return progressState != Paused || progressState == ProgressState::Canceled; });
			if (progressState == ProgressState::Canceled)
			{
				std::cout << "Done canceling" << std::endl;
				return;
			}


			Intersection intersection = scene.get()->CastRay(x, y);
			glm::vec3 color = integrator.get()->RayColor(scene, intersection, 0);// rayColor(intersection);

			target.get()->WritePixel(color, x, y);
		}
	}
	*/

	std::cout << "Done Rendering!" << std::endl;

	pool.Shutdown();

	progressState = ProgressState::Done;
}

void OfflineRenderer::TogglePaused()
{
	if (progressState == Paused)
	{
		progressState = Running;
		pauseCV.notify_all();
	}
	else
	{
		progressState = Paused;
	}
}

void OfflineRenderer::genTiles(std::shared_ptr<RenderTarget> target, int tileSize)
{
	tiles = std::vector<Tile*>();
	for (int y = 0; y < target.get()->GetHeight(); y += tileSize)
	{
		for (int x = 0; x < target.get()->GetWidth(); x += tileSize)
		{
			Tile* tile = new Tile { x, y, std::min(tileSize, target.get()->GetWidth() - x), std::min(tileSize, target.get()->GetHeight() - y), TileState::Waiting };

			tiles.push_back(tile);
		}
	}
}

void OfflineRenderer::renderTile(Tile* tile, std::shared_ptr<RenderTarget> target)
{
	tile->state = TileState::InProgress;
	for (int y = tile->y; y < tile->y + tile->height; y++)
	{
		for (int x = tile->x; x < tile->x + tile->width; x++)
		{
			/*std::unique_lock<std::mutex> lock(pauseMutex);
			pauseCV.wait(lock, [this] { return progressState != Paused || progressState == ProgressState::Canceled; });
			*/
			/*if (progressState == ProgressState::Canceled)
			{
				std::cout << "Done canceling tile " << tile->x << ", " << tile->y << std::endl;
				return;
			}*/


			Intersection intersection = scene.get()->CastRay(x, y);
			glm::vec3 color = integrator.get()->RayColor(scene, intersection, 0);

			target.get()->WritePixel(color, x, y);
		}
	}

	tile->state = TileState::Complete;
	std::cout << "Done rendering tile " << tile->x << ", " << tile->y << std::endl;
}
