#pragma once
#include "OBFResultDB.h"
#include "MapRoutingTypes.h"
#include "RTree.h"

class OBFpoiDB :
	public OBFResultDB
{
	OBFRenderingTypes renderer;
	std::map<long long, boost::unordered_map<std::string, std::string>> propagatedTags;
	std::list<Amenity> tempAmenityList;
public:
	OBFpoiDB(void);
	virtual ~OBFpoiDB(void);
	void indexRelations(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext);
	void iterateMainEntity(std::shared_ptr<EntityBase>& relItem, OBFResultDB& dbContext);
	void insertAmenityIntoPoi(Amenity amenity, OBFResultDB& dbContext);
};


class OBFtransportDB :
	public OBFResultDB
{
public:
	OBFtransportDB(void);
	virtual ~OBFtransportDB(void);
};


class OBFrouteDB :
	public OBFResultDB
{
private:
	static long SHIFT_INSERT_AT;
	static long SHIFT_ORIGINAL;
	static long SHIFT_ID;

	class RouteMissingPoints 
	{
	public:
		std::map<int, __int64> pointsMap;
		std::vector<std::shared_ptr<std::vector<int>>> pointsXToInsert;
		std::vector<std::shared_ptr<std::vector<int>>> pointsYToInsert;
		
		void buildPointsToInsert(int targetLength){
			for(auto p : pointsMap) {
				int insertAfter = p.first & ((1 << SHIFT_INSERT_AT) -1);
				if(pointsXToInsert[insertAfter]) {
					pointsXToInsert[insertAfter].swap(std::shared_ptr<std::vector<int>>(new std::vector<int>()));
					pointsYToInsert[insertAfter].swap(std::shared_ptr<std::vector<int>>(new std::vector<int>()));
				}
				__int64 x = p.second >> 31;
				__int64 y = p.second - (x << 31);
				pointsXToInsert[insertAfter]->push_back((int) x);
				pointsYToInsert[insertAfter]->push_back((int) y);
			}
		}
	};
public:
	std::vector<int> outTypes;
	std::map<MapRouteType, std::string> names;
	std::map<__int64, std::vector<int>> pointTypes;
	RTree routeTree;
	RTree baserouteTree;
	OBFrouteDB(void);
	virtual ~OBFrouteDB(void);
	void indexHighwayRestrictions(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext);
	void indexRelations(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext);
	void iterateMainEntity(std::shared_ptr<EntityBase>& item, OBFResultDB& dbContext);
	void addWayToIndex(long long id, std::vector<std::shared_ptr<EntityNode>>& nodes, OBFResultDB& dbContext, RTree rTree,  bool base);
	void registerBaseIntersectionPoint(long long pointLoc, bool registerId, long long wayId, int insertAt, int originalInd);
	std::string encodeNames(std::map<MapRouteType, std::string> tempNames);
	void putIntersection(long long  point, long long wayNodeId);
	boost::unordered_map<__int64, std::list<__int64>> highwayRestrictions;
	boost::unordered_map<__int64, __int64> basemapRemovedNodes;
	boost::unordered_map<__int64, RouteMissingPoints> basemapNodesToReinsert;
	std::map<long long, boost::unordered_map<std::string, std::string>> propagatedTags;
	OBFRenderingTypes renderer;
	MapRoutingTypes routingTypes;
};

class OBFAddresStreetDB :
	public OBFResultDB
{
public:
	TileManager<MapObject> cityManager;
	TileManager<MapObject> townManager;
	std::map<std::shared_ptr<EntityNode>, MapObject> cities;

	std::set<__int64> visitedBoundaryWays;
	std::set<std::shared_ptr<MultiPoly>> boundaries;
	OBFAddresStreetDB(void);
	virtual ~OBFAddresStreetDB(void);
	void indexBoundary(std::shared_ptr<EntityBase>& baseItem, OBFResultDB& dbContext);
	void iterateOverCity(std::shared_ptr<EntityNode>& cityNode);
	void storeCities(OBFResultDB& dbContext);
};