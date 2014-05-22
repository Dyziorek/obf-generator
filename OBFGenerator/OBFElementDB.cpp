#include "stdafx.h"
#include "OBFElementDB.h"
#include "OBFRenderingTypes.h"
#include "Amenity.h"
#include "ArchiveIO.h"

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
				propagatedTags.insert(std::make_pair(entryRel.first, boost::unordered_map<std::string, std::string>()));
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
	boost::unordered_map<std::string, std::string> tags;
	if (propagatedTags.find(baseItem->id) != propagatedTags.end())
	{
		tags = propagatedTags.at(baseItem->id);
	}
	if (!tags.empty()) {
		boost::unordered_map<std::string, std::string>::iterator iterator = tags.begin();
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
			dbContext.loadRelationMembers(relItem.get());	
			dbContext.loadNodesOnRelation(relItem.get());
		}

		if (tempAmenityList.size() > 3)
		{
			privateReg = true;
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

	boost::unordered_map<std::string, std::string> propagated = routingTypes.getRouteRelationPropogatedTags(*entry.get());
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
				propagatedTags.insert(std::make_pair(entryRel.first, boost::unordered_map<std::string, std::string>()));
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


void OBFrouteDB::iterateMainEntity(std::shared_ptr<EntityBase>& it, OBFResultDB& dbCtx)
{
	std::shared_ptr<EntityWay> wayItem = std::dynamic_pointer_cast<EntityWay, EntityBase>(it);
	if (wayItem) {
		auto tagsIt = propagatedTags.find(wayItem->id);
		if (tagsIt != propagatedTags.end()) {
			boost::unordered_map<std::string, std::string>::iterator itTag = tagsIt->second.begin();
				while (itTag != tagsIt->second.end()) {
					auto ts = *itTag;
					if (wayItem->getTag(ts.first) == "") {
						wayItem->putTag(ts.first, ts.second);
					}
				}
			}
			
			boolean encoded = routingTypes.encodeEntity(*wayItem, outTypes, names) ;
			if (encoded) {
				// Load point with  tags!
				dbCtx.loadWays(wayItem.get());
				routingTypes.encodePointTypes(*wayItem, pointTypes);
				if(wayItem->nodes.size() >= 2) {
					addWayToIndex(wayItem->id, wayItem->nodes, dbCtx, routeTree, false);
				}
			}
			encoded = routingTypes.encodeBaseEntity(*wayItem, outTypes, names) && wayItem->nodes.size() >= 2;
			if (encoded ) {
				std::vector<std::shared_ptr<EntityNode>> result;
				std::vector<std::shared_ptr<EntityNode>> source = wayItem->nodes;
				std::vector<bool> kept = OsmMapUtils::simplifyDouglasPeucker(source, 11 /*zoom*/+ 8 + 1 /*smoothness*/, 3, result, false);
				int indexToInsertAt = 0;
				int originalInd = 0;				
				for(int i = 0; i < kept.size(); i ++) {
					std::shared_ptr<EntityNode> n = source[i];
					if(n) {
						long y31 = MapUtils::get31TileNumberY(n->lat);
						long x31 = MapUtils::get31TileNumberX(n->lon);
						long long point = (x31 << 31) + y31;
						registerBaseIntersectionPoint(point, !kept[i], wayItem->id, indexToInsertAt, originalInd);
						originalInd++;
						if(kept[i]) {
							indexToInsertAt ++;
						}
					}
				}
				
				
				addWayToIndex(wayItem->id, result, dbCtx, baserouteTree, true);
				//generalizeWay(e);

			}
		}
		

}


void OBFrouteDB::addWayToIndex(long long id, std::vector<std::shared_ptr<EntityNode>>& nodes, OBFResultDB& dbContext, RTree rTree, bool base)  {
		boolean init = false;
		int minX = INT_MAX;
		int maxX = 0;
		int minY = INT_MAX;
		int maxY = 0;
		

		std::stringstream bcoordinates;
		std::stringstream bpointIds;
		std::stringstream bpointTypes;
		std::stringstream btypes;

		try {
			for (int j = 0; j < outTypes.size(); j++) {
				writeSmallInt(btypes, outTypes[j]);
			}

			for (std::shared_ptr<EntityNode> n : nodes) {
				if (n) {
					// write id
					writeLongInt(bpointIds, n->id);
					// write point type
					std::map<__int64, std::vector<int>>::iterator typesit = pointTypes.find(n->id);
						if (typesit != pointTypes.end()) {
							std::vector<int> types = typesit->second;
						for (int j = 0; j < types.size(); j++) {
							writeSmallInt(bpointTypes, types[j]);
						}
					}
					writeSmallInt(bpointTypes, 0);
					// write coordinates
					int y = MapUtils::get31TileNumberY(n->lat);
					int x = MapUtils::get31TileNumberX(n->lon);
					minX = min(minX, x);
					maxX = max(maxX, x);
					minY = min(minY, y);
					maxY = max(maxY, y);
					init = true;
					writeInt(bcoordinates, x);
					writeInt(bcoordinates, y);
				}
			}

		} catch (std::bad_exception est) {
			
		}
		if (init) {
			// conn.prepareStatement("insert into route_objects(id, types, pointTypes, pointIds, pointCoordinates, name) values(?, ?, ?, ?, ?, ?, ?)");


			dbContext.addBatchRoute(id, btypes, bpointTypes, bpointIds, bcoordinates, encodeNames(names), base);
			rTree.insertBox(minX, minY, maxX, maxY, id, std::list<long>());

		}
	}

  long OBFrouteDB::SHIFT_INSERT_AT = 12;
  long OBFrouteDB::SHIFT_ORIGINAL = 16;
  long OBFrouteDB::SHIFT_ID = 64 - (SHIFT_INSERT_AT + SHIFT_ORIGINAL);

 void OBFrouteDB::registerBaseIntersectionPoint(long long pointLoc, bool registerId,long long wayId, int insertAt, int originalInd) {
		
		boost::unordered_map<__int64, __int64>::iterator exNode = basemapRemovedNodes.find(pointLoc);
		if(insertAt > (1ll << SHIFT_INSERT_AT)) {
			throw new std::bad_exception("Way index too big");
		}
		if(originalInd > (1ll << SHIFT_ORIGINAL)) {
			throw new std::bad_exception("Way index 2 too big");
		}
		if(wayId > (1ll << SHIFT_ID)) {
			throw new std::bad_exception("Way id too big");
		}
		long long genKey = registerId ? ((wayId << (SHIFT_ORIGINAL+SHIFT_INSERT_AT)) + (originalInd << SHIFT_INSERT_AT) + insertAt) : -1ll; 
		if(exNode == basemapRemovedNodes.end()) {
			basemapRemovedNodes.insert(std::make_pair(pointLoc, genKey));
		} else {
			if(exNode->second != -1) {
				putIntersection(pointLoc, exNode->second);
			}
			basemapRemovedNodes.insert(std::make_pair(pointLoc, -1ll));
			if(genKey != -1) {
				putIntersection(pointLoc, genKey);
			}
		}
		
	}

 void OBFrouteDB::putIntersection(long long  point, long long wayNodeId) {
		if(wayNodeId != -1){
//			long x = point >> 31;
//			long y = point - (x << 31);
//			System.out.println("Put intersection at " + (float) MapUtils.get31LatitudeY((int) y) + " " + (float)MapUtils.get31LongitudeX((int) x));
			long SHIFT = SHIFT_INSERT_AT + SHIFT_ORIGINAL;
			int ind = (int) (wayNodeId & ((1 << SHIFT) - 1));
			long long wayId = wayNodeId >> SHIFT;
			if(basemapNodesToReinsert.find(wayId) == basemapNodesToReinsert.end()) {
				basemapNodesToReinsert.insert(std::make_pair(wayId, RouteMissingPoints()));
			}
			RouteMissingPoints mp = basemapNodesToReinsert.at(wayId);
			mp.pointsMap.insert(std::make_pair(ind, point));
		}
		
	}

 std::string OBFrouteDB::encodeNames(std::map<MapRouteType, std::string> tempNames) {
		std::stringstream strm;
		for (std::pair<MapRouteType, std::string> e : tempNames) {
			if (e.second.size()) {
				strm << "~";
				strm << e.first.getInternalId();
				strm << e.second;
			}
		}
		return  strm.str();
	}

 void OBFrouteDB::processLowLevelWays(OBFResultDB& dbContext){

 }

OBFAddresStreetDB::OBFAddresStreetDB(void)
{
}


OBFAddresStreetDB::~OBFAddresStreetDB(void)
{
}

void OBFAddresStreetDB::indexAddressRelation(std::shared_ptr<EntityRelation>& i, OBFResultDB& dbContext) {
	if (i->getTag("type") == "street" || i->getTag("type") == "associatedStreet") 
	{ 
			
			std::pair<double,double> LatLon(-1000,-1000);
			std::string streetName = "";
			boost::unordered_set<std::string> isInNames;
			dbContext.loadRelationMembers(i.get());
			dbContext.loadNodesOnRelation(i.get());
			
			std::vector<std::shared_ptr<EntityBase>> members;
			for(auto relationMember : i->relations)
			{
				if (relationMember.second == "street")
				{
					members.push_back(relationMember.first.second);
				}
			}
			//Collection<Entity> members = i->relations;
			for(std::shared_ptr<EntityBase> street : members) { // find the first street member with name and use it as a street name
				std::string name = street->getTag("name");
				if (name != "") {
					streetName = name;
					LatLon = street->getLatLon();
					isInNames = street->getIsInNames();
					break;
				}
			}
			
			if (streetName == "") { // use relation name as a street name
				streetName = i->getTag("name");
				LatLon = i->relations.begin()->first.second->getLatLon(); // get coordinates from any relation member
				isInNames = i->getIsInNames();
			}
			
			if (streetName != "") {
				boost::unordered_set<long long> idsOfStreet = getStreetInCity(isInNames, streetName, "", LatLon, dbContext);
				if (!idsOfStreet.isEmpty()) {
					Collection<Entity> houses = i.getMembers("house"); // both house and address roles can have address
					houses.addAll(i.getMembers("address"));
					for (Entity house : houses) {
						std::string hname = house.getTag(OSMTagKey.ADDR_HOUSE_NAME);
						if(hname == null) {
							hname = house.getTag(OSMTagKey.ADDR_HOUSE_NUMBER);
						}
						if (hname == null)
							continue;
						
						if (!streetDAO.findBuilding(house)) {
							// process multipolygon (relation) houses - preload members to create building with correct latlon
							if (house instanceof Relation)
								ctx.loadEntityRelation((Relation) house);
							Building building = EntityParser.parseBuilding(house);
							if (building.getLocation() == null) {
								log.warn("building with empty location! id: " + house.getId());
							}
							building.setName(hname);
							
							streetDAO.writeBuilding(idsOfStreet, building);
						}
					}
				}
			}
		}
	}


std::set<long long> OBFAddresStreetDB::getStreetInCity(boost::unordered_set<std::string> isInNames, std::string name, std::string nameEn, std::pair<double,double> location, OBFResultDB& dbContext) {
		if (name == "" || location.first == -1000) {
			return std::set<long long>();
		
		}
		boost::trim(name);
		boost::unordered_set<CityObj> result;
		std::list<CityObj> nearestObjects;
		std::vector<CityObj> objects = cityManager.getClosestObjects(location.first,location.second);
		nearestObjects.insert(nearestObjects.end(), objects.begin(), objects.end());
		std::vector<CityObj> objects = townManager.getClosestObjects(location.first,location.second);
		nearestObjects.insert(nearestObjects.end(), objects.begin(), objects.end());
		//either we found a city boundary the street is in
		for (CityObj c : nearestObjects) {
			std::shared_ptr<MultiPoly> boundary = cityBoundaries[c];
			if (isInNames.find(c.getName()) != isInNames.end() || (boundary && boundary->containsPoint(location))) {
				result.insert(c);
			}
		}
		// or we need to find closest city
		nearestObjects.sort([&location](CityObj& param1, CityObj& param2){
			double dist1 = MapUtils::getDistance(location.first, location.second, param1.getLatLon().first, param1.getLatLon().second) / param1.getRadius();
			double dist2 = MapUtils::getDistance(location.first, location.second, param2.getLatLon().first, param2.getLatLon().second) / param2.getRadius();
			return dist1 < dist2;
		});

		/*Collections.sort(nearestObjects, new Comparator<City>() {
			@Override
			public int compare(City c1, City c2) {
				double r1 = relativeDistance(location, c1);
				double r2 = relativeDistance(location, c2);
				return Double.compare(r1, r2);
			}
		});*/
		for(CityObj c : nearestObjects) {
			if(MapUtils::getDistance(location.first, location.second, c.getLatLon().first, c.getLatLon().second) / c.getRadius() > 0.2) {
				if(result.size() == 0) {
					result.insert(c);
				}
				break;
			} else if(result.find(c) == result.end()) {
				// city doesn't have boundary or there is a mistake in boundaries and we found nothing before
				if(cityBoundaries.find(c) == cityBoundaries.end() || result.size() == 0) {
					result.insert(c);
				}
			}
		}
		//return registerStreetInCities(name, nameEn, location, result);

		if (result.size()) {
			return std::set<long long>();
		}
		if ( boost::empty(nameEn)) {
			nameEn = boost::trim_copy(name);
		}

		std::set<long long> values;
		for (CityObj city : result) {
			long streetId = getOrRegisterStreetIdForCity(name, nameEn, location, city);
			values.add(streetId);
		}
	}

// from java methods: net.osmand.data.preparation.address.IndexAddressCreator:indexBoundariesRelation(Entity, OsmDbAccessorContext) && extractBoundary
void OBFAddresStreetDB::indexBoundary(std::shared_ptr<EntityBase>& baseItem, OBFResultDB& dbContext)
{
	std::shared_ptr<MultiPoly> boundary = extractBoundary(baseItem, dbContext);
	if (boundary && boundary->isValid())
	{
		std::unique_ptr<std::pair<double, double>> latLon = boundary->getCenterPoint();
		std::list<CityObj> citiesToLookup;
		std::vector<CityObj> closestCities = cityManager.getClosestObjects(latLon->first, latLon->second, 3);
		citiesToLookup.insert(citiesToLookup.end(), closestCities.begin(), closestCities.end());
		closestCities = townManager.getClosestObjects(latLon->first, latLon->second, 3);
		citiesToLookup.insert(citiesToLookup.end(), closestCities.begin(), closestCities.end());

		std::unique_ptr<CityObj> foundCity;
		std::string boundaryName = boost::to_lower_copy(boundary->polyName);
		std::string altBoundaryName = boundary->polyAltName == "" ? "" : boost::to_lower_copy(boundary->polyAltName);
		if(boundary->centerID != -1) {
				for (CityObj c : citiesToLookup) {
					if (c.getID() == boundary->centerID) {
						foundCity.reset(&c);
						break;
					}
				}
			}
			if(!foundCity) {
				for (CityObj c : citiesToLookup) {
					if (( boost::iequals(boundaryName,c.getName()) || boost::iequals(altBoundaryName,c.getName())) 
						&& boundary->containsPoint(c.getLatLon())) {
						foundCity.release();
						foundCity.reset(&c);
						break;
					}
				}
			}
			// We should not look for similarities, this can be 'very' wrong (we can find much bigger region)....
			// but here we just find what is the center of the boundary
			// False case : London Borough of Richmond upon Thames (bigger) -> Richmond!
			if (!foundCity) {
				for (CityObj c : citiesToLookup) {
					std::string lower = boost::to_lower_copy(c.getName());
					if (boost::contains(boundaryName, lower) || boost::contains(altBoundaryName, lower)) {
						if (boundary->containsPoint(c.getLatLon())) {
							foundCity.release();
							foundCity.reset(&c);
							break;
						}
					}
				}
			}
			// Monaco doesn't have place=town point, but has boundary with tags & admin_level
			// It could be wrong if the boundary doesn't match center point
			if(!foundCity /*&& !boundary.hasAdminLevel() */&& 
				(boundary->polyType == "town" ||
					boundary->polyType ==  "hamlet" || 
					boundary->polyType ==  "suburb" || 
					boundary->polyType ==  "village")) {

				std::shared_ptr<EntityRelation> relItem = std::dynamic_pointer_cast<EntityRelation, EntityBase>(baseItem);
				if(relItem) {
					dbContext.loadRelationMembers(relItem.get());
					if (relItem->entityIDs.size() > 0)
					{
						dbContext.loadNodesOnRelation(relItem.get());
					}
				}
				foundCity.reset(&createMissingCity(baseItem, boundary->polyType));
				boundary->centerID = foundCity->getID();
			}
			if (foundCity) {
				putCityBoundary(boundary, *foundCity);
			} else {
				
				//logBoundaryChanged(boundary, null);
				notAssignedBoundaries.insert(boundary);
			}
			

			std::list<CityObj> list(1);
			for (auto c : cities) {
				if (boundary->containsPoint(c.second.getLatLon())) {
					list.push_back(c.second);
				}
			}
			if(list.size() > 0) {
				boundaryToContainingCities[boundary] = list;
			}
			

	}
}

void OBFAddresStreetDB::tryToAssignBoundaryToFreeCities()
{
	int smallestAdminLevel = 7; //start at level 8 for now...
		for (auto c : cities) {

			std::shared_ptr<MultiPoly> cityB = cityBoundaries[c.second];
			if (!cityB && (c.second.getType() == "city" || c.second.getType() == "town")) {
				auto location = c.second.getLatLon();
				std::shared_ptr<MultiPoly> smallestBoundary = nullptr;
				// try to found boundary
				for (std::shared_ptr<MultiPoly> b : notAssignedBoundaries) {
					if (b->level >= smallestAdminLevel) {
						if (b->containsPoint(location)) {
							// the bigger the admin level, the smaller the boundary :-)
							smallestAdminLevel = b->level;
							smallestBoundary = b;
						}
					}
				}
				if (smallestBoundary) {
					putCityBoundary(smallestBoundary, c.second);
					notAssignedBoundaries.erase(smallestBoundary);
				}
			}
		}
}

	int OBFAddresStreetDB::getCityBoundaryImportance(std::shared_ptr<MultiPoly> b, CityObj c) {
		bool nameEq = boost::iequals(b->polyName, c.getName());
		if(!boost::empty(b->polyAltName) && !nameEq) {
			nameEq = boost::iequals(b->polyAltName, c.getName());
		}
		bool cityBoundary = b->polyType != "";
		// max 10
		int adminLevelImportance = 5; 
		{
			if(b->level != -1) {
				int adminLevel = b->level;
				if(adminLevel == 8) {
					adminLevelImportance = 1;
				} else if(adminLevel == 7) {
					adminLevelImportance = 2;
				} else if(adminLevel == 6) {
					adminLevelImportance = 3;
				} else if(adminLevel == 9) {
					adminLevelImportance = 4;
				} else if(adminLevel == 10) {
					adminLevelImportance = 5;
				} else {
					adminLevelImportance = 6;
				}
			}
		}
		
		if(nameEq) {
			if(cityBoundary) {
				return 0;
			} else if(c.getID() == b->centerID || 
				!b->centerID != -1){
				return adminLevelImportance;
			}
			return 10 + adminLevelImportance;
		} else {
			if(c.getID() == b->centerID) {
				return 20 + adminLevelImportance;
			} else {
				return 30  + adminLevelImportance;
			}
		}
	}



std::shared_ptr<MultiPoly> OBFAddresStreetDB::putCityBoundary(std::shared_ptr<MultiPoly> boundary, CityObj cityFound) {
	std::shared_ptr<MultiPoly> oldBoundary = cityBoundaries[cityFound];
		if(!oldBoundary) {
			cityBoundaries[cityFound] = boundary;
			//logBoundaryChanged(boundary, cityFound);
			return oldBoundary;
		} else if (oldBoundary->level == boundary->level
				&& oldBoundary != boundary
				&& boost::iequals(boundary->polyName, oldBoundary->polyName)) {
					oldBoundary->inRing.insert(oldBoundary->inRing.end(), boundary->inRing.begin(), boundary->inRing.end());
					oldBoundary->outRing.insert(oldBoundary->outRing.end(), boundary->outRing.begin(), boundary->outRing.end());
					oldBoundary->updateRings();
			return oldBoundary;
		} else {
			int old = getCityBoundaryImportance(oldBoundary, cityFound);
			int n = getCityBoundaryImportance(boundary, cityFound);
			if (n < old) {
				cityBoundaries[cityFound] = boundary;
				//logBoundaryChanged(boundary, cityFound);
			}
			return oldBoundary;
		}
	}
std::shared_ptr<MultiPoly> OBFAddresStreetDB::extractBoundary(std::shared_ptr<EntityBase>& baseItem, OBFResultDB& dbContext)
{
	std::shared_ptr<EntityRelation> relItem = std::dynamic_pointer_cast<EntityRelation, EntityBase>(baseItem);
	std::shared_ptr<EntityWay> wayItem = std::dynamic_pointer_cast<EntityWay, EntityBase>(baseItem);
	
	if (wayItem.get() == nullptr && relItem.get() == nullptr)
		return std::shared_ptr<MultiPoly>();

	

	BOOL administrative = (baseItem->getTag("boundary") == "administrative");
	if (administrative  || baseItem->getTag("place") != "")
	{
		if (wayItem.get() != nullptr)
		{
			if (visitedBoundaryWays.find(wayItem->id) != visitedBoundaryWays.end())
				return std::shared_ptr<MultiPoly>();
		}
		std::string boundName = baseItem->getTag("name");
		if (relItem != nullptr)
		{
			dbContext.loadRelationMembers(relItem.get());
			if (relItem->entityIDs.size() > 0)
			{
				dbContext.loadNodesOnRelation(relItem.get());
			}
		}
		__int64 centrID = 0;
		std::shared_ptr<MultiPoly> polyline(new MultiPoly);
		if (relItem != nullptr)
		{
			for(auto entityItem : relItem->relations)
			{
				if (entityItem.first.first == 2)
				{
					std::shared_ptr<EntityRelation> relSubItem = std::dynamic_pointer_cast<EntityRelation, EntityBase>(entityItem.first.second);
					if (relSubItem->entityIDs.size() > 0)
					{
						dbContext.loadNodesOnRelation(relSubItem.get());
						for(auto innerEntityItem : relSubItem->relations)
						{
							if (innerEntityItem.first.first == 1)
							{
								bool inner = (innerEntityItem.second == "inner");
								std::shared_ptr<EntityWay> wayPtr = std::dynamic_pointer_cast<EntityWay>(innerEntityItem.first.second);
								if (inner)
								{
									polyline->inWays.push_back(wayPtr);
								}
								else
								{
									polyline->outWays.push_back(wayPtr);
									if (wayPtr->getTag("name") == boundName)
									{
										visitedBoundaryWays.insert(wayPtr->id);
									}
								}
							}
							else if (innerEntityItem.first.first == 0)
							{
								if (innerEntityItem.second == "admin_centre" || innerEntityItem.second == "admin_center")
								{
									centrID = innerEntityItem.first.second->id;
								}
								else if (innerEntityItem.second == "label")
								{
									centrID = innerEntityItem.first.second->id;
								}
							}
						}
					}
				}
				else if (entityItem.first.first == 1)
				{
					polyline->outWays.push_back(std::dynamic_pointer_cast<EntityWay, EntityBase>(entityItem.first.second));
				}
			}
		}
		else if (wayItem != nullptr)
		{
			polyline->outWays.push_back(wayItem);
		}
		polyline->build();
		polyline->centerID = centrID;
		polyline->id = baseItem->id;
		polyline->polyName = boundName;
		polyline->polyAltName = baseItem->getTag("short_name");
		polyline->polyType = baseItem->getTag("place");
		if (baseItem->getTag("admin_level") != "")
		{
			std::string text = baseItem->getTag("admin_level");
			try
			{
				polyline->level = boost::lexical_cast<int>(text);
			}
			catch (boost::bad_lexical_cast& lexx)
			{
				polyline->level = 7;
			}
		}
		// only named boundaries are registering
		if (boundName != "")
		{
			boundaries.insert(polyline);
		}

		return polyline;
	}
	return std::shared_ptr<MultiPoly>();
}

void OBFAddresStreetDB::iterateOverCity(std::shared_ptr<EntityNode>& cityNode)
{
	std::shared_ptr<EntityNode> ptrNode = cityNode;
	std::string placeType = boost::to_upper_copy(cityNode->getTag("place"));
	CityObj objCity;
	if (placeType == "CITY" ||placeType ==  "TOWN" )
	{
		SaverCityNode(ptrNode.get(), cityManager);
		objCity.setType(placeType);
	}
	else if (placeType ==  "VILLAGE" ||placeType ==  "HAMLET" ||placeType ==  "SUBURB" ||placeType ==  "DISTRICT")
	{
		SaverCityNode(ptrNode.get(), townManager);
		objCity.setType(placeType);
	}

				
	objCity.setId(cityNode->id);
	MapObject::parseMapObject(&objCity, cityNode.get());
	//objCity.setLocation(ptrNode->lat, ptrNode->lon);

	if (cityNode->getTag("capital") == "yes")
	{
		objCity.isAlwaysVisible = true;
		objCity.setType("CITY");
	}

	cities.insert(std::make_pair(ptrNode, objCity));

}

 CityObj OBFAddresStreetDB::createMissingCity(std::shared_ptr<EntityBase>& cityNode, std::string t) {
	CityObj objCity;
	std::string placeType = boost::to_upper_copy(cityNode->getTag("place"));
	if (placeType == "CITY" ||placeType ==  "TOWN" )
	{
		SaverCityNode(cityNode.get(), cityManager);
		objCity.setType(placeType);
	}
	else if (placeType ==  "VILLAGE" ||placeType ==  "HAMLET" ||placeType ==  "SUBURB" ||placeType ==  "DISTRICT")
	{
		SaverCityNode(cityNode.get(), townManager);
		objCity.setType(placeType);
	}

				
	objCity.setId(cityNode->id);
	MapObject::parseMapObject(&objCity, cityNode.get());
	

	
	return objCity;
	}

void OBFAddresStreetDB::storeCities(OBFResultDB& dbContext)
{

	//"insert into city (id, latitude, longitude, name, name_en, city_type) values (?1, ?2, ?3, ?4, ?5, ?6)"
	char* errMsg;
	int cityList = 0;
	int SqlCode;
	sqlite3_exec(dbContext.dbAddrCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
	for (auto mapCity: cities)
	{
		sqlite3_bind_int64(dbContext.cityStmt, 1, mapCity.second.getID());
		sqlite3_bind_double(dbContext.cityStmt, 2, mapCity.second.getLatLon().first);
		sqlite3_bind_double(dbContext.cityStmt, 3, mapCity.second.getLatLon().second);
		sqlite3_bind_text(dbContext.cityStmt, 4, mapCity.second.getName().c_str(), mapCity.second.getName().size(), SQLITE_TRANSIENT);
		std::shared_ptr<EntityNode> nodeElem = mapCity.first;
		sqlite3_bind_text(dbContext.cityStmt, 5, nodeElem->getTag("em_name").c_str(), nodeElem->getTag("em_name").size(), SQLITE_TRANSIENT);
		sqlite3_bind_text(dbContext.cityStmt, 6, mapCity.second.getType().c_str(), mapCity.second.getType().size(), SQLITE_TRANSIENT);

		SqlCode = sqlite3_step(dbContext.cityStmt);
		if (SqlCode != SQLITE_DONE)
		{
			//NodeElems = -100;
		}
		SqlCode = sqlite3_clear_bindings(dbContext.cityStmt);
		SqlCode = sqlite3_reset(dbContext.cityStmt);

		cityList++;
		if (cityList > 10000)
		{
			sqlite3_exec(dbContext.dbAddrCtx, "END TRANSACTION", NULL, NULL, &errMsg);
			sqlite3_exec(dbContext.dbAddrCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
			cityList = 0;
		}
	}
	if (cityList > 0)
	{
		sqlite3_exec(dbContext.dbAddrCtx, "END TRANSACTION", NULL, NULL, &errMsg);
	}
}

void OBFAddresStreetDB::storeCity(std::shared_ptr<EntityNode>& cityNode, CityObj objData, OBFResultDB& dbContext)
{

	//"insert into city (id, latitude, longitude, name, name_en, city_type) values (?1, ?2, ?3, ?4, ?5, ?6)"
	char* errMsg;
	int cityList = 0;
	int SqlCode;
	sqlite3_exec(dbContext.dbAddrCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
	
	{
		sqlite3_bind_int64(dbContext.cityStmt, 1, objData.getID());
		sqlite3_bind_double(dbContext.cityStmt, 2, objData.getLatLon().first);
		sqlite3_bind_double(dbContext.cityStmt, 3, objData.getLatLon().second);
		sqlite3_bind_text(dbContext.cityStmt, 4, objData.getName().c_str(), objData.getName().size(), SQLITE_TRANSIENT);
		std::shared_ptr<EntityNode> nodeElem = cityNode;
		sqlite3_bind_text(dbContext.cityStmt, 5, nodeElem->getTag("em_name").c_str(), nodeElem->getTag("em_name").size(), SQLITE_TRANSIENT);
		sqlite3_bind_text(dbContext.cityStmt, 6, objData.getType().c_str(), objData.getType().size(), SQLITE_TRANSIENT);

		SqlCode = sqlite3_step(dbContext.cityStmt);
		if (SqlCode != SQLITE_DONE)
		{
			//NodeElems = -100;
		}
		SqlCode = sqlite3_clear_bindings(dbContext.cityStmt);
		SqlCode = sqlite3_reset(dbContext.cityStmt);

		cityList++;
		if (cityList > 10000)
		{
			sqlite3_exec(dbContext.dbAddrCtx, "END TRANSACTION", NULL, NULL, &errMsg);
			sqlite3_exec(dbContext.dbAddrCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
			cityList = 0;
		}
	}
	if (cityList > 0)
	{
		sqlite3_exec(dbContext.dbAddrCtx, "END TRANSACTION", NULL, NULL, &errMsg);
	}
}