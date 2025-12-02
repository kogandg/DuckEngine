#include <embree/Triangle.h>

Triangle::Triangle(glm::vec3 v1, glm::vec3 v2, glm::vec3 v3)
{
	this->v1 = v1;
	this->v2 = v2;
	this->v3 = v3;
}

void Triangle::InitGeometry(RTCDevice device, RTCScene scene, glm::mat4 transform)
{
	RTCScene baseScene = rtcNewScene(device);

	RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);

	glm::vec3* vertexBuffer = (glm::vec3*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(glm::vec3), 3);
	vertexBuffer[0] = v1;
	vertexBuffer[1] = v2;
	vertexBuffer[2] = v3;

	//glm::ivec3* indexBuffer = (glm::ivec3*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, sizeof(glm::ivec3), 1);
	//indexBuffer[0] = glm::ivec3(0, 1, 2);

	unsigned* indexBuffer = (unsigned*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(unsigned), 1);
	indexBuffer[0] = 0;
	indexBuffer[1] = 1;
	indexBuffer[2] = 2;

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
