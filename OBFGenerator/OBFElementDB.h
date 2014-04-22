#pragma once
#include "OBFResultDB.h"
#include "MapRoutingTypes.h"

class OBFpoiDB :
	public OBFResultDB
{
	OBFRenderingTypes renderer;
	std::map<long long, std::map<std::string, std::string>> propagatedTags;
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
public:
	OBFrouteDB(void);
	virtual ~OBFrouteDB(void);
	void indexHighwayRestrictions(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext);
	void indexRelations(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext);
	std::map<__int64, std::list<__int64>> highwayRestrictions;
	std::map<long long, std::map<std::string, std::string>> propagatedTags;
	OBFRenderingTypes renderer;
	MapRoutingTypes routingTypes;
};

class OBFAddresStreetDB :
	public OBFResultDB
{
public:
	std::set<__int64> visitedBoundaryWays;
	std::set<std::shared_ptr<MultiPoly>> boundaries;
	OBFAddresStreetDB(void);
	virtual ~OBFAddresStreetDB(void);
	void indexBoundary(std::shared_ptr<EntityBase>& baseItem, OBFResultDB& dbContext);
};