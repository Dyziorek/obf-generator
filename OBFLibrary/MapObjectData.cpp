#include "stdafx.h"
#include "MapObjectData.h"


MapObjectData::MapObjectData(void)
{
	localId = 0;
	isArea = false;
	boost::geometry::assign_inverse(bbox);
#ifdef _DEBUG
	correctBBox = true;
#endif

}


MapObjectData::~MapObjectData(void)
{
}
