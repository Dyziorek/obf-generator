#pragma once
#include "Amenity.h"
#include "OBFStreeDB.h"
#include "sqlite3.h"


struct lowLevelMapData
{
	long id;
	long firstid;
	long lastid;
	std::string name;
	std::string nodes;
	std::string types;
	std::string addtypes;
	int level;
};

struct binMapData
{
	long id;
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
	void addBatch(long id, long firstId, long lastId, std::string& name, std::stringstream& bNodes,std::stringstream& bTypes,std::stringstream& bAddtTypes,int level);
	void addBatch(long id, bool area, std::stringstream& bCoord, std::stringstream& bInCoord ,std::stringstream& bTypes,std::stringstream& bAddtTypes,std::string& name);
	OBFResultDB& workContext;
	std::string encodeAdditionalInfo(std::map<std::string, std::string> tempNames, std::string name, std::string nameEn);
	void commit();
private:
	void flush(bool flush = FALSE);
};

