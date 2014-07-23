#include "stdafx.h"
#include "SkCanvas.h"
#include "SkSurface.h"
#include "SkImage.h"
#include "SkData.h"
#include "SkStream.h"
#include "SkGraphics.h"
#include "MultiPoly.h"
#include <google\protobuf\io\coded_stream.h>
#include <google\protobuf\io\zero_copy_stream_impl_lite.h>
#include <google\protobuf\io\zero_copy_stream_impl.h>
#include <google\protobuf\wire_format_lite.h>
#include <boost\container\slist.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "..\..\..\..\core\protos\OBF.pb.h"
#include "RandomAccessFileWriter.h"
#include "BinaryMapDataWriter.h"
#include "OBFElementDB.h"
#include "OBFRenderingTypes.h"
#include "Amenity.h"
#include "Building.h"
#include "Street.h"
#include "DBAStreet.h"
#include "iconv.h"
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
				propagatedTags.insert(std::make_pair(entryRel.first, std::unordered_map<std::string, std::string>()));
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
	std::unordered_map<std::string, std::string> tags;
	if (propagatedTags.find(baseItem->id) != propagatedTags.end())
	{
		tags = propagatedTags.at(baseItem->id);
	}
	if (!tags.empty()) {
		std::unordered_map<std::string, std::string>::iterator iterator = tags.begin();
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


void OBFpoiDB::writePoiDataIndex(BinaryMapDataWriter& writer, OBFResultDB& dbCtx, std::string poiTableName)
{
	POITree treeData;
	std::unordered_map<std::string, std::unordered_set<POIBox>> namesIndex;
	boxI bbox;
	int zoomStart = 6;
	processPOIIntoTree(dbCtx, treeData, zoomStart, bbox, namesIndex);

	__int64 fileP = writer.startWritePoiIndex("POIDATA", bbox.min_corner().get<0>(), bbox.max_corner().get<0>(), bbox.min_corner().get<1>(), bbox.max_corner().get<1>());

	writer.writePoiCategoriesTable(treeData.node.category);

	std::unordered_map<POIBox,  std::list<std::shared_ptr<BinaryFileReference>>> poiDataIdx = writer.writePoiNameIndex(namesIndex, fileP);

	int level = 0;
		for (; level < (16 - zoomStart); level++) {
			int subtrees = treeData.getSubTreesOnLevel(level);
			if (subtrees > 8) {
				level--;
				break;
			}
		}
		if (level > 0) {
			treeData.extractChildrenFromLevel(level);
			zoomStart = zoomStart + level;
		}
		for (std::shared_ptr<POITree> subs : treeData.subNodes) {
			writePoiBoxes(writer, subs, fileP, poiDataIdx, treeData.node.category);
		}

		for (auto entry : poiDataIdx) {
			int z = entry.first.zoom;
			int x = entry.first.x;
			int y = entry.first.y;
			std::vector<std::shared_ptr<BinaryFileReference>> vecDD;
			vecDD.reserve(entry.second.size());
			std::copy(entry.second.begin(), entry.second.end(), vecDD.begin());
			writer.startWritePoiData(z, x, y, vecDD);

			{
				std::list<POIData> poiData = entry.first.values;
				
				for(POIData poi : poiData){
					int x31 = poi.x;
					int y31 = poi.y;
					std::string type = poi.type;
					std::string subtype = poi.subType;
					int x24shift = (x31 >> 7) - (x << (24 - z));
					int y24shift = (y31 >> 7) - (y << (24 - z));
					writer.writePoiDataAtom(poi.id, x24shift, y24shift, type, subtype, poi.additionalTags, renderer, 
							treeData.node.category);	
				}
				
			} 
		}
}

void OBFpoiDB::writePoiBoxes(BinaryMapDataWriter& writer, std::shared_ptr<POITree> tree, 
			__int64 startFpPoiIndex, std::unordered_map<POIBox,  std::list<std::shared_ptr<BinaryFileReference>>>& fpToWriteSeeks,
			POICategory& globalCategories) {
				int x = tree->node.x;
		int y = tree->node.y;
		int zoom = tree->node.zoom;
		boolean end = zoom == 16;
		std::shared_ptr<BinaryFileReference> fileRef = writer.startWritePoiBox(zoom, x, y, startFpPoiIndex, end);
		if(fileRef){
			if(fpToWriteSeeks.find(tree->node) == fpToWriteSeeks.end()) {
				fpToWriteSeeks.insert(std::make_pair(tree->node, std::list<std::shared_ptr<BinaryFileReference>>()));
			}
			fpToWriteSeeks[tree->node].push_back(fileRef);
		}
		if(zoom >= 12 && zoom <= 16){
			POICategory& boxCats = tree->node.category;
			boxCats.buildCategoriesToWrite(globalCategories);
			writer.writePoiCategories(boxCats);
		}
		
		if (!end) {
			for (std::shared_ptr<POITree> subTree : tree->subNodes) {
				writePoiBoxes(writer, subTree, startFpPoiIndex, fpToWriteSeeks, globalCategories);
			}
		}
		writer.endWritePoiBox();
	}

void OBFpoiDB::processPOIIntoTree(OBFResultDB& dbCtx, POITree& treeData, int zoomLevel, boxI& bbox, std::unordered_map<std::string, std::unordered_set<POIBox>>& nameIndex)
{
	int sqlRes = 0;
	sqlite3_stmt* poiSelect = nullptr;

	MapRulType* nameRuleMap = renderer.nameRule;
	MapRulType* nameEnRuleMap = renderer.nameEnRule;

	sqlRes = sqlite3_prepare_v2(dbCtx.dbPoiCtx, "SELECT x,y,type,subtype,id,additionalTags from poi", sizeof("SELECT x,y,type,subtype,id,additionalTags from poi"), &poiSelect, NULL);

	if (poiSelect == nullptr)
	{
		return;
	}
	std::unordered_map<MapRulType, std::string> typeMap;
	sqlRes = sqlite3_step(poiSelect);
	while (sqlRes == SQLITE_ROW)
	{
		int x = sqlite3_column_int(poiSelect, 1);
		int y = sqlite3_column_int(poiSelect, 2);
		bbox.min_corner().set<0>(min(x, bbox.min_corner().get<0>()));
		bbox.min_corner().set<1>(min(y, bbox.min_corner().get<1>()));
		bbox.max_corner().set<0>(max(x, bbox.max_corner().get<0>()));
		bbox.max_corner().set<1>(max(y, bbox.max_corner().get<1>()));
		const unsigned char* typeChar = sqlite3_column_text(poiSelect, 3);
		const unsigned char* subTypeChar = sqlite3_column_text(poiSelect, 4);
		const unsigned char* addTypeChar = sqlite3_column_text(poiSelect, 6);
		decodeAdditionalType(addTypeChar,  typeMap);
		POITree* oldTree = &treeData;
		std::string sType = typeChar == nullptr ? "" : std::string((const char*)typeChar);
		std::string subType = subTypeChar == nullptr ? "" : std::string((const char*)subTypeChar);
		treeData.node.category.addCategory(sType, subType, typeMap);

		for (int i = zoomLevel; i <= 16; i++) {
				int xs = x >> (31 - i);
				int ys = y >> (31 - i);
				std::shared_ptr<POITree> subtree;
				for (std::shared_ptr<POITree> sub : oldTree->subNodes) {
					if (sub->node.x == xs && sub->node.y == ys && sub->node.zoom == i) {
						subtree = sub;
						break;
					}
				}
				if (!subtree) {
					subtree = std::make_shared<POITree>(POITree());
					POIBox poiBox;
					subtree->node = poiBox;
					poiBox.x = xs;
					poiBox.y = ys;
					poiBox.zoom = i;

					oldTree->subNodes.push_back(subtree);
				}
				subtree->node.category.addCategory(sType, subType, typeMap);

				oldTree = subtree.get();
				subtree.reset();
			}
		std::unordered_map<MapRulType, std::string>::iterator lookup;
		
		addNamePrefix(typeMap.find(*nameRuleMap), typeMap.find(*nameEnRuleMap), oldTree->node, nameIndex);

		POIData poiData;
		poiData.x = x;
		poiData.y = y;
		poiData.additionalTags = typeMap;
		poiData.subType = subType;
		poiData.type = sType;

		oldTree->node.values.push_back(poiData);

		sqlRes = sqlite3_step(poiSelect);
	}

}

void OBFpoiDB::addNamePrefix(std::unordered_map<MapRulType, std::string>::iterator& name, std::unordered_map<MapRulType, std::string>::iterator& nameEn, POIBox data, std::unordered_map<std::string, std::unordered_set<POIBox>>& poiData) 
{
	if (name._Ptr != nullptr) {
		parsePrefix(name->second, data, poiData);
		if (nameEn._Ptr != nullptr) {
				iconverter ic("UTF-8", "ASCII");
				nameEn->second = ic.convert(name->second);
			}
		if (!boost::iequals(nameEn->second, name->second)) {
				parsePrefix(nameEn->second, data, poiData);
			}
		}
}

void OBFpoiDB::parsePrefix(std::string name, POIBox data, std::unordered_map<std::string, std::unordered_set<POIBox>>& poiData) {
		int prev = -1;
		for (int i = 0; i <= name.size(); i++) {
			if (i == name.length() || (!std::isalpha(name[i])  && !std::isdigit(name[i]) && name[i] != '\'')) {
				if (prev != -1) {
					std::string substr = name.substr(prev, i);
					if (substr.size() > 4) {
						substr = substr.substr(0, 4);
					}
					std::string val = boost::to_lower_copy(substr);
					if(poiData.find(val) == poiData.end()){
						poiData.insert(std::make_pair(val, std::unordered_set<POIBox>()));
					}
					poiData[val].insert(data);
					prev = -1;
				}
			} else {
				if(prev == -1){
					prev = i;
				}
			}
		}
		
	}

void OBFpoiDB::decodeAdditionalType(const unsigned char* addTypeChar, std::unordered_map<MapRulType, std::string>&  typeMap)
{
	typeMap.clear();
	if (addTypeChar == nullptr)
	{
		return;
	}
	std::string addTypes((const char*)addTypeChar);
	int i = 0, p = 0;

	while (addTypes.size() > 0)
	{
		p = addTypes.find_first_of(-1, i);
		std::string rText = p == std::string::npos ? addTypes.substr(i) : addTypes.substr(i, p);
		MapRulType rType = renderer.getTypeByInternalId((int)rText[0]);
		typeMap.insert(std::make_pair(rType, rText.substr(1)));
		if (rType.isAdditional() && rType.getValue() == "")
		{
			throw std::bad_exception("Map rule type is wrong");
		}
		if (p == -1)
			break;
		i = p+1;
	}
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

	std::unordered_map<std::string, std::string> propagated = routingTypes.getRouteRelationPropogatedTags(*entry.get());
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
				propagatedTags.insert(std::make_pair(entryRel.first, std::unordered_map<std::string, std::string>()));
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
			std::unordered_map<std::string, std::string>::iterator itTag = tagsIt->second.begin();
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


void OBFrouteDB::addWayToIndex(long long id, std::vector<std::shared_ptr<EntityNode>>& nodes, OBFResultDB& dbContext, RTreeValued& rTree, bool base)  {
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
			rTree.insertBox(minX, minY, maxX, maxY, std::make_pair(id, std::vector<short>()));

		}
	}

  long OBFrouteDB::SHIFT_INSERT_AT = 12;
  long OBFrouteDB::SHIFT_ORIGINAL = 16;
  long OBFrouteDB::SHIFT_ID = 64 - (SHIFT_INSERT_AT + SHIFT_ORIGINAL);

 void OBFrouteDB::registerBaseIntersectionPoint(long long pointLoc, bool registerId,long long wayId, int insertAt, int originalInd) {
		
		std::unordered_map<__int64, __int64>::iterator exNode = basemapRemovedNodes.find(pointLoc);
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
			std::unordered_set<std::string> isInNames;
			dbContext.loadRelationMembers(i.get());
			dbContext.loadNodesOnRelation(i.get());
			
			std::vector<std::shared_ptr<EntityBase>> members = i->getMembers("street");
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
				LatLon = std::get<1>(i->relations.begin()->second)->getLatLon(); // get coordinates from any relation member
				isInNames = i->getIsInNames();
			}
			
			DBAStreet streetDAO(dbContext);

			if (streetName != "") {
				std::unordered_set<long long> idsOfStreet = getStreetInCity(isInNames, streetName, "", LatLon, dbContext);
				if (idsOfStreet.size() > 0) {
					std::vector<std::shared_ptr<EntityBase>> houses = i->getMembers("house"); // both house and address roles can have address
					std::vector<std::shared_ptr<EntityBase>> addresses = i->getMembers("address");
					houses.insert(houses.end(), addresses.begin(), addresses.end());
					for (std::shared_ptr<EntityBase> house : houses) {
						std::string hname = house->getTag("addr:housename");
						if(hname == "") {
							hname = house->getTag("addr:housenumber");
						}
						if (hname == "")
							continue;
						
						if (!streetDAO.findBuilding(house)) {
							std::shared_ptr<EntityRelation> houseRel = std::dynamic_pointer_cast<EntityRelation, EntityBase>(house);
							// process multipolygon (relation) houses - preload members to create building with correct latlon
							if (houseRel)
							{
								dbContext.loadRelationMembers(houseRel.get());
								dbContext.loadNodesOnRelation(houseRel.get());
							}
							Building building;
							MapObject::parseMapObject(&building, house.get());
							building.setBuilding(house.get());
							if (building.getLatLon().first == -1000) {
								//log.warn("building with empty location! id: " + house->id);
							}
							building.setName(hname);
							
							streetDAO.writeBuilding(idsOfStreet, building);
						}
					}
				}
			}
		}
	}


std::unordered_set<long long> OBFAddresStreetDB::getStreetInCity(std::unordered_set<std::string> isInNames, std::string name, std::string nameEn, std::pair<double,double> location, OBFResultDB& dbContext) {
		if (name == "" || location.first == -1000) {
			return std::unordered_set<long long>();
		
		}
		boost::trim(name);
		std::unordered_set<CityObj> result;
		std::list<CityObj> nearestObjects;
		std::vector<CityObj> objects = cityManager.getClosestObjects(location.first,location.second);
		nearestObjects.insert(nearestObjects.end(), objects.begin(), objects.end());
		objects = townManager.getClosestObjects(location.first,location.second);
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
			return dist1 > dist2;
		});

		/*Collections.sort(nearestObjects, new Comparator<CityObj>() {
			@Override
			public int compare(CityObj c1, CityObj c2) {
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

		if (result.size() == 0) {
			return std::unordered_set<long long>();
		}
		if ( boost::empty(nameEn)) {

			iconverter ic("UTF-8", "ASCII");
			nameEn = ic.convert(boost::trim_copy(name));
		}

		DBAStreet streetDAO(dbContext);
		std::unordered_set<long long> values;
		for (CityObj city : result) {
			std::string cityPart = findCityPart(location, city);
		
			std::unique_ptr<SimpleStreet> foundStreet = streetDAO.findStreet(name, city, cityPart);
			if (!foundStreet) {
				// by default write city with cityPart of the city
				if(cityPart == "") {
					cityPart = city.getName();
				}
				values.insert(streetDAO.insertStreet(name, nameEn, location, city, cityPart));
			} else {
				values.insert(foundStreet->getId());
			}
		}
		return values;
	}

std::string OBFAddresStreetDB::findCityPart(LatLon location, CityObj city) {
		std::string cityPart = city.getName();
		boolean found = false;
		std::shared_ptr<MultiPoly> cityBoundary = cityBoundaries[city];
		if (cityBoundary) {
			std::list<CityObj> subcities = boundaryToContainingCities[cityBoundary];
			if (subcities.size() > 0) {
				for (CityObj subpart : subcities) {
					if (!(subpart == city)) {
						std::shared_ptr<MultiPoly> subBoundary = cityBoundaries[subpart];
						if (cityBoundary && subBoundary && subBoundary->level > cityBoundary->level) {
							// old code
							cityPart = findNearestCityOrSuburb(subBoundary, location); // subpart.getName();
							// ?FIXME
							if(subBoundary->containsPoint(location)) {
								cityPart = subpart.getName();
								found = true;
								break;	
							}
						}
					}
				}
			}
		}
		if (!found) {
			std::shared_ptr<MultiPoly> b = cityBoundaries[city];
			cityPart = findNearestCityOrSuburb(b, location);
		}
		return cityPart;
	}

 std::string OBFAddresStreetDB::findNearestCityOrSuburb(std::shared_ptr<MultiPoly> greatestBoundary, LatLon location) 
 {
		std::string result = "";
		double dist = HUGE_VAL;
		std::list<CityObj> list;
		if(greatestBoundary) {
			result = greatestBoundary->polyName;
			list = boundaryToContainingCities[greatestBoundary];
		} else {
			std::vector<CityObj> cities = cityManager.getClosestObjects(location.first,location.second);
			list.insert(list.end(), cities.begin(), cities.end());
			cities = townManager.getClosestObjects(location.first,location.second);
			list.insert(list.end(), cities.begin(), cities.end());
		}
		if(list.size()) {
			for (CityObj c : list) {
				double actualDistance = MapUtils::getDistance(location.first, location.second, c.getLatLon().first, c.getLatLon().second);
				if (actualDistance < 1.5 * c.getRadius() && actualDistance < dist) {
					result = c.getName();
					dist = actualDistance;
				}
			}
		}
		return result;
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
			

			std::list<CityObj> list(0);
			for (auto c : cities) {
				if (boundary->containsPoint(c.second.getLatLon())) {
					list.push_back(c.second);
				}
			}
			if(list.size() > 0) {
				boundaryToContainingCities[boundary] = list;
			}
			
			if (foundCity)
			{
				foundCity.release();
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
		if (relItem)
		{
			dbContext.loadRelationMembers(relItem.get());
			if (relItem->entityIDs.size() > 0)
			{
				dbContext.loadNodesOnRelation(relItem.get());
			}
		}
		__int64 centrID = -1;
		std::shared_ptr<MultiPoly> polyline(new MultiPoly);
		if (relItem)
		{
			for(auto entityItem : relItem->relations)
			{
				if (std::get<0>(entityItem.second) == 2)
				{
					std::shared_ptr<EntityRelation> relSubItem = std::dynamic_pointer_cast<EntityRelation, EntityBase>(std::get<1>(entityItem.second));
					if (relSubItem->entityIDs.size() > 0)
					{
						dbContext.loadNodesOnRelation(relSubItem.get());
						for(auto innerEntityItem : relSubItem->relations)
						{
							if ( std::get<0>(innerEntityItem.second) == 1)
							{
								bool inner = std::get<2>(innerEntityItem.second) == "inner";
								std::shared_ptr<EntityWay> wayPtr = std::dynamic_pointer_cast<EntityWay>(std::get<1>(innerEntityItem.second));
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
							else if (std::get<0>(innerEntityItem.second) == 0)
							{
								if (std::get<2>(innerEntityItem.second) == "admin_centre" || std::get<2>(innerEntityItem.second) == "admin_center")
								{
									centrID = std::get<1>(innerEntityItem.second)->id;
								}
								else if (std::get<2>(innerEntityItem.second) == "label")
								{
									centrID = std::get<1>(innerEntityItem.second)->id;
								}
							}
						}
					}
				}
				else if (std::get<0>(entityItem.second) == 1)
				{
					polyline->outWays.push_back(std::dynamic_pointer_cast<EntityWay, EntityBase>(std::get<1>(entityItem.second)));
				}
			}
		}
		else if (wayItem)
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
			//boundaries.insert(polyline);
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
	else // if (placeType ==  "VILLAGE" ||placeType ==  "HAMLET" ||placeType ==  "SUBURB" ||placeType ==  "DISTRICT")
	{
		SaverCityNode(ptrNode.get(), townManager);
		objCity.setType(placeType);
	}

				
	objCity.setId(cityNode->id);
	MapObject::parseMapObject(&objCity, cityNode.get());
	//objCity.setLocation(ptrNode->lat, ptrNode->lon);
	std::string isin = cityNode->getTag("is_in");
	boost::to_lower(isin);
	objCity.setIsin(isin);

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
		//SqlCode = sqlite3_clear_bindings(dbContext.cityStmt);
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
		//SqlCode = sqlite3_clear_bindings(dbContext.cityStmt);
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

void OBFAddresStreetDB::iterateMainEntity(std::shared_ptr<EntityBase>& baseItem, OBFResultDB& dbContext)
{
	// indexing rest of addressable elemetns (building etc)
	DBAStreet streetDAO(dbContext);
	std::string interpolation = baseItem->getTag(OSMTags::ADDR_INTERPOLATION);
	std::shared_ptr<EntityWay> wayItem = std::dynamic_pointer_cast<EntityWay, EntityBase>(baseItem);
	std::shared_ptr<EntityRelation> relItem = std::dynamic_pointer_cast<EntityRelation, EntityBase>(baseItem);
		if (wayItem && interpolation != "" ){
			Building::BuildingInterpolation type = Building::BuildingInterpolation::NONE;
			int interpolationInterval = 0;
			if(interpolation != "") {
					std::string interpolType = boost::to_upper_copy(interpolation);
					if (interpolType == "ALL")
					{
						type = Building::BuildingInterpolation::ALL;
					}
					else if (interpolType == "EVEN")
					{
						type = Building::BuildingInterpolation::EVEN;
					}
					else if (interpolType == "ODD")
					{
						type = Building::BuildingInterpolation::ODD;
					}
					else if (interpolType == "ALPHA")
					{
						type = Building::BuildingInterpolation::ALPHA;
					}
			}
			if (type != Building::BuildingInterpolation::NONE || interpolationInterval > 0) {
				std::vector<std::shared_ptr<EntityNode>> nodesWithHno;
				for (std::shared_ptr<EntityNode> n : wayItem->nodes) {
					if (n->getTag(OSMTags::ADDR_HOUSE_NUMBER) != "" && n->getTag(OSMTags::ADDR_STREET) != "") {
						nodesWithHno.push_back(n);
					}
				}
				if (nodesWithHno.size() > 1) {
					for (int i = 1; i < nodesWithHno.size(); i++) {
						std::shared_ptr<EntityNode> first = nodesWithHno.at(i - 1);
						std::shared_ptr<EntityNode> second = nodesWithHno.at(i);
						boolean exist = streetDAO.findBuilding(first);
						if (exist) {
							streetDAO.removeBuilding(first);
						}
						LatLon l = baseItem->getLatLon();
						std::unordered_set<__int64> idsOfStreet = getStreetInCity(first->getIsInNames(), first->getTag(OSMTags::ADDR_STREET), "", l, dbContext);
						if (!(idsOfStreet.size() == 0)) {
							Building building;
							MapObject::parseMapObject(&building, first.get());
							building.setBuilding(first.get());
							building.interval = interpolationInterval;
							building.interpType = type;
							building.setName(first->getTag(OSMTags::ADDR_HOUSE_NUMBER));
							building.name2 = second->getTag(OSMTags::ADDR_HOUSE_NUMBER);
							building.location2 = second->getLatLon();
							streetDAO.writeBuilding(idsOfStreet, building);
						}
					}
				}
			}
		} 
		std::string houseName = baseItem->getTag(OSMTags::ADDR_HOUSE_NAME);
		std::string houseNumber = baseItem->getTag(OSMTags::ADDR_HOUSE_NUMBER);
		std::string street = baseItem->getTag(OSMTags::ADDR_STREET);
		std::string street2 = baseItem->getTag(OSMTags::ADDR_STREET2);
		if ((houseName != "" || houseNumber != "") && street != "") {
			if(relItem) {
				dbContext.loadRelationMembers(relItem.get());
				dbContext.loadNodesOnRelation(relItem.get());
				std::vector<std::shared_ptr<EntityBase>> outs = relItem->getMembers("outer");
				if(outs.size()) {
					baseItem.swap(*outs.begin());
				}
			}
			// skip relations
			boolean exist = relItem ||  streetDAO.findBuilding(baseItem);
			if (!exist) {
				LatLon l = baseItem->getLatLon();
				std::unordered_set<__int64> idsOfStreet = getStreetInCity(baseItem->getIsInNames(), street, "", l, dbContext);
				if (idsOfStreet.size()) {
					Building building;
					MapObject::parseMapObject(&building, baseItem.get());
					building.setBuilding(baseItem.get());
					std::string hname = houseName;
					if(hname == "") {
						hname = houseNumber;
					}
					int i = hname.find('-');
					if (i != hname.npos && interpolation != "") {
						building.interval = 1;
						std::string interpolType = boost::to_upper_copy(interpolation);
						Building::BuildingInterpolation type = Building::BuildingInterpolation::NONE;
						if (interpolType == "ALL")
						{
							type = Building::BuildingInterpolation::ALL;
						}
						else if (interpolType == "EVEN")
						{
							type = Building::BuildingInterpolation::EVEN;
						}
						else if (interpolType == "ODD")
						{
							type = Building::BuildingInterpolation::ODD;
						}
						else if (interpolType == "ALPHA")
						{
							type = Building::BuildingInterpolation::ALPHA;
						}
						
						building.setName(hname.substr(0, i));
						building.name2 = hname.substr(i + 1);
					} else {
						int secondNumber = hname.find('/');
						if(secondNumber == hname.npos || !(secondNumber < hname.length() - 1)) {
							building.setName(hname);
						} else {
							building.setName(hname.substr(0, secondNumber));
							Building building2;
							MapObject::parseMapObject(&building2, baseItem.get());
							building2.setBuilding(baseItem.get());
							building2.setName(hname.substr(secondNumber + 1));
							std::unordered_set<__int64> ids2OfStreet = getStreetInCity(baseItem->getIsInNames(), street2, "", l, dbContext);
							if (ids2OfStreet.size() > 0)
							{
								for (__int64 idsParent : idsOfStreet)
								{
									if (ids2OfStreet.find(idsParent) != ids2OfStreet.end())
									{
										ids2OfStreet.erase(idsParent);
									}
								}
							}
							
							if(ids2OfStreet.size()) {
								streetDAO.writeBuilding(ids2OfStreet, building2);
							} else {
								building.name2 = building2.getName();
							}
						}
					}
					
					streetDAO.writeBuilding(idsOfStreet, building);
				}
			}
		} else if (wayItem /* && OSMSettings.wayForCar(baseItem->getTag(OSMTags::HIGHWAY)) */
				&& baseItem->getTag(OSMTags::HIGHWAY) != "" && baseItem->getTag(OSMTags::NAME) != "") {
			// suppose that streets with names are ways for car
			// Ignore all ways that have house numbers and highway type
			
			// if we saved address ways we could checked that we registered before
			boolean exist = streetDAO.findStreetNode(baseItem);



			// check that street way is not registered already
			if (!exist) {
				LatLon l = baseItem->getLatLon();
				std::unordered_set<__int64> idsOfStreet = getStreetInCity(baseItem->getIsInNames(), baseItem->getTag(OSMTags::NAME), baseItem->getTag(OSMTags::NAME_EN), l, dbContext);
				if (idsOfStreet.size()) {
					streetDAO.writeStreetWayNodes(idsOfStreet, wayItem);
				}
			}
		}
		if (relItem) {
			if (baseItem->getTag(OSMTags::POSTAL_CODE) != "") {
				dbContext.loadRelationMembers(relItem.get());
				dbContext.loadNodesOnRelation(relItem.get());
				postalCodeRelations.push_back(relItem);
			}
		}

}

void OBFAddresStreetDB::writeAddresMapIndex(BinaryMapDataWriter& writer, std::string regionName, OBFResultDB& dbContext)
{
	writer.startWriteAddressIndex(regionName);
		std::unordered_map<std::string, std::list<CityObj>> cities = readCities(dbContext);

		int sqlStatus = 0;
		sqlite3_stmt* streetstat = nullptr;
		sqlStatus = sqlite3_prepare_v2(dbContext.dbAddrCtx, "SELECT A.id, A.name, A.name_en, A.latitude, A.longitude, B.id, B.name, B.name_en, B.latitude, B.longitude, B.postcode, A.cityPart,  B.name2, B.name_en2, B.lat2, B.lon2, B.interval, B.interpolateType, A.cityPart == C.name as MainTown FROM street A left JOIN building B ON B.street = A.id JOIN city C ON A.city = C.id WHERE A.city = ?1 ORDER BY MainTown DESC, A.name ASC", -1, &streetstat,NULL);
		//sqlStatus = sqlite3_step(streetstat);
		sqlite3_stmt* wayStreetstat = nullptr;
		sqlStatus = sqlite3_prepare_v2(dbContext.dbAddrCtx, "SELECT A.id, A.latitude, A.longitude FROM street_node A WHERE A.street = ?1", -1, &wayStreetstat,NULL);

		//// collect suburbs with is in value
		std::list<CityObj> suburbs;
		std::list<CityObj> cityTowns;
		std::list<CityObj> villages;

		for(auto tPP : cities) {
			if(tPP.first == "city" || tPP.first == "town"){
				cityTowns.insert(cityTowns.end(), tPP.second.begin(), tPP.second.end());
			} else {
				villages.insert(villages.end(), tPP.second.begin(), tPP.second.end());
			}
			if(tPP.first == "suburb"){
				for(CityObj c : tPP.second){
					if(c.getIsInValue() != "") {
						suburbs.push_back(c);
					}
				}
			}
		}

		//
		//progress.startTask(Messages.getString("IndexCreator.SERIALIZING_ADRESS"), cityTowns.size() + villages.size() / 100 + 1); //$NON-NLS-1$
		//
		std::map<std::string, std::list<std::shared_ptr<MapObject>>> namesIndex;
		std::map<std::string, CityObj> postcodes;
		writeCityBlockIndex(writer, "cities",  streetstat, wayStreetstat, suburbs, cityTowns, postcodes, namesIndex);
		writeCityBlockIndex(writer, "villages",  streetstat, wayStreetstat, std::list<CityObj>(), villages, postcodes, namesIndex);
		//
		//// write postcodes		
		std::vector<std::shared_ptr<BinaryFileReference>> refs;
		writer.startCityBlockIndex(2);
		std::vector<CityObj> posts;
		std::for_each(postcodes.begin(), postcodes.end(), [&posts] (std::pair<std::string, CityObj> itMap)
		{
			posts.push_back(itMap.second);
		});
		for (CityObj s : posts) {
			refs.push_back(std::shared_ptr<BinaryFileReference>(writer.writeCityHeader(s, -1)));
		}
		for (int i = 0; i < posts.size(); i++) {
			CityObj postCode = posts[i];
			std::shared_ptr<BinaryFileReference> ref = refs[i];
			putNamedMapObject(namesIndex, std::make_shared<MapObject>(postCode), ref->getStartPointer());
			std::list<Street> streePost;
			std::for_each(postCode.streets.begin(), postCode.streets.end(), [&streePost] (std::pair<std::string, Street> itMap)
			{
				streePost.push_back(itMap.second);
			});
			writer.writeCityIndex(postCode, streePost, std::unordered_map<Street, std::list<EntityNode>>(), ref.get());
		}
		writer.endCityBlockIndex();


		//progress.finishTask();

		writer.writeAddressNameIndex(namesIndex);
		//writer.endWriteAddressIndex();
		//writer.flush();
		//streetstat.close();
		//if (waynodesStat != null) {
		//	waynodesStat.close();
		//}
	writer.endWriteAddressIndex();
}


std::unordered_map<std::string, std::list<CityObj>> OBFAddresStreetDB::readCities(OBFResultDB& dbContext){
		std::unordered_map<std::string, std::list<CityObj>> cities;
		{
			cities.insert(std::make_pair("city", std::list<CityObj>()));
			cities.insert(std::make_pair("town", std::list<CityObj>()));
			cities.insert(std::make_pair("village", std::list<CityObj>()));
			cities.insert(std::make_pair("hamlet", std::list<CityObj>()));
			cities.insert(std::make_pair("suburb", std::list<CityObj>()));
			cities.insert(std::make_pair("district", std::list<CityObj>()));
		}
		int sqlStatus = 0;
		sqlite3_stmt* readCity = nullptr;
		sqlStatus = sqlite3_prepare_v2(dbContext.dbAddrCtx, "select id, latitude, longitude , name , name_en , city_type from city", -1, &readCity,NULL);
		sqlStatus = sqlite3_step(readCity);
		while(sqlStatus == SQLITE_ROW){
			std::string cityType((const char*)sqlite3_column_text(readCity, 5));
			boost::to_lower(cityType);
			CityObj city;
			city.setType(cityType);
			city.setName((const char*)sqlite3_column_text(readCity,3));
			city.setEnName((const char*)sqlite3_column_text(readCity,4));
			city.setLocation(sqlite3_column_double(readCity, 1),sqlite3_column_double(readCity, 2));
			city.setId(sqlite3_column_int64(readCity, 0));
			cities[cityType].push_back(city);
			
			if (cityBoundaries.find(city) != cityBoundaries.end())
			{
				auto cityB = cityBoundaries[city];
				if (cityB) {
					city.setName(city.getName() + " " + boost::lexical_cast<std::string>(cityB->level) + ":" + cityB->polyName);
				}
			}
			sqlStatus = sqlite3_step(readCity);
		}
		sqlite3_finalize(readCity);
		
		for(auto tPair : cities) {
			tPair.second.sort([](const CityObj& op1,const CityObj& op2) 
			{
				CityObj& work1 = const_cast<CityObj&>(op1);
				CityObj& work2 = const_cast<CityObj&>(op2);
				return boost::lexicographical_compare(work1.getName(), work2.getName());
			});
		}
		return cities;
	}

void OBFAddresStreetDB::putNamedMapObject(std::map<std::string, std::list<std::shared_ptr<MapObject>>>& namesIndex, std::shared_ptr<MapObject> o, __int64 fileOffset)
{
	std::string name = o->getName();
	
	int prev = -1;
	for (int i = 0; i <= name.length(); i++) 
	{
		if (i == name.length() || !isalpha(name[i]) && !isdigit(name[i]) && name[i] != '\'') {
			if (prev != -1) {
				std::string substr = name.substr(prev, i);
				if (substr.length() > 4) {
					substr = substr.substr(0, 4);
				}
				std::string val = boost::to_lower_copy(substr);
				if(namesIndex.find(val) == namesIndex.end()){
					namesIndex.insert(std::make_pair(val, std::list<std::shared_ptr<MapObject>>()));
				}
				namesIndex[val].push_back(o);
				prev = -1;
			}
		} else {
			if(prev == -1){
				prev = i;
			}
		}
	}

	if (fileOffset > INT_MAX) {
		throw std::bad_exception("File offset > 2 GB.");
	}
	o->setFileOffset((int) fileOffset);
}

std::vector<EntityNode> OBFAddresStreetDB::loadStreetNodes(__int64 streetId, sqlite3_stmt* waynodesStat)
{
	std::vector<EntityNode> resVec;
	sqlite3_bind_int64( waynodesStat, 1, streetId);
	int sqlCode = SQLITE_OK;
	sqlCode = sqlite3_step(waynodesStat);
	while (sqlCode == SQLITE_ROW) {
		EntityNode streetNode(sqlite3_column_double(waynodesStat, 1), sqlite3_column_double(waynodesStat, 2), sqlite3_column_int64(waynodesStat, 0));
		resVec.push_back(streetNode);
		sqlCode = sqlite3_step(waynodesStat);
	}
	return resVec;
}

void OBFAddresStreetDB::readStreetsAndBuildingsForCity(sqlite3_stmt* streetBuildingsStat, CityObj city,
			sqlite3_stmt* waynodesStat, std::unordered_map<Street, std::list<EntityNode>>& streetNodes, std::unordered_map<__int64, Street>& visitedStreets,
			std::unordered_map<std::string, std::vector<Street>>& uniqueNames)  {
			sqlite3_bind_int64( streetBuildingsStat, 1, city.getID());
			int sqlCode = SQLITE_OK;
			sqlCode = sqlite3_step(streetBuildingsStat);
		while (sqlCode == SQLITE_ROW) {
			long streetId = sqlite3_column_int64(streetBuildingsStat, 0);
			if (visitedStreets.find(streetId) == visitedStreets.end()) {
				std::string streetName = (const char*)sqlite3_column_text(streetBuildingsStat, 1);
				std::string streetEnName = (const char*)sqlite3_column_text(streetBuildingsStat, 2);
				double lat = sqlite3_column_double(streetBuildingsStat, 3);
				double lon = sqlite3_column_double(streetBuildingsStat, 4);
				// load the street nodes
				std::vector<EntityNode> thisWayNodes = loadStreetNodes(streetId, waynodesStat);
				if (uniqueNames.find(streetName) == uniqueNames.end()) {
					uniqueNames.insert(std::make_pair(streetName, std::vector<Street>()));
				}
				Street street(city);
				uniqueNames[streetName].push_back(street);
				street.setLocation(lat, lon);
				street.setId(streetId);
				// If there are more streets with same name in different districts.
				// Add district name to all other names. If sorting is right, the first street was the one in the city
				const char* district = (const char*)sqlite3_column_text(streetBuildingsStat, 11);
				std::string cityPart = district == nullptr || (city.getName() == district) ? "" : " (" + std::string(district) + ")";
				street.setName(streetName + cityPart);
				street.setEnName(streetEnName + cityPart);
				streetNodes.insert(std::make_pair(street, std::list<EntityNode>(thisWayNodes.begin(), thisWayNodes.end())));

				visitedStreets.insert(std::make_pair(streetId, street)); // mark the street as visited
			}
			if (sqlite3_column_text(streetBuildingsStat, 5) != nullptr) {
				Street s = visitedStreets[streetId];
				Building b;
				b.setId(sqlite3_column_int64(streetBuildingsStat, 5));
				const char* bldName = (const char*)sqlite3_column_text(streetBuildingsStat, 6);
				if (bldName != nullptr)
				{
					b.setName(bldName);
				}
				const char* bldEnName = (const char*)sqlite3_column_text(streetBuildingsStat, 7);
				if (bldEnName != nullptr)
				{
					b.setEnName(bldEnName);
				}
				b.setLocation(sqlite3_column_double(streetBuildingsStat, 8), sqlite3_column_double(streetBuildingsStat, 9));
				const char* postCode = (const char*)sqlite3_column_text(streetBuildingsStat, 10);
				if (postCode != nullptr)
				{
					b.postCode = std::string(postCode);
				}
				const char* name2 = (const char*)sqlite3_column_text(streetBuildingsStat, 12);
				if (name2 != nullptr)
				{
					b.name2 = std::string(name2);
				}
				// no en name2 for now
				name2 = (const char*)sqlite3_column_text(streetBuildingsStat, 13);
				if (name2 != nullptr)
				{
					b.name2 = std::string(name2);
				}
				double lat2 = sqlite3_column_double(streetBuildingsStat,14);
				double lon2 = sqlite3_column_double(streetBuildingsStat,15);
				if (lat2 != 0 || lon2 != 0) {
					b.location2 = LatLon(lat2, lon2);
				}
				b.interval = sqlite3_column_int(streetBuildingsStat, 16);
				const char* itype = (const char*)sqlite3_column_text(streetBuildingsStat, 17);
				if (itype != nullptr)
				{
					std::string type(itype);
					if (type != "") {
						b.setInterpType(type);
					}
				}
				s.addBuildingCheckById(b);
			}
			sqlCode = sqlite3_step(streetBuildingsStat);
		}

		
	}

double OBFAddresStreetDB::getDistance(Street s, Street c, std::unordered_map<Street, std::list<EntityNode>>& streetNodes) {
		std::list<EntityNode> thisWayNodes = streetNodes[s];
		std::list<EntityNode> oppositeStreetNodes = streetNodes[c];
		if(thisWayNodes.size() == 0) {
			thisWayNodes.push_back(EntityNode(s.getLatLon().first, s.getLatLon().second, -1));
		}
		if(oppositeStreetNodes.size() == 0) {
			oppositeStreetNodes.push_back(EntityNode(c.getLatLon().first, c.getLatLon().second, -1));
		}
		double md = DBL_MAX;
		for(EntityNode n : thisWayNodes) {
			for(EntityNode d : oppositeStreetNodes) {
				if(n.getLatLon().first != -1000 && d.getLatLon().first != -1000) {
					md = min(md, OsmMapUtils::getDistance(n, d));
				}
			}
		}
		return md;
	}

std::list<Street> OBFAddresStreetDB::readStreetsBuildings(sqlite3_stmt* streetBuildingsStat, CityObj city, sqlite3_stmt*  waynodesStat,
			std::unordered_map<Street, std::list<EntityNode>>&  streetNodes, std::vector<CityObj> citySuburbs) {
		std::unordered_map<__int64, Street> visitedStreets;
		std::unordered_map<std::string, std::vector<Street>> uniqueNames;

		// read streets for city
		readStreetsAndBuildingsForCity(streetBuildingsStat, city, waynodesStat, streetNodes, visitedStreets, uniqueNames);
		// read streets for suburbs of the city
		if (!citySuburbs.empty()) {
			for (CityObj suburb : citySuburbs) {
				readStreetsAndBuildingsForCity(streetBuildingsStat, suburb, waynodesStat, streetNodes, visitedStreets, uniqueNames);
			}
		}

		for(auto streetNameVal : uniqueNames) {
			std::vector<Street> streets = streetNameVal.second;
			if(streets.size() > 1) {
				for(int i = 0; i < streets.size() - 1 ; ) {
					Street s = streets[i];
					boolean merged = false;
					for (int j = i + 1; j < streets.size(); ) {
						Street candidate = streets[j];
						if(getDistance(s, candidate, streetNodes) <= 900) { 
							merged = true;
							//logMapDataWarn.info("City : " + s.getCity() + 
							//	" combine 2 district streets '" + s.getName() + "' with '" + candidate.getName() + "'");
							s.mergeWith(candidate);
							if(candidate.getName() == s.getName()) {
								candidate.getCity().unregisterStreet(candidate.getName());
							}
							std::unordered_map<Street, std::list<EntityNode>>::iterator itOld = streetNodes.find(candidate);
							std::list<EntityNode> old = itOld->second;
							streetNodes[s].insert(streetNodes[s].end(), old.begin(), old.end());
							streets.erase(streets.begin()+j);
							//streets.remove(j);
						} else {
							j++;
						}
					}
					if(!merged) {
						i++;
					}
				}
			
			}
		}

		std::list<Street> stList;
		std::for_each(streetNodes.begin(), streetNodes.end(), [&stList] (std::pair<Street, std::list<EntityNode>> itElems)
		{
			stList.push_back(itElems.first);
		});
		return stList;
		//return new ArrayList<Street>(streetNodes.keySet());
	}
	



	


void OBFAddresStreetDB::writeCityBlockIndex(BinaryMapDataWriter& writer, std::string citytype, sqlite3_stmt* streetstat, sqlite3_stmt* waynodesStat,
			std::list<CityObj>& suburbs, std::list<CityObj>& cities, std::map<std::string, CityObj>& postcodes, std::map<std::string, std::list<std::shared_ptr<MapObject>>>& namesIndex)			
			 {
		std::vector<BinaryFileReference*> refs;
		// 1. write cities
		if (citytype == "cities")
		{
			writer.startCityBlockIndex(1);
		}
		else if (citytype == "villages")
		{
			writer.startCityBlockIndex(3);
		}
		for (CityObj c : cities) {
			refs.push_back(writer.writeCityHeader(c, c.getRadius()));
		}
		for (int i = 0; i < cities.size(); i++) {
			std::list<CityObj>::iterator cit = cities.begin();
			std::advance(cities.begin(), i);
			CityObj city = *cit;
			BinaryFileReference* ref = refs[i];
			putNamedMapObject(namesIndex, std::make_shared<MapObject>(city), ref->getStartPointer());
			std::unordered_map<Street, std::list<EntityNode>> streetNodes;
			std::vector<CityObj> listSuburbs;
			if (!suburbs.empty()) {
				for (CityObj suburb : suburbs) {
					if ( boost::contains( suburb.getIsInValue(), boost::to_lower_copy(city.getName()))) {
						listSuburbs.push_back(suburb);
					}
				}
			}
			auto duration = boost::chrono::system_clock::now().time_since_epoch();
			__int64 millis = boost::chrono::duration_cast<boost::chrono::milliseconds>(duration).count();

			std::list<Street> streets = readStreetsBuildings(streetstat, city, waynodesStat, streetNodes, listSuburbs);
			auto duration2 =  boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::system_clock::now().time_since_epoch()).count();
			__int64 millisf = duration2 - millis;
			writer.writeCityIndex(city, streets, streetNodes, ref);
			int bCount = 0;
			// register postcodes and name index
			for (Street s : streets) {
				putNamedMapObject(namesIndex, std::make_shared<MapObject>(s), s.getFileOffset());
				
				for (Building b : s.getBuildings()) {
					bCount++;
					if (city.isPostcode() && b.postCode == "") {
						b.postCode == city.getPostcode();
					}
					if (b.postCode != "") {
						if (postcodes.find(b.postCode) != postcodes.end()) {
							CityObj p = CityObj::createPostcode(b.postCode);
							p.setLocation(b.getLatLon().first, b.getLatLon().second);
							postcodes.insert(std::make_pair(b.postCode, p));
						}
						CityObj post = postcodes[b.postCode];
						Street newS = post.streets[s.getName()];
						if(newS.init == false) {
							newS = Street(post);
							newS.setName(s.getName());
							newS.setEnName(s.getEnName());
							newS.setLocation(s.getLatLon().first, s.getLatLon().second);
							//newS.getWayNodes().addAll(s.getWayNodes());
							newS.setId(s.getID());
							post.registerStreet(newS);
						}
						newS.addBuildingCheckById(b);
					}
				}
			}
			if (millisf > 500) {
				std::wstringstream wstrm;
				std::wstring wname;
				wname.assign(city.getName().begin(), city.getName().end());
				wstrm << L"!" << wname <<  L" ! " << millisf << " ms " << streets.size() << L" streets " << bCount << L" Buildings" << std::endl;
				OutputDebugString(wstrm.str().c_str());
			}
		}
		writer.endCityBlockIndex();
	}
