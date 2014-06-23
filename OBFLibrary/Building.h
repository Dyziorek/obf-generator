#pragma once
#include "mapobject.h"
class Building :
	public MapObject
{
public:
	enum BuildingInterpolation { NONE = 0, ALL = -1, EVEN = -2, ODD = -3, ALPHA = -4};
	BuildingInterpolation interpType;
	int getInterpValue();
	void setInterpType(std::string typeVal);
	Building(void);
	~Building(void);
	void setBuilding(EntityBase* obj);
	std::string postCode;
	std::string name2;
	LatLon location2;
	int interval;
	std::string getName2() {return name2;}
	LatLon getLatLon2(){return location2;}
	int getInterval() { return interval;}
	bool operator<(Building const& op) {return true;}
};

