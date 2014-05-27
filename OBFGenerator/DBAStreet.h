#pragma once

class SimpleStreet
{
private:
	const long id;
	const std::string name;
	const std::string cityPart;
	std::pair<double, double> location;
public:
	SimpleStreet(long id, std::string name, std::string cityPart, double latitude, double longitude)
		: id(id), name(name), cityPart(cityPart)
	{
		location = std::make_pair(latitude,longitude);
	}

	SimpleStreet(long id, std::string name, std::string cityPart,
		std::pair<double, double> location)  : id(id), name(name), cityPart(cityPart)
	{
		this->location = location;
	}
	std::string getCityPart() {
			return cityPart;
		}
	long getId() {
			return id;
		}
	std::string getName() {
			return name;
		}
	std::pair<double, double> getLocation() {
			return location;
	}

};


class DBAStreet
{
private:
	OBFResultDB& workCtx;
	DBAStreet(void);
	static __int64 streetIdSeq;
public:
	DBAStreet(OBFResultDB& dbContext);
	std::unique_ptr<SimpleStreet> findStreet(std::string name,CityObj city,std::string cityPart);
	std::unique_ptr<SimpleStreet> findStreet(std::string name,CityObj city);
	__int64 insertStreet(std::string name,std::string nameEn,LatLon location,CityObj city,std::string cityPart);
	bool findBuilding(std::shared_ptr<EntityBase> house);
	void writeBuilding(std::set<long long> idsOfStreet, Building building);
	~DBAStreet(void);
};

