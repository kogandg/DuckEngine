#include <graphics/RenderTarget.h>

RenderTarget::RenderTarget(int width, int height)
{
	this->width = width;
	this->height = height;

	image = new unsigned char[width * height * 3];
	for (int i = 0; i < width * height * 3; i++)
	{
		image[i] = 0;
	}
}

int clamp(int value, int lower, int upper)
{
	if (value > upper) return upper;
	if (value < lower) return lower;
	return value;
}

void RenderTarget::WritePixel(glm::vec3 color, int x, int y)
{
	int index = (y * width + x) * 3; // 3 channels (RGB) per pixel

	int red = clamp(255.99 * color.r, 0, 255);
	int green = clamp(255.99 * color.g, 0, 255);
	int blue = clamp(255.99 * color.b, 0, 255);

	image[index + 0] = red;  // Red
	image[index + 1] = green;  // Green
	image[index + 2] = blue;  // Blue
}