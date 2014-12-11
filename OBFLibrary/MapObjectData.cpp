#include "stdafx.h"
#include "MapObjectData.h"
#include "BinaryMapDataObjects.h"


MapObjectData::MapObjectData(std::shared_ptr<BinaryMapSection> workSection) : section(workSection)
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

bool MapObjectData::containsTypeSlow( const std::string& tag, const std::string& value, bool checkAdditional /*= false*/ ) const
{
	if(section.expired())
		return false;

	std::shared_ptr<BinaryMapSection> sectionData = section.lock();

	const auto typeRuleId = sectionData->rules->getruleIdFromNames(tag, value);

	std::vector<int> typeList = checkAdditional ? addtypeIds : typeIds;

	for (int typeId : typeList)
	{
		if (typeId == typeRuleId)
			return true;
	}

    return false;
}

bool MapObjectData::containsType(const uint32_t typeRuleId, bool checkAdditional) const
{
	std::vector<int> typeList = checkAdditional ? addtypeIds : typeIds;
	for (int typeId : typeList)
	{
		if (typeId == typeRuleId)
			return true;
	}

    return false;
}

int MapObjectData::getSimpleLayerValue() const
{
	if(section.expired())
		return 0;

	std::shared_ptr<BinaryMapSection> sectionData = section.lock();


    for(const auto typeRuleId : addtypeIds)
    {
        

		if(sectionData->rules->positiveLayers_encodingRuleIds.find(typeRuleId) != sectionData->rules->positiveLayers_encodingRuleIds.end())
            return 1;
		else if(sectionData->rules->negativeLayers_encodingRuleIds.find(typeRuleId) != sectionData->rules->negativeLayers_encodingRuleIds.end())
            return -1;
        else if(sectionData->rules->zeroLayers_encodingRuleIds.find(typeRuleId) != sectionData->rules->zeroLayers_encodingRuleIds.end())
            return 0;
    }

    return 0;
}

bool MapObjectData::isClosedFigure(bool checkInner /*= false*/) const
{
    if(checkInner)
    {
        for(const auto polygon : innerpolypoints)
        {

            if(polygon.empty())
                continue;

			if(!bg::equals(polygon.front(),polygon.back()))
                return false;
        }
        return true;
    }
    else
        return bg::equals(points.front(), points.back());
}