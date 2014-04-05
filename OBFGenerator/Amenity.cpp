#include "stdafx.h"
#include "Amenity.h"


Amenity::Amenity(void)
{
}


Amenity::~Amenity(void)
{
}


std::list<Amenity> Amenity::parseAmenities (OBFRenderingTypes renderer, EntityBase* baseItem,  std::list<Amenity> amenitiesList)
{
	amenitiesList.clear();
	if (baseItem == nullptr)
		return amenitiesList;
	EntityRelation* relIt = dynamic_cast<EntityRelation*>(baseItem);
	bool isRelation = relIt != nullptr;

	std::list<std::map<std::string, std::string>> it = renderer.splitTagsIntoDifferentObjects(baseItem->tags);
	for (auto tags : it)
	{
		if (tags.size() != 0)
		{
			std::string mpoly = "";
			if (tags.find("type") != tags.end())
			{
				mpoly = tags.at("type");
			}
			bool purerelation = isRelation &&  mpoly != "multipolygon";
				for (auto e : tags) {
					AmenityType type = purerelation ? renderer.getAmenityTypeForRelation(e.first, e.second)
							: renderer.getAmenityType(e.first, e.second, false);
					if (!type.isEmpty()) {
						std::string subtype = renderer.getAmenitySubtype(e.first, e.second);
						Amenity a = Amenity::parseAmenity(*baseItem, type, subtype, tags, renderer);
						bool Notfound = true;
						for(Amenity b : amenitiesList)
						{
							if(b.getType() == a.getType() && a.subType < b.subType){
								Notfound = false;
								break;								
							}
						}

						if (Notfound && subtype != "no") {
							amenitiesList.push_back(a);
						}
					}
				}
		}
	}
	return amenitiesList;
}


Amenity Amenity::parseAmenity (EntityBase& entity, 
							  AmenityType type, 
							  std::string subtype, 
							  std::map<std::string, std::string> tagValues,
							  OBFRenderingTypes renderer ) 
{
	std::string info;
	Amenity am;
	parseMapObject(&am, &entity);
	if(tagValues.empty()) {
		tagValues = entity.tags;
	}
	am.setType(type.getDefaultTag());
	am.subType = subtype;
	am.setAdditionalInfo(renderer.getAmenityAdditionalInfo(tagValues, type, subtype));
	am.setAdditionalInfo("website", "http://en/wikipedia.org");
	return am;
}


