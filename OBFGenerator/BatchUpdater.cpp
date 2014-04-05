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
void BatchUpdater::flush()
{
	USES_CONVERSION;
	if (amenities.size() > 0)
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
			SqlCode = sqlite3_bind_int(amenityStmt, 3, MapUtils::get31TileNumberX(am.getLatLon().first) );
			SqlCode = sqlite3_bind_text(amenityStmt, 4, am.getType().c_str(), am.getType().size(), SQLITE_TRANSIENT);
			SqlCode = sqlite3_bind_text(amenityStmt, 5, am.subType.c_str(), am.subType.size(), SQLITE_TRANSIENT);
			std::string additional = encodeAdditionalInfo(am.additionalInfo, am.getName(), am.getEnName());
			SqlCode = sqlite3_bind_text(amenityStmt, 5, additional.c_str(), additional.size() , SQLITE_TRANSIENT);
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
			SqlCode = sqlite3_clear_bindings(amenityStmt);
			SqlCode = sqlite3_reset(amenityStmt);
		}
		sqlite3_exec(dbCtx, "END TRANSACTION", NULL, NULL, &errMsg);
	}
	amenities.clear();
}

std::string BatchUpdater::encodeAdditionalInfo(std::map<std::string, std::string> tempNames, std::string name, std::string nameEn) {
		if(!(name == "")) {
			tempNames.insert(std::make_pair("name", name));
		}
		if(!(nameEn == "") && !(name == nameEn)) {
			tempNames.insert(std::make_pair("name:en", nameEn));
		}
		std::string b = "";
		for (std::pair<std::string, std::string> e : tempNames) {
			MapRulType rulType = renderer.getRuleType(e.first, e.second, true);
			if(rulType.isEmpty()) {
				
			}
			if(!rulType.isText() ||  !(e.second == "")) {
				if(b.length() > 0){
					b.push_back(-1);
				}
				if(rulType.isAdditional() && rulType.getValue() == "") {
					
				}
				b.push_back((char)(rulType.getInternalId()) );
				b.append(e.second);
			}
		}
		return b;
	}


void BatchUpdater::commit()
{
	flush();

}