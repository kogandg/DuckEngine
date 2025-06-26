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

	genTiles(target, 8);

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
			pool.Cancel();
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


	if (progressState == ProgressState::Canceled)
	{
		bool allTilesDoneCanceling = false;
		while (!allTilesDoneCanceling)
		{
			allTilesDoneCanceling = true;
			for (auto tile : tiles)
			{
				if (tile->state == TileState::InProgress)
				{
					allTilesDoneCanceling = false;
				}
			}
		}
		std::println("Done canceling");
		return;
	}

	std::println("Done rendering!");
	progressState = ProgressState::Done;
}

void OfflineRenderer::TogglePaused()
{
	switch (progressState)
	{
	case Running:
		progressState = Paused;
		break;
	case Paused:
		progressState = Running;
		break;
	default://Not sure how to cancel pausing in other states, maybe nothing needs to change
		break;
	}
	pauseCV.notify_all();
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

	{
		std::unique_lock<std::mutex> lock(pauseMutex);
		pauseCV.wait(lock, [&] {return progressState != ProgressState::Paused || progressState == ProgressState::Canceled; });
	
		if (progressState == ProgressState::Canceled)
		{
			std::println("Canceled tile {0}, {1}", tile->x, tile->y);
			return;
		}
	}

	tile->state = TileState::InProgress;
	for (int y = tile->y; y < tile->y + tile->height; y++)
	{
		for (int x = tile->x; x < tile->x + tile->width; x++)
		{
			Intersection intersection = scene.get()->CastRay(x, y);
			glm::vec3 color = integrator.get()->RayColor(scene, intersection, 0);

			target.get()->WritePixel(color, x, y);
			pixelsRendered++;
		}
	}

	tile->state = TileState::Complete;
	std::println("Done rendering tile {0}, {1}", tile->x, tile->y);
}
