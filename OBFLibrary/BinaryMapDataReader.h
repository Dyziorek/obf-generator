#pragma once

#include "RTree.h"
#include "MapObjectData.h"
namespace gp = google::protobuf;
namespace gio = google::protobuf::io;

typedef RTree<std::pair<__int64, std::vector<std::shared_ptr<MapObjectData>>>> treeMap;

typedef bg::model::point<int, 2, bg::cs::cartesian> point;
typedef bg::model::box<point> box;

struct BinaryMapSection : public std::enable_shared_from_this<BinaryMapSection>
{
	std::pair<gp::uint32, gp::uint32> zoomLevels;
	treeMap::box rootBox;
	boost::geometry::model::box<boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>> geoBox;
	gp::uint32 offset;
	gp::uint32 dataOffset;
	std::list<std::shared_ptr<BinaryMapSection>> childSections;
	std::shared_ptr<BinaryMapSection> getSharedPtr()
	{
		return shared_from_this();
	}

	void translateBox()
	{
		geoBox.min_corner().set<0>(MapUtils::get31LongitudeX(rootBox.min_corner().get<0>()));
		geoBox.min_corner().set<1>(MapUtils::get31LatitudeY(rootBox.min_corner().get<1>()));
		geoBox.max_corner().set<0>(MapUtils::get31LongitudeX(rootBox.max_corner().get<0>()));
		geoBox.max_corner().set<1>(MapUtils::get31LatitudeY(rootBox.max_corner().get<1>()));
	}
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
	void loadTreeNodes(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection>& section);
	void loadChildTreeNode(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection>& childSection);

private:
	std::vector<std::tuple<treeMap::box, std::pair<gp::uint32, gp::uint32> ,std::shared_ptr<BinaryMapSection>>> sections;
	std::string mapName;
	std::unique_ptr<BinaryMapRules> mapRules;
	GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(BinaryMapDataReader);
};

