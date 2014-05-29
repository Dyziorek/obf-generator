#pragma once

#include "TileManager.h"

class MapObject;

#define PHASEINDEXCITY		10
#define PHASEINDEXADDRREL	20
#define PHASEMAINITERATE	30
#define PHASECOMBINE		40

#define NODEELEM 1
#define NODEREL 2
#define NODEWAY 3
#define NODEWAYBOUNDARY 4


class OBFResultDB
{
public:
	OBFResultDB(void);
	~OBFResultDB(void);
	void close(void);
	sqlite3* dbMapCtx;
	sqlite3_stmt* mapStmt;
	sqlite3_stmt* lowStmt;
	sqlite3* dbRouteCtx;
	sqlite3_stmt* routeStmt;
	sqlite3_stmt* baseRouteStmt;
	sqlite3* dbAddrCtx;
	sqlite3_stmt* streetStmt;
	sqlite3_stmt* streetNodeStmt;
	sqlite3_stmt* buildStmt;
	sqlite3_stmt* searchStrStmt;
	sqlite3_stmt* searchStrNoCityStmt;
	sqlite3_stmt* updateCityStmt;
	sqlite3_stmt* searchBuildStmt;
	sqlite3_stmt* removeBuildStmt;
	sqlite3_stmt* searchStrNodeStmt;
	sqlite3_stmt* cityStmt;

	

	// selectors from d
	sqlite3_stmt* selNodeStmt;
	sqlite3_stmt* selWayStmt;
	sqlite3_stmt* selRelStmt;
	sqlite3_stmt* itNodeStmt;
	sqlite3_stmt* itWayStmt;
	sqlite3_stmt* itRelStmt;
	sqlite3_stmt* itWayBoundStmt;


	sqlite3* dbPoiCtx;
	sqlite3_stmt* poiNodeStmt;

	sqlite3* dbTransCtx;
	void storeCities();
	boost::unordered_map<__int64, CityObj> cityLocator;
	boost::unordered_map<__int64, CityObj> villageLocator;
	int PrepareDB(sqlite3* dbCtxSrc);
	static int shell_callback(void *pArg, int nArg, char **azArg, char **azCol);
	int iterateOverElements(int type, std::function<void (std::shared_ptr<EntityBase>)>saver);
	int iterateOverElements(int typeElem);
	void SaverCityNode(EntityBase*, TileManager<CityObj>& manager);
	void loadRelationMembers(EntityRelation* relItem);
	void loadWays(EntityWay* wayItem);
	void loadNodesOnRelation(EntityRelation* relItem);
	

	void mainIteration(std::shared_ptr<EntityBase>& relItem);
	virtual void iterateMainEntity(std::shared_ptr<EntityBase>& relItem, OBFResultDB& dbContext);
	std::set<std::shared_ptr<EntityRelation>> relations;
	void addBatch(Amenity am);
	void addBatch(__int64 id, __int64 firstId, __int64 lastId, std::string& name, std::stringstream& bNodes,std::stringstream& bTypes,std::stringstream& bAddtTypes,int level);
	void addBatch(__int64 id, bool area, std::stringstream& bCoord, std::stringstream& bInCoord ,std::stringstream& bTypes,std::stringstream& bAddtTypes,std::string& name);
	void addBatchRoute(__int64 id, std::stringstream& types, std::stringstream& ptTypes ,std::stringstream& ptIds,std::stringstream& coords, std::string& name, bool base);
	void flush();
	std::map<__int64, std::shared_ptr<EntityNode>> nodes;
	std::map<__int64, std::shared_ptr<EntityWay>> ways;
	std::map<__int64, std::shared_ptr<EntityWay>> waybounds;
	std::map<__int64, std::shared_ptr<EntityRelation>> relNodes;

	void imageResult();
	OBFResultDB* mapIndexer;
	OBFResultDB* poiIndexer;
	OBFResultDB* transIndexer;
	OBFResultDB* routeIndexer;
	OBFResultDB* addresIndexer;
	BatchUpdater* storeData;
};

