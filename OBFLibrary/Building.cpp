#include "stdafx.h"
#include "Building.h"


Building::Building(void)
{
	interpType = NONE;
	interval = 0;
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

void Building::setInterpType(std::string typeVal)
{
	boost::to_upper(typeVal);

	interpType = BuildingInterpolation::NONE;

	if (typeVal == "NONE")
	{
		interpType = BuildingInterpolation::NONE;
	}
	else if (typeVal == "ALL")
	{
		interpType = BuildingInterpolation::ALL;
	}
	else if (typeVal == "EVEN")
	{
		interpType = BuildingInterpolation::EVEN;
	}else if (typeVal == "ODD")
	{
		interpType = BuildingInterpolation::ODD;
	}else if( typeVal == "ALPHA")
	{
		interpType = BuildingInterpolation::ALPHA;
	}
}