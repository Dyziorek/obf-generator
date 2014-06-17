#include "stdafx.h"
#include "TileManager.h"
#include "EntityBase.h"
#include "EntityNode.h"
#include "MapObject.h"
#include "Amenity.h"
#include "BatchUpdater.h"
#include "OBFResultDB.h"
#include "MapUtils.h"
#pragma push_macro("realloc")
#undef realloc
#include "SkCanvas.h"
#include "SkSurface.h"
#include "SkImage.h"
#include "SkData.h"
#include "SkStream.h"
#include "SkGraphics.h"
#pragma pop_macro("realloc")
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
#include "BinaryMapDataWriter.h"
#include "OBFMapDB.h"
#include "OBFElementDB.h"
#include "OBFRenderingTypes.h"


OBFResultDB::OBFResultDB(void) 
{
}


OBFResultDB::~OBFResultDB(void)
{

}

void OBFResultDB::close(void)
{
	delete (OBFMapDB*)mapIndexer;
	delete (OBFpoiDB*)poiIndexer;
	delete (OBFtransportDB*)transIndexer;
	delete (OBFrouteDB*)routeIndexer;
	delete (OBFAddresStreetDB*)addresIndexer;
	delete storeData;
	OBFRenderingTypes::rules.clear();
	OBFRenderingTypes::namedRulType.clear();


		int sqlCode;
	sqlCode = sqlite3_finalize(mapStmt);
	sqlCode = sqlite3_finalize(lowStmt);
	
	sqlCode = sqlite3_finalize(routeStmt);
	sqlCode = sqlite3_finalize(baseRouteStmt);
	
	sqlCode = sqlite3_finalize(streetStmt);
	sqlCode = sqlite3_finalize(streetNodeStmt);
	sqlCode = sqlite3_finalize(buildStmt);
	sqlCode = sqlite3_finalize(searchStrStmt);
	sqlCode = sqlite3_finalize(searchStrNoCityStmt);
	sqlCode = sqlite3_finalize(updateCityStmt);
	sqlCode = sqlite3_finalize(searchBuildStmt);
	sqlCode = sqlite3_finalize(removeBuildStmt);
	sqlCode = sqlite3_finalize(searchStrNodeStmt);
	sqlCode = sqlite3_finalize(cityStmt);
	
	
	

	// selectors from d
sqlCode = 	sqlite3_finalize(selNodeStmt);
	sqlCode = sqlite3_finalize(selWayStmt);
	sqlCode = sqlite3_finalize(selRelStmt);
	sqlCode = sqlite3_finalize(itNodeStmt);
	sqlCode = sqlite3_finalize(itWayStmt);
	sqlCode = sqlite3_finalize(itRelStmt);
	sqlCode = sqlite3_finalize(itWayBoundStmt);


	
sqlCode = 	sqlite3_finalize(poiNodeStmt);
	
	sqlCode = sqlite3_close(dbPoiCtx);
	sqlCode = sqlite3_close(dbRouteCtx);
	sqlCode = sqlite3_close(dbAddrCtx);
	sqlCode = sqlite3_close(dbTransCtx);
	sqlCode = sqlite3_close(dbMapCtx);
}


int OBFResultDB::shell_callback(void *pArg, int nArg, char **azArg, char **azCol)
{
	return 0;
}

int OBFResultDB::PrepareDB(sqlite3 *dbCtxSrc)
{
	int dbRes = SQLITE_OK;
	char* errMsg;
	FILE* fp = NULL;
	errno_t err;
	
	OBFRenderingTypes rederer;
	rederer.loadXmlData();

	if (mapIndexer == nullptr)
	{
		mapIndexer = new OBFMapDB();
	}

	if (poiIndexer == nullptr)
	{
		poiIndexer = new OBFpoiDB();
	}

	if (addresIndexer == nullptr)
	{
		addresIndexer = new OBFAddresStreetDB();
	}

	if (routeIndexer == nullptr)
	{
		routeIndexer = new OBFrouteDB();
	}

	if (transIndexer == nullptr)
	{
		transIndexer = new OBFtransportDB();
	}

	if (storeData == nullptr)
	{
		storeData = new BatchUpdater(*this);
	}

	if ((err = fopen_s(&fp, "D:\\osmData\\tempLocalMap.db", "r")) == 0)
	{
		fclose(fp);
		remove("D:\\osmData\\tempLocalMap.db");
	}
	fp = NULL;
	if ((err = fopen_s(&fp,"D:\\osmData\\tempLocalRoute.db", "r"))== 0)
	{
		fclose(fp);
		remove("D:\\osmData\\tempLocalRoute.db");
	}
	fp = NULL;
	if ((err = fopen_s(&fp,"D:\\osmData\\tempLocalAddr.db", "r"))== 0)
	{
		fclose(fp);
		remove("D:\\osmData\\tempLocalAddr.db");
	}
	fp = NULL;
	if ((err = fopen_s(&fp,"D:\\osmData\\tempLocalPoi.db", "r"))== 0)
	{
		fclose(fp);
		remove("D:\\osmData\\tempLocalPoi.db");
	}
	dbRes = sqlite3_open("D:\\osmData\\tempLocalMap.db", &dbMapCtx);
	dbRes = sqlite3_open("D:\\osmData\\tempLocalRoute.db", &dbRouteCtx);
	dbRes = sqlite3_open("D:\\osmData\\tempLocalAddr.db", &dbAddrCtx);
	dbRes = sqlite3_open("D:\\osmData\\tempLocalPoi.db", &dbPoiCtx);
	


	dbRes = sqlite3_exec(dbMapCtx, "create table binary_map_objects (id bigint primary key, name varchar(4096), area smallint, types binary, additionalTypes binary, coordinates binary, innerPolygons binary)", &OBFResultDB::shell_callback,this,&errMsg);
	dbRes = sqlite3_exec(dbMapCtx, "create index binary_map_objects_ind on binary_map_objects (id)", &OBFResultDB::shell_callback,this,&errMsg);

	dbRes = sqlite3_exec(dbMapCtx, "create table low_level_map_objects (id bigint primary key, start_node bigint, end_node bigint, name varchar(1024), nodes binary, type binary, addType binary, level smallint)", &OBFResultDB::shell_callback,this,&errMsg);
	dbRes = sqlite3_exec(dbMapCtx, "create index low_level_map_objects_ind on low_level_map_objects (id)", &OBFResultDB::shell_callback,this,&errMsg);
	dbRes = sqlite3_exec(dbMapCtx, "create index low_level_map_objects_ind_st on low_level_map_objects (start_node, type)", &OBFResultDB::shell_callback,this,&errMsg);
	dbRes = sqlite3_exec(dbMapCtx, "create index low_level_map_objects_ind_end on low_level_map_objects (end_node, type)", &OBFResultDB::shell_callback,this,&errMsg);

	dbRes = sqlite3_prepare_v2(dbMapCtx, "insert into binary_map_objects(id, area, coordinates, innerPolygons, types, additionalTypes, name) values(?1, ?2, ?3, ?4, ?5, ?6, ?7)", sizeof("insert into binary_map_objects(id, area, coordinates, innerPolygons, types, additionalTypes, name) values(?1, ?2, ?3, ?4, ?5, ?6, ?7)"), &mapStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbMapCtx, "insert into low_level_map_objects(id, start_node, end_node, name, nodes, type, addType, level) values(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8)", sizeof("insert into low_level_map_objects(id, start_node, end_node, name, nodes, type, addType, level) values(?1, ?2, ?3, ?4, ?5, ?6, ?7, ?)"), &lowStmt, NULL);

	dbRes = sqlite3_exec(dbMapCtx, "PRAGMA synchronous=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbMapCtx, "PRAGMA count_changes=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbMapCtx, "PRAGMA journal_mode=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbMapCtx, "PRAGMA temp_store=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbMapCtx, "PRAGMA cache_size=100000", NULL, NULL, &errMsg);

	dbRes = sqlite3_exec(dbRouteCtx,"create table route_objects (id bigint primary key, types binary, pointTypes binary, pointIds binary, pointCoordinates binary, name varchar(4096))", &OBFResultDB::shell_callback,this,&errMsg);
	dbRes = sqlite3_exec(dbRouteCtx,"create table baseroute_objects (id bigint primary key, types binary, pointTypes binary, pointIds binary, pointCoordinates binary, name varchar(4096))", &OBFResultDB::shell_callback,this,&errMsg);
	dbRes = sqlite3_exec(dbRouteCtx, "create index route_objects_ind  on route_objects (id)" , &OBFResultDB::shell_callback,this,&errMsg);
	dbRes = sqlite3_exec(dbRouteCtx, "create index baseroute_objects_ind  on route_objects (id)", &OBFResultDB::shell_callback,this,&errMsg);

	dbRes = sqlite3_prepare_v2(dbRouteCtx, "insert into route_objects(id, types, pointTypes, pointIds, pointCoordinates, name) values(?1, ?2, ?3, ?4, ?5, ?6)", sizeof("insert into route_objects(id, types, pointTypes, pointIds, pointCoordinates, name) values(?1, ?2, ?3, ?4, ?5, ?6)"), &routeStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbRouteCtx, "insert into baseroute_objects(id, types, pointTypes, pointIds, pointCoordinates, name) values(?1, ?2, ?3, ?4, ?5, ?6)", sizeof("insert into baseroute_objects(id, types, pointTypes, pointIds, pointCoordinates, name) values(?1, ?2, ?3, ?4, ?5, ?6)"), &baseRouteStmt, NULL);
	
	dbRes = sqlite3_exec(dbRouteCtx, "PRAGMA synchronous=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbRouteCtx, "PRAGMA count_changes=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbRouteCtx, "PRAGMA journal_mode=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbRouteCtx, "PRAGMA temp_store=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbRouteCtx, "PRAGMA cache_size=100000", NULL, NULL, &errMsg);

	dbRes = sqlite3_exec(dbAddrCtx,"create table street (id bigint primary key, latitude double, longitude double, name varchar(1024), name_en varchar(1024), city bigint, citypart varchar(1024))", &OBFResultDB::shell_callback,this,&errMsg);
	dbRes = sqlite3_exec(dbAddrCtx,"create index street_cnp on street (city,citypart,name,id)", &OBFResultDB::shell_callback,this,&errMsg);
    dbRes = sqlite3_exec(dbAddrCtx,"create index street_city on street (city)", &OBFResultDB::shell_callback,this,&errMsg);
    dbRes = sqlite3_exec(dbAddrCtx,"create index street_id on street (id)", &OBFResultDB::shell_callback,this,&errMsg);
    dbRes = sqlite3_exec(dbAddrCtx,"create table building (id bigint, latitude double, longitude double, name2 varchar(1024), name_en2 varchar(1024), lat2 double, lon2 double, interval int, interpolateType varchar(50), name varchar(1024), name_en varchar(1024), street bigint, postcode varchar(1024), primary key(street, id))", &OBFResultDB::shell_callback,this,&errMsg);
    dbRes = sqlite3_exec(dbAddrCtx,"create index building_postcode on building (postcode)", &OBFResultDB::shell_callback,this,&errMsg);
    dbRes = sqlite3_exec(dbAddrCtx,"create index building_street on building (street)", &OBFResultDB::shell_callback,this,&errMsg);
    dbRes = sqlite3_exec(dbAddrCtx,"create index building_id on building (id)", &OBFResultDB::shell_callback,this,&errMsg);
    dbRes = sqlite3_exec(dbAddrCtx,"create table street_node (id bigint, latitude double, longitude double, street bigint, way bigint)", &OBFResultDB::shell_callback,this,&errMsg);
    dbRes = sqlite3_exec(dbAddrCtx,"create index street_node_street on street_node (street)", &OBFResultDB::shell_callback,this,&errMsg);
    dbRes = sqlite3_exec(dbAddrCtx,"create index street_node_way on street_node (way)", &OBFResultDB::shell_callback,this,&errMsg);
	dbRes = sqlite3_exec(dbAddrCtx, "create table city (id bigint primary key, latitude double, longitude double, name varchar(1024), name_en varchar(1024), city_type varchar(32))", &OBFResultDB::shell_callback,this,&errMsg);
	dbRes = sqlite3_exec(dbAddrCtx, "create index city_ind on city (id, city_type)", &OBFResultDB::shell_callback,this,&errMsg);

	dbRes = sqlite3_prepare_v2(dbAddrCtx, "insert into street (id, latitude, longitude, name, name_en, city, citypart) values (?1, ?2, ?3, ?4, ?5, ?6, ?7)", sizeof("insert into street (id, latitude, longitude, name, name_en, city, citypart) values (?1, ?2, ?3, ?4, ?5, ?6, ?7)"), &streetStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbAddrCtx, "insert into street_node (id, latitude, longitude, street, way) values (?1, ?2, ?3, ?4, ?5)", sizeof("insert into street_node (id, latitude, longitude, street, way) values (?1, ?2, ?3, ?4, ?5)"), &streetNodeStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbAddrCtx, "insert into building (id, latitude, longitude, name, name_en, street, postcode, name2, name_en2, lat2, lon2, interval, interpolateType) values (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9 ,?10 ,?11 ,?12 ,?13)", sizeof("insert into building (id, latitude, longitude, name, name_en, street, postcode, name2, name_en2, lat2, lon2, interval, interpolateType) values (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9 ,?10 ,?11 ,?12 ,?13)"), &buildStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbAddrCtx, "SELECT id,latitude,longitude FROM street WHERE ?1 = city AND ?2 = citypart AND ?3 = name", sizeof("SELECT id,latitude,longitude FROM street WHERE ?1 = city AND ?2 = citypart AND ?3 = name"), &searchStrStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbAddrCtx, "SELECT id,name,citypart,latitude,longitude FROM street WHERE ?1 = city AND ?2 = name", sizeof("SELECT id,name,citypart,latitude,longitude FROM street WHERE ?1 = city AND ?2 = name"), &searchStrNoCityStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbAddrCtx, "UPDATE street SET citypart = ?1 WHERE id = ?2", sizeof("UPDATE street SET citypart = ?1 WHERE id = ?2"), &updateCityStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbAddrCtx, "SELECT id FROM building where ?1 = id", sizeof("SELECT id FROM building where ?1 = id"), &searchBuildStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbAddrCtx, "DELETE FROM building where ?1 = id", sizeof("DELETE FROM building where ?1 = id"), &removeBuildStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbAddrCtx, "SELECT way FROM street_node WHERE ?1 = way", sizeof("SELECT way FROM street_node WHERE ?1 = way"),&searchStrNodeStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbAddrCtx, "insert into city (id, latitude, longitude, name, name_en, city_type) values (?1, ?2, ?3, ?4, ?5, ?6)", sizeof("insert into city (id, latitude, longitude, name, name_en, city_type) values (?1, ?2, ?3, ?4, ?5, ?6)"), &cityStmt, NULL);

	dbRes = sqlite3_exec(dbAddrCtx, "PRAGMA synchronous=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbAddrCtx, "PRAGMA count_changes=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbAddrCtx, "PRAGMA journal_mode=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbAddrCtx, "PRAGMA temp_store=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbAddrCtx, "PRAGMA cache_size=100000", NULL, NULL, &errMsg);

	dbRes = sqlite3_exec(dbPoiCtx, "create table poi (id bigint, x int, y int, type varchar(1024), subtype varchar(1024), additionalTags varchar(8096), primary key(id, type, subtype))", &OBFResultDB::shell_callback,this,&errMsg);
	dbRes = sqlite3_exec(dbPoiCtx, "create index poi_loc on poi (x, y, type, subtype)", &OBFResultDB::shell_callback,this,&errMsg);
	dbRes = sqlite3_exec(dbPoiCtx, "create index poi_id on poi (id, type, subtype)", &OBFResultDB::shell_callback,this,&errMsg);
		
	
	dbRes = sqlite3_prepare_v2(dbPoiCtx, "INSERT INTO poi (id, x, y, type, subtype, additionalTags) VALUES (?1, ?2, ?3, ?4, ?5, ?6)", sizeof("INSERT INTO poi (id, x, y, type, subtype, additionalTags) VALUES (?1, ?2, ?3, ?4, ?5, ?6)"),  &poiNodeStmt, NULL); //$NON-NLS-1$
	dbRes = sqlite3_exec(dbPoiCtx, "PRAGMA synchronous=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbPoiCtx, "PRAGMA count_changes=OFF", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbPoiCtx, "PRAGMA journal_mode=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbPoiCtx, "PRAGMA temp_store=MEMORY", NULL, NULL, &errMsg);
	dbRes = sqlite3_exec(dbPoiCtx, "PRAGMA cache_size=100000", NULL, NULL, &errMsg);
	// reopen node way rel dbx and attach them into single connection
	//dbRes = sqlite3_open("D:\\osmData\\tempLocalNode.db", &dbCtx);
	//sqlite_exec(
	// selectors


	dbRes = sqlite3_prepare_v2(dbCtxSrc, "select n.latitude, n.longitude, n.tags from node n where n.id = ?1", sizeof("select n.latitude, n.longitude, n.tags from node n where n.id = ?1"),  &selNodeStmt, NULL); //$NON-NLS-1$
	dbRes = sqlite3_prepare_v2(dbCtxSrc, "select w.node, w.ord, w.tags, n.latitude, n.longitude, n.tags from ways w left join node n on w.node = n.id where w.wayid = ?1 order by w.ord", sizeof("select w.node, w.ord, w.tags, n.latitude, n.longitude, n.tags from ways w left join node n on w.node = n.id where w.wayid = ?1 order by w.ord"), &selWayStmt, NULL); //$NON-NLS-1$
	dbRes = sqlite3_prepare_v2(dbCtxSrc, "select r.member, r.type, r.role, r.ord, r.tags from relations r where r.relid = ?1 order by r.ord", sizeof("select r.member, r.type, r.role, r.ord, r.tags from relations r where r.relid = ?1 order by r.ord"), &selRelStmt, NULL); //$NON-NLS-1$
		
	dbRes = sqlite3_prepare_v2(dbCtxSrc,"select n.id, n.latitude, n.longitude, n.tags from node n where length(n.tags) > 0", sizeof("select n.id, n.latitude, n.longitude, n.tags from node n where length(n.tags) > 0"), &itNodeStmt, NULL);
	dbRes = sqlite3_prepare_v2(dbCtxSrc, "select w.wayid, w.node, w.ord, w.tags, n.latitude, n.longitude, n.tags from ways w left join node n on w.node = n.id order by w.id",sizeof("select w.wayid, w.node, w.ord, w.tags, n.latitude, n.longitude, n.tags from ways w left join node n on w.node = n.id order by w.id"), &itWayStmt, NULL); //$NON-NLS-1$
	dbRes = sqlite3_prepare_v2(dbCtxSrc, "select w.wayid, w.node, w.ord, w.tags, n.latitude, n.longitude, n.tags from ways w left join node n on w.node = n.id  where w.boundary > 0 order by w.id", sizeof("select w.wayid, w.node, w.ord, w.tags, n.latitude, n.longitude, n.tags from ways w left join node n on w.node = n.id  where w.boundary > 0 order by w.id"), &itWayBoundStmt, NULL); //$NON-NLS-1$
	dbRes = sqlite3_prepare_v2(dbCtxSrc, "select r.id, r.relid, r.tags from relations r where length(r.tags) > 0", sizeof("select r.id, r.relid, r.tags from relations r where length(r.tags) > 0"), &itRelStmt, NULL); //$NON-NLS-1$


	return 0;
}

void OBFResultDB::imageResult()
{
	OBFAddresStreetDB* addresor = (OBFAddresStreetDB*)addresIndexer;

	((OBFMapDB*)mapIndexer)->paintPolys();
	//((OBFMapDB*)mapIndexer)->paintTreeData(*this, addresor->boundaries, addresor->cities);
}

int OBFResultDB::iterateOverElements(int iterationPhase)
{
	if (iterationPhase == PHASEINDEXCITY)
	{
		iterateOverElements(NODEELEM, 
			[=](std::shared_ptr<EntityBase> itm) {  
				if (itm->getTag("place") != std::string(""))
				{
					 std::shared_ptr<EntityNode> ptrNode = std::dynamic_pointer_cast<EntityNode, EntityBase>(itm);
					 ((OBFAddresStreetDB*)addresIndexer)->iterateOverCity(ptrNode);
				}
		});
		((OBFAddresStreetDB*)addresIndexer)->storeCities(*this);
	}
	if (iterationPhase == PHASEINDEXADDRREL)
	{
		int numbers = 0;
		iterateOverElements(NODEREL,
			[&](std::shared_ptr<EntityBase> itm) {  
				
				std::shared_ptr<EntityRelation> relItem = std::dynamic_pointer_cast<EntityRelation, EntityBase>(itm);

				((OBFAddresStreetDB*)addresIndexer)->indexBoundary(itm, *this);
				

				((OBFMapDB*)mapIndexer)->indexMapAndPolygonRelations(relItem, *this);

				((OBFrouteDB*)routeIndexer)->indexRelations(relItem, *this);

				((OBFpoiDB*)poiIndexer)->indexRelations(relItem, *this);

				//relations.insert(relItem);
				numbers++;
				
		});

		std::wstringstream strbuf;
		strbuf << L"Counter relations: ";
		strbuf << numbers;
		strbuf << std::endl;
		std::wstring buffText = strbuf.str();
		OutputDebugString(buffText.c_str());

		numbers = 0;
		iterateOverElements(NODEWAYBOUNDARY,
			[=, &numbers](std::shared_ptr<EntityBase> itm) {  
				((OBFAddresStreetDB*)addresIndexer)->indexBoundary(itm, *this);
				numbers++;
		});

		((OBFAddresStreetDB*)addresIndexer)->tryToAssignBoundaryToFreeCities();

		iterateOverElements(NODEREL,
			[&](std::shared_ptr<EntityBase> itm) { 
				std::shared_ptr<EntityRelation> relItem = std::dynamic_pointer_cast<EntityRelation, EntityBase>(itm);
				((OBFAddresStreetDB*)addresIndexer)->indexAddressRelation(relItem, *this);
		});
		strbuf.str(L"");
		strbuf.clear();
		strbuf << L"Counter boundaries: ";
		strbuf << numbers;
		strbuf << std::endl;
		buffText = strbuf.str();
		OutputDebugString(buffText.c_str());
		/*
		iterateOverElements(NODEREL,
			[&](EntityBase* itm) { 
				// Address relation
		});
		*/

	}
	if (iterationPhase == PHASEMAINITERATE)
	{
		int numbers = 0;
		iterateOverElements(NODEELEM, 
			[=, &numbers](std::shared_ptr<EntityBase> itm) {  
				mainIteration(itm);
				numbers++;
		});

		std::wstringstream strbuf;
		strbuf << L"Counter node elements: ";
		strbuf << numbers;
		strbuf << std::endl;
		std::wstring buffText = strbuf.str();
		OutputDebugString(buffText.c_str());
		numbers = 0;
		iterateOverElements(NODEREL,
			[&](std::shared_ptr<EntityBase> itm) { 
				mainIteration(itm);
				numbers++;
		});
		strbuf.str(L"");
		strbuf.clear();
		strbuf << L"Counter node relations: ";
		strbuf << numbers;
		strbuf << std::endl;
		buffText = strbuf.str();
		OutputDebugString(buffText.c_str());
		numbers = 0;
		iterateOverElements(NODEWAY,
			[=, &numbers](std::shared_ptr<EntityBase> itm) {  
			mainIteration(itm);
			numbers++;
		});
		strbuf.str(L"");
		strbuf.clear();
		strbuf << L"Counter node ways: ";
		strbuf << numbers;
		strbuf << std::endl;
		buffText = strbuf.str();
		OutputDebugString(buffText.c_str());
		numbers = 0;
	}
	if (iterationPhase == PHASECOMBINE)
	{
		((OBFMapDB*)mapIndexer)->processLowLevelWays(*this);
		this->storeData->commit();
	}
	if (iterationPhase == PHASESAVE)
	{
		boost::filesystem::path pather("D:\\OsmData\\binarResult.bin");
		
		RandomAccessFile rafek(pather, RandomAccessFile::READWRITE);

		BinaryMapDataWriter writter(&rafek);

		((OBFMapDB*)mapIndexer)->writeBinaryMapIndex(writter, mapName, *this);
		((OBFAddresStreetDB*)addresIndexer)->writeAddresMapIndex(writter,mapName, *this);
		//((OBFpoiDB*)poiIndexer)->writePoiMapIndex(writter, mapName, *this);
		//((OBFrouteDB*)routeIndexer)->writeRouteMapIndex(writter, mapName, *this);
	}
	return 0;
}

void OBFResultDB::iterateMainEntity(std::shared_ptr<EntityBase>& relItem, OBFResultDB& dbContext)
{

}

void OBFResultDB::mainIteration(std::shared_ptr<EntityBase>& relItem)
{
	mapIndexer->iterateMainEntity(relItem, *this);
	poiIndexer->iterateMainEntity(relItem, *this);
	transIndexer->iterateMainEntity(relItem, *this);
	routeIndexer->iterateMainEntity(relItem, *this);
	addresIndexer->iterateMainEntity(relItem, *this);
}


void OBFResultDB::loadWays(EntityWay* wayItem)
{

	// select w.node, w.ord, w.tags, n.latitude, n.longitude, n.tags from ways
	int  dbRet = SQLITE_OK;
	sqlite3_reset(selWayStmt);
	sqlite3_bind_int64(selWayStmt, 1, wayItem->id);
	dbRet = sqlite3_step(selWayStmt);

	std::list<std::shared_ptr<EntityNode>> nodeList;
	if (dbRet != SQLITE_ROW)
	{
		sqlite3_reset(selWayStmt);
		if (dbRet == SQLITE_DONE)
		{
			// not found exiting
			return;
		}
	}
	
	do
	{
		__int64 nodeId = sqlite3_column_int64(selWayStmt, 0);
		int ord = sqlite3_column_int(selWayStmt, 1);
		if (ord == 0)
		{
			const unsigned char* columnTextData = sqlite3_column_text(selWayStmt, 2);
			if (columnTextData != nullptr)
			{
				std::string tags(reinterpret_cast<const char*>(sqlite3_column_text(selWayStmt, 2)));
				wayItem->parseTags(tags);
			}
			else
			{
				std::wstring error = L"Problem on tags colum data size: ";
				error += boost::lexical_cast<std::wstring, int>(sqlite3_column_bytes(selWayStmt, 2));
				error += L"\r\n";
				OutputDebugString(error.c_str());
			}
		}
		if (sqlite3_column_type(selWayStmt, 4) != SQLITE_NULL)
		{
			if (nodes.find(nodeId) == nodes.end())
			{
				std::shared_ptr<EntityNode> node(new EntityNode);
				node->id = nodeId;
				node->lat = sqlite3_column_double(selWayStmt, 3);
				node->lon = sqlite3_column_double(selWayStmt, 4);
				if (sqlite3_column_type(selWayStmt, 5) != SQLITE_NULL)
				{
					const unsigned char* columnTextData = sqlite3_column_text(selWayStmt, 5);
					if (columnTextData != nullptr)
					{
						std::string tags(reinterpret_cast<const char*>(sqlite3_column_text(selWayStmt, 5)));
						node->parseTags(tags);
					}
					else
					{
						std::wstring error = L"Problem on tags colum data size: ";
						error += boost::lexical_cast<std::wstring, int>(sqlite3_column_bytes(selWayStmt, 5));
						error += L"\r\n";
						OutputDebugString(error.c_str());
					}

				}
				nodeList.push_back(node);
			}
			else
			{
				std::shared_ptr<EntityNode> node = nodes.at(nodeId);
				nodeList.push_back(node);
			}
		}
		wayItem->nodeIDs.push_back(nodeId);
		dbRet = sqlite3_step(selWayStmt);
		
	}
	while (dbRet == SQLITE_ROW);
	
	wayItem->nodes.swap(std::vector<std::shared_ptr<EntityNode>>(nodeList.begin(), nodeList.end()));

}


void OBFResultDB::loadRelationMembers(EntityRelation* relItem)
{
	if (relItem->entityIDs.size() > 0)
	{
		return;
	}
	int  dbRet = SQLITE_OK;
	sqlite3_reset(selRelStmt);
	sqlite3_bind_int64(selRelStmt, 1, relItem->id);
	dbRet = sqlite3_step(selRelStmt);

	if (dbRet != SQLITE_ROW)
	{
		sqlite3_reset(selRelStmt);
		if (dbRet == SQLITE_DONE)
		{
			// not found exiting
			return;
		}
	}
	
	do
	{
		__int64 id = sqlite3_column_int64(selRelStmt, 0);
		int typeRel = sqlite3_column_int(selRelStmt, 1);
		std::string role(reinterpret_cast<const char*>(sqlite3_column_text(selRelStmt, 2)));
		int ord = sqlite3_column_int(selRelStmt, 3);
		//std::string tags(reinterpret_cast<const char*>(sqlite3_column_text(selRelStmt, 4)));
		relItem->entityIDs.push_back(std::make_pair(id,std::make_pair(typeRel,role)));
		dbRet = sqlite3_step(selRelStmt);
	}
	while (dbRet == SQLITE_ROW);
	

}

void OBFResultDB::loadNodesOnRelation(EntityRelation* relItem)
{
	if (relItem == nullptr)
		return;

	for(auto relIDMember : relItem->entityIDs)
	{
		int  dbRet = SQLITE_OK;
		if (relIDMember.second.first == 0)
		{
			sqlite3_reset(selNodeStmt);
			sqlite3_bind_int64(selNodeStmt, 1, relIDMember.first);
			dbRet = sqlite3_step(selNodeStmt);

			if (dbRet != SQLITE_ROW)
			{
				sqlite3_reset(selNodeStmt);
				if (dbRet == SQLITE_DONE)
				{
					// not found exiting
					continue;
				}
			}

			do
			{
				std::shared_ptr<EntityBase> node(new EntityNode);
				node->id = relIDMember.first;
				EntityNode* nodeData = reinterpret_cast<EntityNode*>(node.operator->());
				nodeData->lat = sqlite3_column_double(selNodeStmt, 0);
				nodeData->lon = sqlite3_column_double(selNodeStmt, 1);
				const unsigned char* columnTextData = sqlite3_column_text(selNodeStmt, 2);
				if (columnTextData != nullptr)
				{
					std::string tags(reinterpret_cast<const char*>(sqlite3_column_text(selNodeStmt, 2)));
					nodeData->parseTags(tags);
				}
				else
				{
					std::wstring error = L"Problem on tags colum data size: ";
					error += boost::lexical_cast<std::wstring, int>(sqlite3_column_bytes(selNodeStmt, 2));
					error += L"\r\n";
					OutputDebugString(error.c_str());
				}
				//std::pair<int,std::shared_ptr<EntityBase>> mapPair = std::make_pair(0,node);
				relItem->relations[relIDMember.first] = std::make_tuple(0,node, relIDMember.second.second);
				dbRet = sqlite3_step(selNodeStmt);
			}
			while (dbRet == SQLITE_ROW);
		}
		if (relIDMember.second.first == 1)
		{
				std::shared_ptr<EntityWay> way(new EntityWay);
				way->id = relIDMember.first;
				loadWays(way.get());
				relItem->relations[relIDMember.first] = std::make_tuple(1,way, relIDMember.second.second);
		}

		if (relIDMember.second.first == 2)
		{
			std::shared_ptr<EntityRelation> relation(new EntityRelation);
			relation->id = relIDMember.first;
			loadRelationMembers(relation.get());
			relItem->relations[relIDMember.first] = std::make_tuple(2,relation, relIDMember.second.second);
		}


	}
}

void OBFResultDB::SaverCityNode(EntityBase* nn, TileManager<CityObj>& manager)
{
	
	CityObj objCity;
	
	MapObject::parseMapObject(&objCity, nn);
	
	manager.registerObject(objCity.getLatLon().first, objCity.getLatLon().second, objCity);

}



int OBFResultDB::iterateOverElements(int type, std::function<void (std::shared_ptr<EntityBase>)> saver)
{
	int  dbRet = SQLITE_OK;
	if (type == NODEELEM)
	{
		dbRet = sqlite3_step(itNodeStmt);

		if (dbRet != SQLITE_ROW)
		{
			sqlite3_reset(itNodeStmt);
		}
	
		do
		{
			__int64 nodeId = sqlite3_column_int64(itNodeStmt, 0);
			if (nodes.find(nodeId) == nodes.end())
			{
				std::shared_ptr<EntityNode> nodeData(new EntityNode);
				nodeData->id = sqlite3_column_int64(itNodeStmt, 0);
				nodeData->lat = sqlite3_column_double(itNodeStmt, 1);
				nodeData->lon = sqlite3_column_double(itNodeStmt, 2);
				const unsigned char* columnTextData = sqlite3_column_text(itNodeStmt, 3);
				if (columnTextData != nullptr)
				{
					std::string tags(reinterpret_cast<const char*>(sqlite3_column_text(itNodeStmt, 3)));
					nodeData->parseTags(tags);
				}
				else
				{
					std::wstring error = L"Problem on tags colum data size: ";
					error += boost::lexical_cast<std::wstring, int>(sqlite3_column_bytes(itNodeStmt, 3));
					error += L"\r\n";
					OutputDebugString(error.c_str());
				}
				saver(nodeData);
				//nodes.insert(std::make_pair(nodeId, nodeData));
			}
			else
			{
				std::shared_ptr<EntityNode> nodeData = nodes.at(nodeId);
				saver(nodeData);
			}
			dbRet = sqlite3_step(itNodeStmt);
		}
		while (dbRet == SQLITE_ROW);
	
		return 0;
	}

	if (type == NODEREL)
	{
		dbRet = sqlite3_step(itRelStmt);

		if (dbRet != SQLITE_ROW)
		{
			sqlite3_reset(itRelStmt);
			if (dbRet == SQLITE_DONE)
			{
				return -1;
			}
		}
		do
		{
			__int64 nodeId = sqlite3_column_int64(itRelStmt, 0);
			if (relNodes.find(nodeId) == relNodes.end())
			{
				std::shared_ptr<EntityRelation> nodeData(new EntityRelation);
				nodeData->id = sqlite3_column_int64(itRelStmt, 1);
				const unsigned char* columnTextData = sqlite3_column_text(itRelStmt, 2);
				if (columnTextData != nullptr)
				{
					std::string tags(reinterpret_cast<const char*>(sqlite3_column_text(itRelStmt, 2)));
					nodeData->parseTags(tags);
				}
				else
				{
					std::wstring error = L"Problem on tags colum data size: ";
					error += boost::lexical_cast<std::wstring, int>(sqlite3_column_bytes(itRelStmt, 2));
					error += L"\r\n";
					OutputDebugString(error.c_str());
				}
				saver(nodeData);
				//relNodes.insert(std::make_pair(nodeId, nodeData));
			}
			else
			{
				std::shared_ptr<EntityRelation> nodeData = relNodes.at(nodeId);
				saver(nodeData);
			}
			dbRet = sqlite3_step(itRelStmt);
		}
		while (dbRet == SQLITE_ROW);
	
		return 0;
	}
	
	if (type == NODEWAY)
	{
		dbRet = sqlite3_step(itWayStmt);

		if (dbRet != SQLITE_ROW)
		{
			sqlite3_reset(itWayStmt);
			if (dbRet == SQLITE_DONE)
			{
				return -1;
			}
		}
		std::shared_ptr<EntityWay> prevEntity;
		__int64 prevId = 0;
		do
		{
			// select w.wayid, w.node, w.ord, w.tags, n.latitude, n.longitude, n.tags from ways w left join node n on w.node = n.id order by w.id
			__int64 wayID = sqlite3_column_int64(itWayStmt, 0);
			if (ways.find(wayID) == ways.end())
			{
				std::shared_ptr<EntityWay> wayData;
				bool newId = (wayID != prevId);
				if (prevEntity && !newId)
				{
					 wayData = prevEntity;
				}
				else
				{
					wayData.reset(new EntityWay);		
				}
				wayData->id = wayID;
				int ord =  sqlite3_column_int(itWayStmt, 2);
				if (ord == 0){
					const unsigned char* columnTextData = sqlite3_column_text(itWayStmt, 3);
					if (columnTextData != nullptr)
					{
						std::string tags(reinterpret_cast<const char*>(sqlite3_column_text(itWayStmt, 3)));
						wayData->parseTags(tags);
					}
					else
					{
						std::wstring error = L"Problem on tags colum data size: ";
						error += boost::lexical_cast<std::wstring, int>(sqlite3_column_bytes(itWayStmt, 3));
						error += L"\r\n";
						OutputDebugString(error.c_str());
					}
				}
				if (sqlite3_column_type(itWayStmt, 4) != SQLITE_NULL)
				{
					__int64 nodeId = sqlite3_column_int64(itWayStmt, 1);
					if (nodes.find(nodeId) == nodes.end())
					{
						std::shared_ptr<EntityNode> entNode(new EntityNode);
						entNode->id = nodeId;
						entNode->lat = sqlite3_column_double(itWayStmt, 4);
						entNode->lon = sqlite3_column_double(itWayStmt, 5);
						const unsigned char* columnTextData = sqlite3_column_text(itWayStmt, 6);
						if (columnTextData != nullptr)
						{
							std::string tags(reinterpret_cast<const char*>(sqlite3_column_text(itWayStmt, 6)));
							entNode->parseTags(tags);
						}
						else
						{
							std::wstring error = L"Problem on tags colum data size: ";
							error += boost::lexical_cast<std::wstring, int>(sqlite3_column_bytes(itWayStmt, 6));
							error += L"\r\n";
							OutputDebugString(error.c_str());
						}
						wayData->nodes.push_back(entNode);
						wayData->nodeIDs.push_back(entNode->id);
					}
					else
					{
						std::shared_ptr<EntityNode> entNode = nodes.at(nodeId);
						wayData->nodes.push_back(entNode);
						wayData->nodeIDs.push_back(entNode->id);
					}
				}
				else
				{
					wayData->nodeIDs.push_back(sqlite3_column_int64(itWayStmt, 1));
				}
				if (prevEntity && newId)
				{
					saver(prevEntity);
					//ways.insert(std::make_pair(prevEntity->id, prevEntity));
				}
				prevEntity = wayData;
				prevId = wayData->id;
			}
			else
			{
				std::shared_ptr<EntityWay> wayData = ways.at(wayID);
				saver(wayData);

			}
			dbRet = sqlite3_step(itWayStmt);
		}
		while (dbRet == SQLITE_ROW);
	
		return 0;
	}
	if (type == NODEWAYBOUNDARY)
	{
		dbRet = sqlite3_step(itWayBoundStmt);

		if (dbRet != SQLITE_ROW)
		{
			sqlite3_reset(itWayBoundStmt);
			if (dbRet == SQLITE_DONE)
			{
				return -1;
			}
		}
		std::shared_ptr<EntityWay> prevEntity;
		__int64 prevId = 0;
		do
		{
			// select w.wayid, w.node, w.ord, w.tags, n.latitude, n.longitude, n.tags from ways w left join node n on w.node = n.id order by w.id, w.ord
			__int64 wayID = sqlite3_column_int64(itWayBoundStmt, 0);
			if (waybounds.find(wayID) == waybounds.end())
			{
				std::shared_ptr<EntityWay> wayData;
				bool newId = (wayID != prevId);
				if (prevEntity && !newId)
				{
					 wayData = prevEntity;
				}
				else
				{
					wayData.reset(new EntityWay);		
				}
				wayData->id = wayID;
				int ord =  sqlite3_column_int(itWayBoundStmt, 2);
				if (ord == 0){
					const unsigned char* columnTextData = sqlite3_column_text(itWayBoundStmt, 3);
					if (columnTextData != nullptr)
					{
						std::string tags(reinterpret_cast<const char*>(sqlite3_column_text(itWayBoundStmt, 3)));
						wayData->parseTags(tags);
					}
					else
					{
						std::wstring error = L"Problem on tags colum data size: ";
						error += boost::lexical_cast<std::wstring, int>(sqlite3_column_bytes(itWayBoundStmt, 3));
						error += L"\r\n";
						OutputDebugString(error.c_str());
					}
				}
				if (sqlite3_column_type(itWayBoundStmt, 4) != SQLITE_NULL)
				{
					__int64 nodeId = sqlite3_column_int64(itWayBoundStmt, 1);
					if (nodes.find(nodeId) == nodes.end())
					{
						std::shared_ptr<EntityNode> entNode(new EntityNode);
						entNode->id = nodeId;
						entNode->lat = sqlite3_column_double(itWayBoundStmt, 4);
						entNode->lon = sqlite3_column_double(itWayBoundStmt, 5);
						const unsigned char* columnTextData = sqlite3_column_text(itWayBoundStmt, 6);
						if (columnTextData != nullptr)
						{
							std::string tags(reinterpret_cast<const char*>(sqlite3_column_text(itWayBoundStmt, 6)));
							entNode->parseTags(tags);
						}
						else
						{
							std::wstring error = L"Problem on tags colum data size: ";
							error += boost::lexical_cast<std::wstring, int>(sqlite3_column_bytes(itWayBoundStmt, 6));
							error += L"\r\n";
							OutputDebugString(error.c_str());
						}
						wayData->nodes.push_back(entNode);
						wayData->nodeIDs.push_back(entNode->id);
					}
					else
					{
						std::shared_ptr<EntityNode> entNode = nodes.at(nodeId);
						wayData->nodes.push_back(entNode);
						wayData->nodeIDs.push_back(entNode->id);
					}
				}
				else
				{
					wayData->nodeIDs.push_back(sqlite3_column_int64(itWayBoundStmt, 1));
				}
			
				if (prevEntity && newId)
				{
					saver(prevEntity);
					//waybounds.insert(std::make_pair(prevEntity->id, prevEntity));
				}
				prevEntity = wayData;
				prevId = wayData->id;
			}
			else
			{
				std::shared_ptr<EntityWay> wayData = waybounds.at(wayID);
				saver(wayData);
			}
			dbRet = sqlite3_step(itWayBoundStmt);
		}
		while (dbRet == SQLITE_ROW);
	
		return 0;
	}

}



void OBFResultDB::addBatch(Amenity am)
{
	storeData->addBatch(am);
}

void OBFResultDB::addBatch(__int64 id, __int64 firstId, __int64 lastId, std::string& name, std::stringstream& bNodes,std::stringstream& bTypes,std::stringstream& bAddtTypes,int level)
{
	storeData->addBatch(id,firstId, lastId, name, bNodes, bTypes, bAddtTypes, level );
}

void OBFResultDB::addBatch(__int64 id, bool area, std::stringstream& bCoord, std::stringstream& bInCoord ,std::stringstream& bTypes,std::stringstream& bAddtTypes,std::string& name)
{
	storeData->addBatch(id,area, bCoord, bInCoord, bTypes, bAddtTypes, name );
}

void OBFResultDB::addBatchRoute(__int64 id, std::stringstream& types, std::stringstream& ptTypes ,std::stringstream& ptIds,std::stringstream& coords, std::string& name, bool base)
{
	storeData->addBatchRoute(id,types, ptTypes, ptIds, coords, name, base );
}

void OBFResultDB::flush()
{
	storeData->commit();
}