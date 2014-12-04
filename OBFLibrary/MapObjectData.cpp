#include "stdafx.h"
#include "MapObjectData.h"
#include "BinaryMapDataObjects.h"

MapObjectData::MapObjectData(void)
{
	localId = 0;
	isArea = false;
	boost::geometry::assign_inverse(bbox);
#ifdef _DEBUG
	correctBBox = true;
#endif
	section.reset(new BinaryMapSection);
	section->rules.reset(new BinaryMapRules());
	section->rules->createMissingRules();
}

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
	const auto typeRuleId = section->rules->getruleIdFromNames(tag, value);

	std::list<int> typeList = checkAdditional ? addtype : type;

	for (int typeId : typeList)
	{
		if (typeId == typeRuleId)
			return true;
	}

    return false;
}

bool MapObjectData::containsType(const uint32_t typeRuleId, bool checkAdditional) const
{
	std::list<int> typeList = checkAdditional ? addtype : type;
	for (int typeId : typeList)
	{
		if (typeId == typeRuleId)
			return true;
	}

    return false;
}

int MapObjectData::getSimpleLayerValue() const
{
    for(const auto typeRuleId : addtype)
    {
        

		if(section->rules->positiveLayers_encodingRuleIds.find(typeRuleId) != section->rules->positiveLayers_encodingRuleIds.end())
            return 1;
		else if(section->rules->negativeLayers_encodingRuleIds.find(typeRuleId) != section->rules->negativeLayers_encodingRuleIds.end())
            return -1;
        else if(section->rules->zeroLayers_encodingRuleIds.find(typeRuleId) != section->rules->zeroLayers_encodingRuleIds.end())
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