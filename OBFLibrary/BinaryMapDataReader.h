#pragma once

#include "RTree.h"
#include "MapObjectData.h"
namespace gp = google::protobuf;
namespace gio = google::protobuf::io;

typedef RTree<std::pair<__int64, std::vector<std::shared_ptr<MapObjectData>>>> treeMap;

struct BinaryMapSection
{
	std::pair<gp::uint32, gp::uint32> zoomLevels;
	treeMap::box rootBox;
	gp::uint32 offset;
	treeMap mapDataTree;
};


class BinaryMapRules
{
public:
	BinaryMapRules();
	~BinaryMapRules();

	void createMissingRules();
	void createRule(uint32_t id, uint32_t ruleType, std::string name, std::string value);
private:
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

class BinaryMapDataReader
{
public:


	BinaryMapDataReader(void);
	~BinaryMapDataReader(void);

	
	void ReadMapDataSection(gio::CodedInputStream* cis);
	void readMapLevelHeader(gio::CodedInputStream* cis,  std::shared_ptr<BinaryMapSection> section, int offset);

	void readMapEncodingRules(gio::CodedInputStream* cis, uint32_t defRuleId);

private:
	std::vector<std::tuple<treeMap::box, std::pair<gp::uint32, gp::uint32> ,std::shared_ptr<BinaryMapSection>>> sections;
	std::string mapName;
	std::unique_ptr<BinaryMapRules> mapRules;
	GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(BinaryMapDataReader);
};

