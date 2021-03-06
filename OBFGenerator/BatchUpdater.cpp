#include "stdafx.h"
#include "BatchUpdater.h"


BatchUpdater::~BatchUpdater(void)
{
}

void BatchUpdater::addBatch(Amenity itemToAdd) 
{
	amenities.push_back(itemToAdd); 
	if(amenities.size() > 10000) {
		flush();
	} 
}

void BatchUpdater::addBatch(__int64 id, __int64 firstId, __int64 lastId, std::string& name, std::stringstream& bNodes,std::stringstream& bTypes,std::stringstream& bAddtTypes,int level)
{
	lowLevelMapData mapData;
	mapData.id = id;
	mapData.firstid = firstId;
	mapData.lastid = lastId;
	mapData.name = name;
	mapData.nodes = bNodes.str();
	mapData.types = bTypes.str();
	mapData.addtypes = bAddtTypes.str();
	mapData.level = level;

	lowLevelMapList.push_back(mapData);

	if (lowLevelMapList.size() > 10000)
	{
		flush();
	}

}

void BatchUpdater::addBatch(__int64 id, bool area, std::stringstream& bCoord, std::stringstream& bInCoord ,std::stringstream& bTypes,std::stringstream& bAddtTypes,std::string& name)
{
	binMapData binMap;
	binMap.area = area;
	binMap.id = id;
	binMap.coord = bCoord.str();
	binMap.incoord = bInCoord.str();
	binMap.types = bTypes.str();
	binMap.addtypes = bAddtTypes.str();
	binMap.name = name;

	binaryMapList.push_back(binMap);
	if (binaryMapList.size() > 10000)
	{
		flush();
	}
}

void BatchUpdater::addBatchRoute(__int64 id, std::stringstream& types, std::stringstream& ptTypes ,std::stringstream& ptIds,std::stringstream& coords, std::string& name, bool base)
{
	routeData routeElem;
	routeElem.id = id;
	routeElem.types = types.str();
	routeElem.ptTypes = ptTypes.str();
	routeElem.ptIds = ptIds.str();
	routeElem.coords = coords.str();
	routeElem.name = name;
	if (base)
	{
		baserouteList.push_back(routeElem);
		if (baserouteList.size() > 10000)
		{
			flush();
		}

	}
	else
	{
	routeList.push_back(routeElem);
	if (routeList.size() > 10000)
	{
		flush();
	}
	}
}

void BatchUpdater::flush(bool bFlush)
{
	USES_CONVERSION;
	if (amenities.size() > 10000 || bFlush)
	{
		sqlite3_stmt* amenityStmt = workContext.poiNodeStmt;
		sqlite3* dbCtx = workContext.dbPoiCtx;
		char* errMsg;
		int SqlCode;
		sqlite3_exec(dbCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
		for(Amenity am : amenities)
		{
			// INSERT INTO poi (id, x, y, type, subtype, additionalTags) VALUES (1?, 2?, 3?, 4?, 5?, 6?)"
			SqlCode = sqlite3_bind_int64(amenityStmt, 1, am.getID());
			SqlCode = sqlite3_bind_int(amenityStmt, 2,  MapUtils::get31TileNumberX(am.getLatLon().second) );
			SqlCode = sqlite3_bind_int(amenityStmt, 3, MapUtils::get31TileNumberY(am.getLatLon().first) );
			SqlCode = sqlite3_bind_text(amenityStmt, 4, am.getType().c_str(), am.getType().size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_text(amenityStmt, 5, am.subType.c_str(), am.subType.size(), SQLITE_TRANSIENT);
			std::string additional = encodeAdditionalInfo(am.additionalInfo, am.getName(), am.getEnName());
			SqlCode = sqlite3_bind_text(amenityStmt, 6, additional.c_str(), additional.size() , SQLITE_TRANSIENT);
			SqlCode = sqlite3_step(amenityStmt);
			if (SqlCode != SQLITE_DONE)
			{
				if (SqlCode == SQLITE_CONSTRAINT)
				{
					if (errMsg != nullptr)
					{
						std::wstring errorCode = A2W(errMsg);
						OutputDebugString(errorCode.c_str());
					}
				}
			}
			//SqlCode = sqlite3_clear_bindings(amenityStmt);
			SqlCode = sqlite3_reset(amenityStmt);
		}
		sqlite3_exec(dbCtx, "END TRANSACTION", NULL, NULL, &errMsg);
		amenities.clear();
	}
	
	if (lowLevelMapList.size() > 10000 || bFlush)
	{
		//insert into low_level_map_objects(id, start_node, end_node, name, nodes, type, addType, level)
		sqlite3_stmt* lowMapStmt = workContext.lowStmt;
		sqlite3* dbCtx = workContext.dbMapCtx;
		char* errMsg;
		int SqlCode;
		sqlite3_exec(dbCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
		for (lowLevelMapData mapData : lowLevelMapList)
		{
			SqlCode = sqlite3_bind_int64(lowMapStmt, 1, mapData.id);
			SqlCode = sqlite3_bind_int64(lowMapStmt, 2, mapData.firstid);
			SqlCode = sqlite3_bind_int64(lowMapStmt, 3, mapData.lastid);
			SqlCode = sqlite3_bind_text(lowMapStmt, 4, mapData.name.c_str(), mapData.name.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_blob(lowMapStmt, 5, mapData.nodes.c_str(), mapData.nodes.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_blob(lowMapStmt, 6, mapData.types.c_str(), mapData.types.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_blob(lowMapStmt, 7, mapData.addtypes.c_str(), mapData.addtypes.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_int(lowMapStmt, 8, mapData.level);
			SqlCode = sqlite3_step(lowMapStmt);
			if (SqlCode != SQLITE_DONE)
			{
				if (SqlCode == SQLITE_CONSTRAINT)
				{
					if (errMsg != nullptr)
					{
						std::wstring errorCode = A2W(errMsg);
						OutputDebugString(errorCode.c_str());
					}
				}
			}
			//SqlCode = sqlite3_clear_bindings(lowMapStmt);
			SqlCode = sqlite3_reset(lowMapStmt);
		}
		sqlite3_exec(dbCtx, "END TRANSACTION", NULL, NULL, &errMsg);
		lowLevelMapList.clear();
	}

	if (binaryMapList.size() > 10000 || bFlush)
	{
		// "insert into binary_map_objects(id, area, coordinates, innerPolygons, types, additionalTypes, name) values(?1, ?2, ?3, ?4, ?5, ?6, ?7)"
		sqlite3_stmt* binMapStmt = workContext.mapStmt;
		sqlite3* dbCtx = workContext.dbMapCtx;
		char* errMsg;
		int SqlCode;
		sqlite3_exec(dbCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
		for (binMapData mapData : binaryMapList)
		{
			SqlCode = sqlite3_bind_int64(binMapStmt, 1, mapData.id);
			SqlCode = sqlite3_bind_int(binMapStmt, 2, mapData.area);
			SqlCode = sqlite3_bind_blob(binMapStmt, 3, mapData.coord.c_str(), mapData.coord.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_blob(binMapStmt, 4, mapData.incoord.c_str(), mapData.incoord.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_blob(binMapStmt, 5, mapData.types.c_str(), mapData.types.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_blob(binMapStmt, 6, mapData.addtypes.c_str(), mapData.addtypes.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_text(binMapStmt, 7, mapData.name.c_str(), mapData.name.size(), SQLITE_TRANSIENT);
			
			SqlCode = sqlite3_step(binMapStmt);
			if (SqlCode != SQLITE_DONE)
			{
				if (SqlCode == SQLITE_CONSTRAINT)
				{
					if (errMsg != nullptr)
					{
						std::wstring errorCode = A2W(errMsg);
						OutputDebugString(errorCode.c_str());
					}
				}
			}
			//SqlCode = sqlite3_clear_bindings(binMapStmt);
			SqlCode = sqlite3_reset(binMapStmt);
		}
		sqlite3_exec(dbCtx, "END TRANSACTION", NULL, NULL, &errMsg);
		binaryMapList.clear();
	}

	if (routeList.size() > 10000 || bFlush)
	{
		// "insert into route_objects(id, types, pointTypes, pointIds, pointCoordinates, name) values(?1, ?2, ?3, ?4, ?5, ?6)",
		sqlite3_stmt* routeStmt = workContext.routeStmt;
		sqlite3* dbCtx = workContext.dbMapCtx;
		char* errMsg;
		int SqlCode;
		sqlite3_exec(dbCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
		for (routeData routeData : routeList)
		{
			SqlCode = sqlite3_bind_int64(routeStmt, 1, routeData.id);
			SqlCode = sqlite3_bind_blob(routeStmt, 2, routeData.types.c_str(), routeData.types.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_blob(routeStmt, 3, routeData.ptTypes.c_str(), routeData.ptTypes.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_blob(routeStmt, 4, routeData.ptIds.c_str(), routeData.ptIds.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_blob(routeStmt, 5, routeData.coords.c_str(), routeData.coords.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_text(routeStmt, 6, routeData.name.c_str(), routeData.name.size(), SQLITE_TRANSIENT);
			
			SqlCode = sqlite3_step(routeStmt);
			if (SqlCode != SQLITE_DONE)
			{
				if (SqlCode == SQLITE_CONSTRAINT)
				{
					if (errMsg != nullptr)
					{
						std::wstring errorCode = A2W(errMsg);
						OutputDebugString(errorCode.c_str());
					}
				}
			}
			//SqlCode = sqlite3_clear_bindings(routeStmt);
			SqlCode = sqlite3_reset(routeStmt);
		}
		sqlite3_exec(dbCtx, "END TRANSACTION", NULL, NULL, &errMsg);
		routeList.clear();
	}
	if (baserouteList.size() > 10000 || bFlush)
	{
		// "insert into baseroute_objects(id, types, pointTypes, pointIds, pointCoordinates, name) values(?1, ?2, ?3, ?4, ?5, ?6)"
		sqlite3_stmt* routeStmt = workContext.baseRouteStmt;
		sqlite3* dbCtx = workContext.dbMapCtx;
		char* errMsg;
		int SqlCode;
		sqlite3_exec(dbCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
		for (routeData routeData : baserouteList)
		{
			SqlCode = sqlite3_bind_int64(routeStmt, 1, routeData.id);
			SqlCode = sqlite3_bind_blob(routeStmt, 2, routeData.types.c_str(), routeData.types.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_blob(routeStmt, 3, routeData.ptTypes.c_str(), routeData.ptTypes.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_blob(routeStmt, 4, routeData.ptIds.c_str(), routeData.ptIds.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_blob(routeStmt, 5, routeData.coords.c_str(), routeData.coords.size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_text(routeStmt, 6, routeData.name.c_str(), routeData.name.size(), SQLITE_TRANSIENT);
			
			SqlCode = sqlite3_step(routeStmt);
			if (SqlCode != SQLITE_DONE)
			{
				if (SqlCode == SQLITE_CONSTRAINT)
				{
					if (errMsg != nullptr)
					{
						std::wstring errorCode = A2W(errMsg);
						OutputDebugString(errorCode.c_str());
					}
				}
			}
			//SqlCode = sqlite3_clear_bindings(routeStmt);
			SqlCode = sqlite3_reset(routeStmt);
		}
		sqlite3_exec(dbCtx, "END TRANSACTION", NULL, NULL, &errMsg);
		baserouteList.clear();
	}
}

std::string BatchUpdater::encodeAdditionalInfo(std::unordered_map<std::string, std::string>& tempNames, std::string name, std::string nameEn) {
		if(!(name == "")) {
			tempNames.insert(std::make_pair("name", name));
		}
		if(!(nameEn == "") && !(name == nameEn)) {
			tempNames.insert(std::make_pair("name:en", nameEn));
		}
		std::string b = "";
		for (std::pair<std::string, std::string> e : tempNames) {
			MapRulType* rulType = renderer.getRuleType(e.first, e.second, true);
			if(rulType == nullptr) {
				throw new std::bad_exception();
			}
			if(!rulType->isText() ||  !(e.second == "")) {
				if(b.length() > 0){
					b.push_back(-1);
					b.push_back(-1);
				}
				if(rulType->isAdditional() && rulType->getValue() == "") {
					std::stringstream strStr;
					strStr << "Invalid rule (empty) adding to db";
					strStr << " tag ID:" << rulType->getTag() << std::endl;
					throw new std::bad_exception(strStr.str().c_str());
				}
				short ruleId = (short)rulType->getInternalId();
				b.push_back((char)(ruleId & 0xFF) );
				b.push_back((char)(ruleId >> 8) );
				b.append(e.second);
			}
		}
		return b;
	}


void BatchUpdater::commit()
{
	flush(true);
}