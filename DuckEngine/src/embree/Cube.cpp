#include <embree/Cube.h>

Cube::Cube()
{
}

void Cube::InitGeometry(RTCDevice device, RTCScene scene, glm::mat4 transform)
{
	RTCScene baseScene = rtcNewScene(device);

	RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

	glm::vec3* vertexBuffer = (glm::vec3*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3), 24);

	// Front
	vertexBuffer[0] = glm::vec3(cubeMin.x, cubeMin.y, cubeMax.z);
	vertexBuffer[1] = glm::vec3(cubeMax.x, cubeMin.y, cubeMax.z);
	vertexBuffer[2] = glm::vec3(cubeMax.x, cubeMax.y, cubeMax.z);
	vertexBuffer[3] = glm::vec3(cubeMin.x, cubeMax.y, cubeMax.z);

	// Back
	vertexBuffer[4] = glm::vec3(cubeMax.x, cubeMin.y, cubeMin.z);
	vertexBuffer[5] = glm::vec3(cubeMin.x, cubeMin.y, cubeMin.z);
	vertexBuffer[6] = glm::vec3(cubeMin.x, cubeMax.y, cubeMin.z);
	vertexBuffer[7] = glm::vec3(cubeMax.x, cubeMax.y, cubeMin.z);

	// Top
	vertexBuffer[8] = glm::vec3(cubeMin.x, cubeMax.y, cubeMax.z);
	vertexBuffer[9] = glm::vec3(cubeMax.x, cubeMax.y, cubeMax.z);
	vertexBuffer[10] = glm::vec3(cubeMax.x, cubeMax.y, cubeMin.z);
	vertexBuffer[11] = glm::vec3(cubeMin.x, cubeMax.y, cubeMin.z);

	// Bottom
	vertexBuffer[12] = glm::vec3(cubeMin.x, cubeMin.y, cubeMin.z);
	vertexBuffer[13] = glm::vec3(cubeMax.x, cubeMin.y, cubeMin.z);
	vertexBuffer[14] = glm::vec3(cubeMax.x, cubeMin.y, cubeMax.z);
	vertexBuffer[15] = glm::vec3(cubeMin.x, cubeMin.y, cubeMax.z);

	// Left
	vertexBuffer[16] = glm::vec3(cubeMin.x, cubeMin.y, cubeMin.z);
	vertexBuffer[17] = glm::vec3(cubeMin.x, cubeMin.y, cubeMax.z);
	vertexBuffer[18] = glm::vec3(cubeMin.x, cubeMax.y, cubeMax.z);
	vertexBuffer[19] = glm::vec3(cubeMin.x, cubeMax.y, cubeMin.z);

	// Right
	vertexBuffer[20] = glm::vec3(cubeMax.x, cubeMin.y, cubeMax.z);
	vertexBuffer[21] = glm::vec3(cubeMax.x, cubeMin.y, cubeMin.z);
	vertexBuffer[22] = glm::vec3(cubeMax.x, cubeMax.y, cubeMin.z);
	vertexBuffer[23] = glm::vec3(cubeMax.x, cubeMax.y, cubeMax.z);

	unsigned* indexBuffer = (unsigned*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned), 12);
	std::vector<unsigned int> indices = {
			0, 1, 2, 0, 2, 3,        // Front
			4, 5, 6, 4, 6, 7,        // Back
			8, 9, 10, 8, 10, 11,     // Top
			12, 13, 14, 12, 14, 15,  // Bottom
			16, 17, 18, 16, 18, 19,  // Left
			20, 21, 22, 20, 22, 23  // Right
	};
	for (int i = 0; i < indices.size(); i++)
	{
		indexBuffer[i] = indices[i];
	}

	rtcCommitGeometry(geom);
	rtcAttachGeometry(baseScene, geom);
	rtcReleaseGeometry(geom);
	rtcCommitScene(baseScene);

	RTCGeometry instance = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_INSTANCE);

	rtcSetGeometryInstancedScene(instance, baseScene);

	rtcSetGeometryTransform(instance, 0, RTC_FORMAT_FLOAT4X4_COLUMN_MAJOR, &transform[0]);

	rtcCommitGeometry(instance);
	geometryID = rtcAttachGeometry(scene, instance);
	rtcReleaseGeometry(instance);

	// Release the base scene after attaching it
	rtcReleaseScene(baseScene);
}
