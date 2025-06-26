#pragma once

#include <iostream>

#include <embree4/rtcore.h>
#include <glm/glm.hpp>

#include "Intersection.h"
#include "Triangle.h"
#include "Sphere.h"
#include "Scene.h"
#include "RenderableObject.h"
#include "Integrator.h"
#include "RenderTarget.h"

#include <thread>
#include <chrono>
#include <algorithm>
#include <condition_variable>
#include <queue>


enum ProgressState
{
	NotStarted,
	Running,
	Done,
	Canceled,
	Paused
};

enum TileState
{
	Waiting,
	InProgress,
	Complete
};

struct Tile
{
	int x;
	int y;
	int width;
	int height;
	TileState state;
};

class OfflineRenderer
{
public:
	OfflineRenderer(std::shared_ptr<Integrator> integrator, std::shared_ptr<Scene> scene);

	void Initialize();
	void Render(std::shared_ptr<RenderTarget> target);
	
	inline ProgressState GetProgressState() { return progressState; }
	inline void Cancel() { progressState = ProgressState::Canceled; pauseCV.notify_all(); }
	void TogglePaused();

	inline int GetPixelWidth() { return scene.get()->GetCamera().get()->GetPixelWidth(); }
	inline int GetPixelHeight() { return scene.get()->GetCamera().get()->GetPixelHeight(); }
	inline int GetPixelsRendered() { return pixelsRendered; }

	inline std::vector<Tile*> GetTiles() { return tiles; }

private:
	RTCDevice embreeDevice;

	ProgressState progressState = ProgressState::NotStarted;
	std::atomic<int> pixelsRendered;

	std::mutex pauseMutex;
	std::condition_variable pauseCV;

	int maxDepth;
	
	std::shared_ptr<Scene> scene;
	std::shared_ptr<Integrator> integrator;

	std::vector<Tile*> tiles;
	void genTiles(std::shared_ptr<RenderTarget> target, int tileSize = 32);
	
	void renderTile(Tile* tile, std::shared_ptr<RenderTarget> target);
};

