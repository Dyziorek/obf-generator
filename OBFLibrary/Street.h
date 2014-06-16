#pragma once
#include "mapobject.h"
#include "Building.h"



class Street :
	public MapObject
{
protected:
	std::list<Building> buildings; 
	std::list<Street> intersectedStreets;
	CityObj city;
public:
	Street(void);
	~Street(void);

	bool operator==(const Street & x) const;

	 Street(CityObj city);
	
	 void addBuilding(Building building);
	
	std::list<Street> getIntersectedStreets();
	
	 void addIntersectedStreet(Street s);
	
	 void addBuildingCheckById(Building building);
	
	 std::list<Building> getBuildings();
	
	void setName(std::string name);
	
	std::string getNameWithoutCityPart(bool en);
	
	CityObj getCity();
	
	void sortBuildings();

	void mergeWith(Street street);

};

namespace boost
{
	template<>
	struct hash<Street>
	{
		std::size_t operator()(const Street& hashWork) const
		{
			size_t seed;
			Street& objCon = const_cast<Street&>(hashWork);
			hash_combine(seed, objCon.getName());
			hash_combine(seed, objCon.getEnName());
			hash_combine(seed, objCon.getID());
			for (Building build :objCon.getBuildings())
			{
				hash_combine(seed, build.getName());
				hash_combine(seed, build.getID());
			}
			return seed;
		}
	};
}