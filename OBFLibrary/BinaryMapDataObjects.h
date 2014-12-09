
#pragma once

#include "RTree.h"
#include "MapObjectData.h"

namespace bgm = boost::geometry::model;

typedef RTree<std::pair<__int64, std::vector<std::shared_ptr<MapObjectData>>>> treeMap;

class BinaryMapRules;

struct BinaryMapSection : public std::enable_shared_from_this<BinaryMapSection>
{
	std::pair<uint32_t, uint32_t> zoomLevels;
	treeMap::box rootBox;
	boxD geoBox;
	uint32_t offset;
	uint32_t dataOffset;
	std::list<std::shared_ptr<BinaryMapSection>> childSections;
	std::unordered_map<uint64_t, std::shared_ptr<MapObjectData>> sectionData;
	std::shared_ptr<BinaryMapRules> rules;
	std::shared_ptr<BinaryMapSection> getSharedPtr()
	{
		return shared_from_this();
	}


	BinaryMapSection()
	{
		offset = 0;
		dataOffset = 0;
	}

	~BinaryMapSection()
	{
		sectionData.clear();
	}
};

boxD translateBox(boxI inputBox);
boxI invTranslateBox(boxD geoBox);

class BinaryMapRules
{
public:
	BinaryMapRules();
	~BinaryMapRules();

	void createMissingRules();
	void createRule(uint32_t id, uint32_t ruleType, std::string name, std::string value);
	uint32_t getruleIdFromNames(const std::string& tag,const std::string& name);
	MapDecodingRule getRuleInfo(uint32_t id) { return mapRules[id];}
public:
    uint32_t name_encodingRuleId;
    uint32_t ref_encodingRuleId;
    uint32_t naturalCoastline_encodingRuleId;
    uint32_t naturalLand_encodingRuleId;
    uint32_t naturalCoastlineBroken_encodingRuleId;
    uint32_t naturalCoastlineLine_encodingRuleId;
    uint32_t highway_encodingRuleId;
    uint32_t oneway_encodingRuleId;
    uint32_t onewayReverse_encodingRuleId;
    uint32_t tunnel_encodingRuleId;
    uint32_t bridge_encodingRuleId;
    uint32_t layerLowest_encodingRuleId;

    std::unordered_set<uint32_t> positiveLayers_encodingRuleIds;
    std::unordered_set<uint32_t> zeroLayers_encodingRuleIds;
    std::unordered_set<uint32_t> negativeLayers_encodingRuleIds;

	std::unordered_map<uint32_t, MapDecodingRule> mapRules;
	std::unordered_map<std::string, std::unordered_map<std::string, uint32_t>> mapRuleIdNames;
};
