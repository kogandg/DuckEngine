#include <integrators/Integrator.h>

glm::vec3 Integrator::background(Intersection intersection)
{
	switch (backgroundType)
	{
	case None:
		return glm::vec3(0.0f);
		break;

	case Color:
		return backgroundColor;
		break;

	case SimpleSky:
	{
		glm::vec3 unitDirection = glm::normalize(intersection.direction);
		float a = 0.5 * (unitDirection.y + 1.0);
		return (1.0f - a) * glm::vec3(1, 1, 1) + a * glm::vec3(0.5, 0.7, 1);
	}
	break;

	default:
		break;
	}
	return glm::vec3(0.0f);
}
