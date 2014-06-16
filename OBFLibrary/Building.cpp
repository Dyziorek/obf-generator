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
	postCode = obj->getTag(OSMTags::ADDR_POSTCODE);
	if (postCode == "")
	{
		postCode = obj->getTag(OSMTags::POSTAL_CODE);
	}
}

int Building::getInterpValue()
{
	return interpType;
}
