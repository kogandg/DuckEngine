#pragma once

#include <embree4/rtcore.h>

class EmbreeGeometry
{
public: 
	virtual void InitGeometry(RTCDevice device, RTCScene scene);

	inline unsigned int GetGeometryID() { return geometryID; }
	
protected:
	unsigned int geometryID = RTC_INVALID_GEOMETRY_ID;
};

