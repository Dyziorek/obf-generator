
#include "stdafx.h"
#include "BinaryMapDataObjects.h"

boxD translateBox(boxI rootBox)
{
	boxD geoBox;
	geoBox.min_corner().set<0>(MapUtils::get31LongitudeX(rootBox.min_corner().get<0>()));
	geoBox.min_corner().set<1>(MapUtils::get31LatitudeY(rootBox.min_corner().get<1>()));
	geoBox.max_corner().set<0>(MapUtils::get31LongitudeX(rootBox.max_corner().get<0>()));
	geoBox.max_corner().set<1>(MapUtils::get31LatitudeY(rootBox.max_corner().get<1>()));
	return geoBox;
}

boxI invTranslateBox(boxD geoBox)
{
	boxI tileBox;
	tileBox.min_corner().set<0>(MapUtils::get31TileNumberX(geoBox.min_corner().get<0>()));
	tileBox.min_corner().set<1>(MapUtils::get31TileNumberY(geoBox.min_corner().get<1>()));
	tileBox.max_corner().set<0>(MapUtils::get31TileNumberX(geoBox.max_corner().get<0>()));
	tileBox.max_corner().set<1>(MapUtils::get31TileNumberY(geoBox.max_corner().get<1>()));
	return tileBox;
}

#pragma push_macro("max")
#undef max

 BinaryMapRules::BinaryMapRules() :  name_encodingRuleId(0), 
	ref_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    naturalCoastline_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    naturalLand_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    naturalCoastlineBroken_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    naturalCoastlineLine_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    highway_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    oneway_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    onewayReverse_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    tunnel_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    bridge_encodingRuleId(std::numeric_limits<uint32_t>::max()),
    layerLowest_encodingRuleId(std::numeric_limits<uint32_t>::max())
 {

 }

#pragma pop_macro("max")

BinaryMapRules::~BinaryMapRules()
{
}

void BinaryMapRules::createMissingRules()
{
	
}

void BinaryMapRules::createRule(uint32_t ruleType, uint32_t id, std::string name, std::string value)
{
	std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>>::iterator itMapRule = mapRuleIdNames.find(name);
	if (itMapRule == mapRuleIdNames.end())
	{
		itMapRule = mapRuleIdNames.insert(std::make_pair(name, std::unordered_map<std::string, uint32_t>())).first;
	}
	(*itMapRule).second.insert(std::make_pair(value, id));
	//mapRuleIdNames.insert(itMapRule, std::make_pair(name, id));
	if (mapRules.find(id) == mapRules.end())
	{
		MapDecodingRule ruleData;
		ruleData.type = ruleType;
		ruleData.tag = name;
		ruleData.value = value;
		mapRules.insert(std::make_pair(id, ruleData));
	}
	if(name == "name")
        name_encodingRuleId = id;
    else if(name == "ref")
        ref_encodingRuleId = id;
    else if(name == "natural" && value == "coastline")
        naturalCoastline_encodingRuleId = id;
    else if(name == "natural" && value == "land")
        naturalLand_encodingRuleId = id;
    else if(name == "natural" && value == "coastline_broken")
        naturalCoastlineBroken_encodingRuleId = id;
    else if(name == "natural" && value == "coastline_line")
        naturalCoastlineLine_encodingRuleId = id;
    else if(name == "oneway" && value == "yes")
        oneway_encodingRuleId = id;
    else if(name == "oneway" && value == "-1")
        onewayReverse_encodingRuleId = id;
    else if(name == "tunnel" && value == "yes")
    {
        tunnel_encodingRuleId = id;
        negativeLayers_encodingRuleIds.insert(id);
    }
    else if(name == "bridge"  && value == "yes")
    {
        bridge_encodingRuleId = id;
        positiveLayers_encodingRuleIds.insert(id);
    }
    else if(name == "layer")
    {
        if(!value.empty() && value != "0")
        {
            if(value[0] == '-')
                negativeLayers_encodingRuleIds.insert(id);
            else if(value[0] == '0')
                zeroLayers_encodingRuleIds.insert(id);
            else
                positiveLayers_encodingRuleIds.insert(id);
        }
    }
}

uint32_t BinaryMapRules::getruleIdFromNames(const std::string& tag,const std::string& name)
{
	return mapRuleIdNames.find(tag)->second.find(name)->second;
}
