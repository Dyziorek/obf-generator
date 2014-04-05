#pragma once
#include "Amenity.h"
#include "OBFStreeDB.h"
#include "sqlite3.h"

class BatchUpdater
{
public:
	BatchUpdater(OBFResultDB& dbContext) : workContext(dbContext) {}
	~BatchUpdater(void);

	OBFRenderingTypes renderer;
	std::list<Amenity> amenities;
	void addBatch(Amenity itemToAdd);
	OBFResultDB& workContext;
	std::string encodeAdditionalInfo(std::map<std::string, std::string> tempNames, std::string name, std::string nameEn);
	void commit();
private:
	void flush();
};

