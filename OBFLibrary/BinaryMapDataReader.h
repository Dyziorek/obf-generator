#pragma once

#include "RTree.h"
#include "BinaryMapDataObjects.h"
namespace gp = google::protobuf;
namespace gio = google::protobuf::io;
namespace bgm = boost::geometry::model;


class BinaryMapRules;
class MapStyleInfo;
class MapStyleEval;

class BinaryMapDataReader
{
public:


	BinaryMapDataReader(void);
	~BinaryMapDataReader(void);

	
	void ReadMapDataSection(gio::CodedInputStream* cis);
	void readMapLevelHeader(gio::CodedInputStream* cis,  std::shared_ptr<BinaryMapSection> section, int offset, boxI& region);

	void readMapEncodingRules(gio::CodedInputStream* cis, uint32_t defRuleId);
	void loadTreeNodes(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection>& section, boxI& area);
	void loadMapDataObjects(gio::CodedInputStream* cis,  std::shared_ptr<BinaryMapSection>& section, boxI& area);
	//std::shared_ptr<MapObjectData> loadMapDataObjectData(gio::CodedInputStream* cis,  std::shared_ptr<BinaryMapSection>& section, boxI& area);
	void loadChildTreeNode(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection>& childSection, boxI& area);
	void PaintSections();
	void paintSection(std::shared_ptr<BinaryMapSection>& subChildsPop, boxI& cover, double  minX,double minY, double scale, void* painter);
	bool isCovered(std::shared_ptr<BinaryMapSection>& subChildsPop, boxI& cover);
	void paintSectionData(std::unordered_map<uint64_t, std::shared_ptr<MapObjectData>> &sectionData, double minX, double minY,double scale, void* painter);
	void getBoxesReferences(std::shared_ptr<BinaryMapSection>& section);
	void readMapObject(gio::CodedInputStream* cis, std::shared_ptr<BinaryMapSection>& section,uint64_t baseid, std::unordered_map<uint64_t, std::shared_ptr<MapObjectData>>& objects);
	void MergeStringsToObjects(std::unordered_map<uint64_t, std::shared_ptr<MapObjectData>>& objects, std::vector<std::string>& stringList);
	
	void evaluate(std::shared_ptr<MapStyleInfo>& infoDump);

	void loadMapDataObjects(gio::CodedInputStream* cis, boxI& area, int currentZoom, std::list<std::shared_ptr<const MapObjectData>>& resultOut);
	const std::vector<std::tuple<treeMap::box, std::pair<gp::uint32, gp::uint32> ,std::shared_ptr<BinaryMapSection>>> getSections() const
	{
		return sections;
	}
	void getSectionData(boxI& area, std::list<std::shared_ptr<BinaryMapSection>>& children, std::list<std::shared_ptr<BinaryMapSection>>& dataOut);
private:
	std::vector<std::tuple<treeMap::box, std::pair<gp::uint32, gp::uint32> ,std::shared_ptr<BinaryMapSection>>> sections;
	std::string mapName;
	std::shared_ptr<BinaryMapRules> mapRules;
	std::map<long long, std::shared_ptr<BinaryMapSection>> mapDataReferences;
	OBFRenderingTypes renderEncoder;
	GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(BinaryMapDataReader);

	    enum {
        ShiftCoordinates = 5,
        MaskToRead = ~((1u << ShiftCoordinates) - 1),
    };

	void evaluateObject(std::shared_ptr<MapObjectData>& infoDump, MapStyleEval& ordevaluator
								   , MapStyleEval& textevaluator
								   , MapStyleEval& polyevaluator
								   , MapStyleEval& ptevaluator
								   , MapStyleEval& lineevaluator);
};

