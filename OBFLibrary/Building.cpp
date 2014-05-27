#include "stdafx.h"
#include "Building.h"


Building::Building(void)
{
	interpType = NONE;
}


Building::~Building(void)
{
}

void Building::setBuilding(EntityBase* obj)
{
	postCode = obj->getTag("addr:postCode");
	if (postCode == "")
	{
		postCode = obj->getTag("postal_code");
	}
}
