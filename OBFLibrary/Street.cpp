#include "stdafx.h"
#include "Building.h"
#include "Street.h"



Street::Street(void)
{
}


Street::~Street(void)
{
}


	 Street::Street(CityObj city) {
		this->city = city;
	}
	
	 void Street::addBuilding(Building building){
		 buildings.push_back(building);
	}
	
	std::list<Street> Street::getIntersectedStreets() {
		return intersectedStreets;
	}
	
	 void Street::addIntersectedStreet(Street s){
		intersectedStreets.push_back(s);
	}
	
	 void Street::addBuildingCheckById(Building building){
		for(Building b : buildings) {
			if(b.getID() == building.getID()){
				return;
			}
		}
		buildings.push_back(building);
	}
	
	 std::list<Building> Street::getBuildings() {
		return buildings;
	}
	
	void Street::setName(std::string name) {
		if (name == getName()) {
			return;
		}
		if (city.getLatLon().first != -1000 && city.streets[getName()].getName() == getName()) {
			city.unregisterStreet(getName());
			((MapObject*)this)->setName(name);
			Street s = city.registerStreet(*this);
		} else {
			((MapObject*)this)->setName(name);
		}
	}
	
	std::string Street::getNameWithoutCityPart(bool en) {
		std::string nm = getName();
		int t = nm.find_last_of('(');
		if(t > 0) {
			return nm.substr(0, t);
		}
		return nm;
		
	}
	
	CityObj Street::getCity() {
		return city;
	}
	
	void Street::sortBuildings(){
		buildings.sort();
	}

	void Street::mergeWith(Street street) {
		buildings.insert(buildings.end(),  street.getBuildings().begin(), street.getBuildings().end());
	}

bool Street::operator==(const Street & x) const
{
	Street& obj1 = const_cast<Street&>(*this);
	Street& obj2 = const_cast<Street&>(x);

	return obj1.getName() == obj2.getName() && obj1.getID() == obj2.getID();
}