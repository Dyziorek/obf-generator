#pragma once

#include "OBFRenderingTypes.h"
#include "RTree.h"



class OBFMapDB : public OBFResultDB
{
	typedef RTree<std::pair<__int64, std::vector<short>>> RTreeValued;

protected:
	std::list<long> typeUse;
	std::list<long> addtypeUse;
	std::map<MapRulType, std::string> namesUse;
	std::list<MapRulType> tempNameUse;
	MapZooms mapZooms;
	sqlite3_stmt* lowLevelWayItStart;
	sqlite3_stmt* lowLevelWayItEnd;

private:
	static int MAP_LEVELS_POWER;
	static int MAP_LEVELS_MAX;
public:
	OBFMapDB(void);
	virtual ~OBFMapDB(void);

	void indexMapAndPolygonRelations(std::shared_ptr<EntityRelation>& relItem, OBFResultDB& dbContext);
	void indexMultiPolygon(std::shared_ptr<EntityRelation>& relItem, OBFResultDB& dbContext);
	void paintPolys();

	//std::map<long long, std::vector<long>> TLongHashArray;
	std::map<long long, std::vector<long>> multiPolygonsWays;
	std::list<std::shared_ptr<MultiPoly>> polyLines;
	static int numberCalls;
	sqlite3_stmt* selectData;

	OBFRenderingTypes renderEncoder;
	std::map<long long, std::map<std::string, std::string>> propagatedTags;

	static long long notUsedId;
	void iterateMainEntity(std::shared_ptr<EntityBase>& relItem, OBFResultDB& dbContext);
	void iterateMainEntityPost(std::shared_ptr<EntityBase>& e, OBFResultDB& dbContext) ;
	void processLowLevelWays(OBFResultDB& dbContext);
	void paintTreeData(OBFResultDB& dbContext, std::set<std::shared_ptr<MultiPoly>>& bounds, std::map<std::shared_ptr<EntityNode>, CityObj>& cities);
	void parseAndSort(const void* blobData, int blobSize, std::list<long>& toData);
	bool checkForSmallAreas(std::vector<std::shared_ptr<EntityNode>> nodes, int zoom, int minz, int maxz);
	std::vector<std::shared_ptr<EntityNode>> simplifyCycleWay(std::vector<std::shared_ptr<EntityNode>> ns, int zoom, int zoomWaySmothness);
	void excludeFromMainIteration(std::vector<std::shared_ptr<EntityWay>> l);
	__int64 convertBaseIdToGeneratedId(__int64 baseId, int level) {
		if (level >= MAP_LEVELS_MAX) {
			return -1;
		}
		return ((baseId << MAP_LEVELS_POWER) | level) << 1;
	}
	__int64 convertGeneratedIdToObfWrite(__int64 id) {
		return (id >> (MAP_LEVELS_POWER)) + (id & 1);
	}

	void decodeNames(std::string name, std::map<MapRulType, std::string>& tempNames);

	void insertLowLevelMapBinaryObject(int level, int zoom, std::list<long> types, std::list<long> addTypes, __int64 id, std::vector<std::shared_ptr<EntityNode>> in, std::string name, OBFResultDB& dbContext);
	bool insertBinaryMapRenderObjectIndex(RTreeValued& mapTree, std::list<std::shared_ptr<EntityNode>>& nodes, std::vector<std::vector<std::shared_ptr<EntityNode>>>& innerWays,
			std::map<MapRulType, std::string>& names, __int64 id, bool area, std::list<long>& types, std::list<long>& addTypes, bool commit, OBFResultDB& dbContext);
	std::vector<RTreeValued> mapTree;
	std::string encodeNames(std::map<MapRulType, std::string> tempNames);
	int zoomWaySmothness;

	void writeBinaryMapIndex(BinaryMapDataWriter& writer, std::string regionName, OBFResultDB& ctx);
	void writeBinaryMapTree(RTreeValued& parent, RTreeValued::box& re, BinaryMapDataWriter& writer, std::unordered_map<__int64, boost::tuple<obf::MapDataBlock*,BinaryFileReference*>>& bounds);
	void writeBinaryMapBlock(RTreeValued& treeMap, RTreeValued::box& parentBounds, BinaryMapDataWriter& writer, sqlite3_stmt* selectData,
			std::unordered_map<__int64, boost::tuple<obf::MapDataBlock*,BinaryFileReference*>>& bounds, std::unordered_map<std::string, int>& tempStringTable, std::map<MapRulType, std::string>& tempNames, MapZooms::MapZoomPair level);
	void callNodeBox(const RTreeValued::box& re, bool begin, bool hasContent, BinaryMapDataWriter& writer, std::unordered_map<__int64, boost::tuple<obf::MapDataBlock*,BinaryFileReference*>>& bounds);
	void callNodeBoxBlock(const RTreeValued::box& boxParam,const RTreeValued::value& valData, bool fromleaf, bool isLeaf, BinaryMapDataWriter& writer, std::unordered_map<__int64, boost::tuple<obf::MapDataBlock*,BinaryFileReference*>>& bounds, sqlite3_stmt* selectData, std::unordered_map<std::string, int>& tempStringTable, std::map<MapRulType, std::string>& tempNames, MapZooms::MapZoomPair level);
};

