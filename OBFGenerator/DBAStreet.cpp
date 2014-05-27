#include "stdafx.h"
#include "Building.h"
#include "DBAStreet.h"


__int64 DBAStreet::streetIdSeq = 0;

DBAStreet::DBAStreet(OBFResultDB& dbContext) :  workCtx(dbContext)
{

}

DBAStreet::~DBAStreet(void)
{
}

std::unique_ptr<SimpleStreet> DBAStreet::findStreet(std::string name,CityObj city,std::string cityPart)
{
		if(cityPart == "") {
			return findStreet(name, city);
		}
		//SELECT id,latitude,longitude FROM street WHERE ?1 = city AND ?2 = citypart AND ?3 = name
		sqlite3_stmt* addrSearch = workCtx.searchStrStmt;
		sqlite3_bind_int64(addrSearch, 1, city.getID());
		sqlite3_bind_text(addrSearch, 2, cityPart.c_str(), cityPart.size(), SQLITE_TRANSIENT);
		sqlite3_bind_text(addrSearch, 3, name.c_str(), name.size(), SQLITE_TRANSIENT);
		int sqlDB = sqlite3_step(addrSearch);
		__int64 idStreet;
		std::pair<double,double> latLon;
		if (sqlDB == SQLITE_ROW)
		{
			idStreet = sqlite3_column_int64(addrSearch, 0);
			latLon.first = sqlite3_column_double(addrSearch, 1);
			latLon.second = sqlite3_column_double(addrSearch, 2);
			std::unique_ptr<SimpleStreet> foundStreet(new SimpleStreet(idStreet, name, cityPart, latLon));
			return foundStreet;
		}
		return std::unique_ptr<SimpleStreet>();
}

std::unique_ptr<SimpleStreet> DBAStreet::findStreet(std::string name,CityObj city)
{
		//SELECT id,name,citypart,latitude,longitude FROM street WHERE ?1 = city AND ?2 = name
		sqlite3_stmt* addrSearch = workCtx.searchStrNoCityStmt;
		sqlite3_bind_int64(addrSearch, 1, city.getID());
		sqlite3_bind_text(addrSearch, 2, name.c_str(), name.size(), SQLITE_TRANSIENT);
		int sqlDB = sqlite3_step(addrSearch);
		__int64 idStreet;
		std::pair<double,double> latLon;
		if (sqlDB == SQLITE_ROW)
		{
			idStreet = sqlite3_column_int64(addrSearch, 0);
			latLon.first = sqlite3_column_double(addrSearch, 3);
			latLon.second = sqlite3_column_double(addrSearch, 4);
			std::string cname((const char*)sqlite3_column_text(addrSearch, 1));
			std::string cityPart((const char*)sqlite3_column_text(addrSearch, 2));
			std::unique_ptr<SimpleStreet> foundStreet(new SimpleStreet(idStreet, cname, cityPart, latLon));
			return foundStreet;
		}
		return std::unique_ptr<SimpleStreet>();

}

__int64 DBAStreet::insertStreet(std::string name,std::string nameEn,LatLon location, CityObj city,std::string cityPart)
{
	//"insert into street (id, latitude, longitude, name, name_en, city, citypart) values (?1, ?2, ?3, ?4, ?5, ?6, ?7)"
	sqlite3_stmt* addrInsert = workCtx.streetStmt;
	sqlite3* streetCtx = workCtx.dbAddrCtx;
	char* errMsg;
		int SqlCode;
	sqlite3_exec(streetCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
	sqlite3_bind_int64(addrInsert, 1, city.getID());
	sqlite3_bind_text(addrInsert, 4, name.c_str(), name.size(), SQLITE_TRANSIENT);
	sqlite3_bind_text(addrInsert, 5, nameEn.c_str(), nameEn.size(), SQLITE_TRANSIENT);
	sqlite3_bind_int64(addrInsert, 6, streetIdSeq++);
	sqlite3_bind_text(addrInsert, 5, cityPart.c_str(), cityPart.size(), SQLITE_TRANSIENT);
	SqlCode = sqlite3_step(addrInsert);
	SqlCode = sqlite3_clear_bindings(addrInsert);
	SqlCode = sqlite3_reset(addrInsert);
	sqlite3_exec(streetCtx, "END TRANSACTION", NULL, NULL, &errMsg);

	return streetIdSeq;
}

bool DBAStreet::findBuilding(std::shared_ptr<EntityBase> house)
{
	sqlite3_stmt* searchBuildStmt = workCtx.searchBuildStmt;
	sqlite3_bind_int64(searchBuildStmt, 1, house->id);
	int SqlCode = sqlite3_step(searchBuildStmt);
	return (SqlCode == SQLITE_ROW || SqlCode == SQLITE_OK || SqlCode == SQLITE_DONE);
}

void DBAStreet::writeBuilding(std::set<long long> idsOfStreet, Building building)
{
	//"insert into building (id, latitude, longitude, name, name_en, street, postcode, name2, name_en2, lat2, lon2, interval, interpolateType) values (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9 ,?10 ,?11 ,?12 ,?13)"
	sqlite3_stmt* bldInsert = workCtx.buildStmt;
	sqlite3* bldCtx = workCtx.dbAddrCtx;
	char* errMsg;
		int SqlCode;
	sqlite3_exec(bldCtx, "BEGIN TRANSACTION", NULL, NULL, &errMsg);
	for (long long id : idsOfStreet)
	{
		sqlite3_bind_int64(bldInsert, 1, building.getID());
		sqlite3_bind_double(bldInsert, 2, building.getLatLon().first);
		sqlite3_bind_double(bldInsert, 3, building.getLatLon().second);
		sqlite3_bind_text(bldInsert, 4, building.getName().c_str(), building.getName().size(), SQLITE_TRANSIENT);
		sqlite3_bind_text(bldInsert, 5, building.getEnName().c_str(), building.getEnName().size(), SQLITE_TRANSIENT);
		sqlite3_bind_int64(bldInsert, 6, id);
		std::string postCode = boost::to_upper_copy(building.postCode);
		sqlite3_bind_text(bldInsert, 7, postCode.c_str(), postCode.size(), SQLITE_TRANSIENT);
		sqlite3_bind_text(bldInsert, 8, building.getName2().c_str(), building.getName2().size(), SQLITE_TRANSIENT);
		sqlite3_bind_text(bldInsert, 9, building.getName2().c_str(), building.getName2().size(), SQLITE_TRANSIENT);
		sqlite3_bind_double(bldInsert, 10, building.getLatLon2().first == -1000 ? 0 : building.getLatLon2().first);
		sqlite3_bind_double(bldInsert, 11, building.getLatLon2().second == -1000 ? 0 : building.getLatLon2().second);
		sqlite3_bind_int(bldInsert, 12, building.getInterval());
		std::string interpCode = "";
		switch (building.interpType)
		{
		case Building::Interpolation::ALL:
			interpCode = "ALL";
			break;
		case Building::Interpolation::EVEN:
			interpCode = "EVEN";
			break;
		case Building::Interpolation::ODD:
			interpCode = "ODD";
			break;
		case Building::Interpolation::ALPHA:
			interpCode = "ALPHA";
			break;
		default:
			break;
		}
		sqlite3_bind_text(bldInsert, 13, interpCode.c_str(), interpCode.size(), SQLITE_TRANSIENT);
		SqlCode = sqlite3_step(bldInsert);
		SqlCode = sqlite3_clear_bindings(bldInsert);
		SqlCode = sqlite3_reset(bldInsert);
	}
	sqlite3_exec(bldCtx, "END TRANSACTION", NULL, NULL, &errMsg);
}