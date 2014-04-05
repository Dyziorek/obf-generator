#include "stdafx.h"
#include "OBFElementDB.h"
#include "OBFRenderingTypes.h"
#include "Amenity.h"

OBFpoiDB::OBFpoiDB(void)
{
}


OBFpoiDB::~OBFpoiDB(void)
{
}


void OBFpoiDB::indexRelations(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext)
{
	for (auto strId : entry->tags)
	{
		AmenityType amType = renderer.getAmenityTypeForRelation(strId.first, strId.second);
		if (!amType.isEmpty())
		{
			dbContext.loadNodesOnRelation(entry.get());
			for (auto entryRel : entry->entityIDs)
			{
			if (propagatedTags.find(entryRel.first) == propagatedTags.end())
			{
				propagatedTags.insert(std::make_pair(entryRel.first, std::map<std::string, std::string>()));
			}
			propagatedTags.find(entryRel.first)->second.insert(std::make_pair(strId.first, strId.second));
			}
		}
	}
}


void OBFpoiDB::iterateMainEntity(std::shared_ptr<EntityBase>& baseItem, OBFResultDB& dbContext)
{
	tempAmenityList.clear();
	std::shared_ptr<EntityRelation> relItem = std::dynamic_pointer_cast<EntityRelation, EntityBase>(baseItem);
	std::map<std::string, std::string> tags;
	if (propagatedTags.find(baseItem->id) != propagatedTags.end())
	{
		tags = propagatedTags.at(baseItem->id);
	}
	if (!tags.empty()) {
		std::map<std::string, std::string>::iterator iterator = tags.begin();
		while (iterator != tags.end()) {
			auto ts = *iterator;
			if (baseItem->getTag(ts.first) == "") {
				baseItem->putTag(ts.first, ts.second);
			}
		}
	}
	bool privateReg = baseItem->getTag("access") == "private"; 
	tempAmenityList = Amenity::parseAmenities(renderer, baseItem.get(), tempAmenityList);
	if (!tempAmenityList.empty() ) {
		if(relItem) {
			dbContext.loadNodesOnRelation(relItem.get());
		}
		for (Amenity a : tempAmenityList) {
			if(a.getType() == LEISURE.name && privateReg) {
				// don't index private swimming pools 
				continue;
			}
			// do not add that check because it is too much printing for batch creation
			// by statistic < 1% creates maps manually
			// checkEntity(e);
			MapObject::parseMapObject(&a, baseItem.get());
			if (a.getLatLon().first != -1000) {
				// do not convert english name
				// convertEnglishName(a);
				insertAmenityIntoPoi(a, dbContext);
			}
		}
	}
}

void OBFpoiDB::insertAmenityIntoPoi(Amenity amenity, OBFResultDB& dbContext) {
	dbContext.addBatch(amenity);
}

OBFtransportDB::OBFtransportDB(void)
{
}


OBFtransportDB::~OBFtransportDB(void)
{
}



OBFrouteDB::OBFrouteDB(void)
{
	routingTypes = MapRoutingTypes(renderer);
}


OBFrouteDB::~OBFrouteDB(void)
{
}

void OBFrouteDB::indexRelations(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext)
{
	indexHighwayRestrictions(entry, dbContext);

	std::map<std::string, std::string> propagated = routingTypes.getRouteRelationPropogatedTags(*entry.get());
	if (propagated.size() >0)
	{
		if (entry->entityIDs.size() == 0)
		{
			dbContext.loadRelationMembers(entry.get());
		}
		dbContext.loadNodesOnRelation(entry.get());

		for (auto entryRel : entry->entityIDs)
		{
			if (propagatedTags.find(entryRel.first) == propagatedTags.end())
			{
				propagatedTags.insert(std::make_pair(entryRel.first, std::map<std::string, std::string>()));
			}
			propagatedTags.find(entryRel.first)->second.insert(propagated.begin(), propagated.end());
		}
	}

}

void OBFrouteDB::indexHighwayRestrictions(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext)
{
	if (entry.get() != nullptr && entry->getTag("type") == "restriction")
	{
		std::string restriction = entry->getTag("restriction");
		if (!restriction.empty())
		{
			boost::algorithm::to_lower(restriction);
			byte type = -1;
			if (restriction == "no_right_turn")
				type = OBFRenderingTypes::RESTRICTION_NO_RIGHT_TURN;
			if (restriction == "no_left_turn")
				type = OBFRenderingTypes::RESTRICTION_NO_LEFT_TURN;
			if (restriction == "no_u_turn")
				type = OBFRenderingTypes::RESTRICTION_NO_U_TURN;
			if (restriction == "no_straight_on")
				type = OBFRenderingTypes::RESTRICTION_NO_STRAIGHT_ON;
			if (restriction == "only_right_turn")
				type = OBFRenderingTypes::RESTRICTION_ONLY_RIGHT_TURN;
			if (restriction == "only_left_turn")
				type = OBFRenderingTypes::RESTRICTION_ONLY_LEFT_TURN;
			if (restriction == "only_straight_on")
				type = OBFRenderingTypes::RESTRICTION_ONLY_STRAIGHT_ON;
			if (type != -1)
			{
				dbContext.loadRelationMembers(entry.get());
				dbContext.loadNodesOnRelation(entry.get());
				std::list<std::pair<int, __int64>> fromId = entry->getEntityIDforName("from");
				std::list<std::pair<int, __int64>> toId = entry->getEntityIDforName("to");
				if (!fromId.empty() && !toId.empty())
				{
					if (fromId.begin()->first == WAY_ID)
					{
						std::pair<int, __int64> fromIDItem = *fromId.begin();
						if (highwayRestrictions.find(fromIDItem.second) == highwayRestrictions.end())
						{
							highwayRestrictions.insert(std::make_pair(fromIDItem.second, std::list<_int64>()));
						}

						highwayRestrictions.find(fromIDItem.second)->second.push_back((toId.begin()->second << 3) | type);
					}
				}
			}
		}
	}
}


