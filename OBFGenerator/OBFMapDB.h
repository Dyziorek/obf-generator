#pragma once

#include "OBFRenderingTypes.h"
#include "RTree.h"

class OBFMapDB : public OBFResultDB
{
protected:
	std::list<long> typeUse;
	std::list<long> addtypeUse;
	std::map<MapRulType, std::string> namesUse;
	std::list<MapRulType> tempNameUse;
	MapZooms mapZooms;
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

	OBFRenderingTypes renderEncoder;
	std::map<long long, std::map<std::string, std::string>> propagatedTags;

	static long long notUsedId;
	void iterateMainEntity(std::shared_ptr<EntityBase>& relItem, OBFResultDB& dbContext);
	void iterateMainEntityPost(std::shared_ptr<EntityBase>& e) ;
	void excludeFromMainIteration(std::vector<std::shared_ptr<EntityWay>> l);
	long convertBaseIdToGeneratedId(long baseId, int level) {
		if (level >= MAP_LEVELS_MAX) {
			return -1;
		}
		return ((baseId << MAP_LEVELS_POWER) | level) << 1;
	}
	void insertLowLevelMapBinaryObject(int level, int zoom, std::list<long> types, std::list<long> addTypes, long id, std::vector<std::shared_ptr<EntityNode>> in, std::string name);
	void insertBinaryMapRenderObjectIndex(RTree mapTree, std::list<std::shared_ptr<EntityNode>>& nodes, std::list<std::list<std::shared_ptr<EntityNode>>>& innerWays,
			std::map<MapRulType, std::string>& names, long id, bool area, std::list<long>& types, std::list<long>& addTypes, bool commit);
	std::vector<RTree> mapTree;
	int zoomWaySmothness;
};

