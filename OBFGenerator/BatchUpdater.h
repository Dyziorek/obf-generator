#pragma once
#include "Amenity.h"
#include "OBFStreeDB.h"
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
};

class BatchUpdater
{
public:
	BatchUpdater(OBFResultDB& dbContext) : workContext(dbContext) {}
	~BatchUpdater(void);

	OBFRenderingTypes renderer;
	std::list<Amenity> amenities;
	std::list<lowLevelMapData> lowLevelMapList;
	std::list<binMapData> binaryMapList;
	void addBatch(Amenity itemToAdd);
	void addBatch(__int64 id, __int64 firstId, __int64 lastId, std::string& name, std::stringstream& bNodes,std::stringstream& bTypes,std::stringstream& bAddtTypes,int level);
	void addBatch(__int64 id, bool area, std::stringstream& bCoord, std::stringstream& bInCoord ,std::stringstream& bTypes,std::stringstream& bAddtTypes,std::string& name);
	OBFResultDB& workContext;
	std::string encodeAdditionalInfo(std::map<std::string, std::string> tempNames, std::string name, std::string nameEn);
	void commit();
private:
	void flush(bool flush = FALSE);
};

