#pragma once

#include <glm/glm.hpp>

class RenderTarget
{
public:
	RenderTarget(int width, int height);

	void WritePixel(glm::vec3 color, int x, int y);

	inline int GetWidth() { return width; }
	inline int GetHeight() { return height; }
	inline unsigned char* GetImage() { return image; }

private:
	int width;
	int height;
	unsigned char* image;
};
