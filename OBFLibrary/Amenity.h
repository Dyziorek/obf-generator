#pragma once
#include "mapobject.h"
#include "OBFRenderingTypes.h"

class Amenity :
	public MapObject
{
public:
	Amenity(void);
	virtual ~Amenity(void);

	std::string subType;
	std::unordered_map<std::string, std::string> additionalInfo;
	static std::string getUrl(EntityBase& entity);
	static std::list<Amenity> parseAmenities(OBFRenderingTypes renderer, EntityBase*,  std::list<Amenity> amenitiesList);

	static Amenity parseAmenity(EntityBase& entity, AmenityType type, std::string subtype, std::unordered_map<std::string, std::string> tagValues,	OBFRenderingTypes renderer);
	void setAdditionalInfo(std::unordered_map<std::string, std::string> otherInfo) { additionalInfo = otherInfo;}
	void setAdditionalInfo(std::string arg1, std::string arg2) { additionalInfo.insert(std::make_pair(arg1, arg2));}
};

