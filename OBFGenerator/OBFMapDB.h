#pragma once

#include "OBFRenderingTypes.h"

class OBFMapDB : public OBFResultDB
{
public:
	OBFMapDB(void);
	virtual ~OBFMapDB(void);
	MapZooms mapZooms;
	void indexMapAndPolygonRelations(std::shared_ptr<EntityRelation>& relItem, OBFResultDB& dbContext);
	void paintPolys();

	std::map<long long, std::vector<long>> TLongHashArray;
	std::list<std::shared_ptr<MultiPoly>> polyLines;
	static int numberCalls;

	OBFRenderingTypes renderEncoder;
	std::map<long long, std::map<std::string, std::string>> propagatedTags;

	static long long notUsedId;
	void iterateMainEntity(std::shared_ptr<EntityBase>& relItem, OBFResultDB& dbContext);
	void iterateMainEntityPost(std::shared_ptr<EntityBase>& e) ;
};

