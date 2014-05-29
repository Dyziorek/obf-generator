#pragma once
#include "Amenity.h"
#include "sqlite3.h"


struct lowLevelMapData
{
	__int64 id;
	__int64 firstid;
	__int64 lastid;
	std::string name;
	std::string nodes;
	std::string types;
	std::string addtypes;
	int level;
	~lowLevelMapData(){}
};

struct binMapData
{
	__int64 id;
	bool area;
	std::string coord;
	std::string incoord;
	std::string types;
	std::string addtypes;
	std::string name;
	~binMapData(){}
};


struct routeData
{
	__int64 id;
	std::string types;
	std::string ptTypes;
	std::string ptIds;
	std::string coords;
	std::string name;
};

class OBFResultDB;

class BatchUpdater
{
public:
	BatchUpdater(OBFResultDB& dbContext) : workContext(dbContext) {}
	~BatchUpdater(void);

	OBFRenderingTypes renderer;
	std::list<Amenity> amenities;
	std::list<lowLevelMapData> lowLevelMapList;
	std::list<binMapData> binaryMapList;
	std::list<routeData> routeList;
	std::list<routeData> baserouteList;
	void addBatch(Amenity itemToAdd);
	void addBatch(__int64 id, __int64 firstId, __int64 lastId, std::string& name, std::stringstream& bNodes,std::stringstream& bTypes,std::stringstream& bAddtTypes,int level);
	void addBatch(__int64 id, bool area, std::stringstream& bCoord, std::stringstream& bInCoord ,std::stringstream& bTypes,std::stringstream& bAddtTypes,std::string& name);
	void addBatchRoute(__int64 id, std::stringstream& types, std::stringstream& ptTypes ,std::stringstream& ptIds,std::stringstream& coords, std::string& name, bool base);
	OBFResultDB& workContext;
	std::string encodeAdditionalInfo(boost::unordered_map<std::string, std::string>& tempNames, std::string name, std::string nameEn);
	void commit();
private:
	void flush(bool flush = FALSE);
};

