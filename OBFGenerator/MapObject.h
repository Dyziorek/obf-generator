#pragma once
#include "stdafx.h"

class MapObject
{
public:
	MapObject(void) {name = ""; type = ""; lat = -1000; lon = -1000;}
	virtual ~MapObject(void);
	std::string getName(){return name;}
	std::string getEnName(){return enName;}
	void setName(std::string tags) { name = tags;}
	void setEnName(std::string tags) { enName= tags;}

	void setLocation(double rlat, double rlon) { lat = rlat; lon = rlon;}
	
	void setId(__int64 objId) {id = objId;}
	std::pair<double , double> getLatLon() { return std::make_pair(lat, lon);}

	static void parseMapObject(MapObject* mo, EntityBase* e);

	 static void setNameFromRef(MapObject& mo, EntityBase& e);

	static void setNameFromOperator(MapObject& mo, EntityBase& e);

	__int64 getID(void) {return id;}
	void setType(std::string Rtype) { type = Rtype;}
	std::string getType() { return type;}

private:
	std::string name;
	std::string enName;
	__int64 id;
	double lat;
	double lon;
	std::string type;
};

class CityObj :
	public MapObject
{
public:
	CityObj(void);
	virtual ~CityObj(void);

	bool isAlwaysVisible;
};