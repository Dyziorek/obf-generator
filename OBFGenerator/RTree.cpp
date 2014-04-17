#include "stdafx.h"
#include <boost\geometry.hpp>
#include "RTree.h"




RTree::RTree(void)
{
}


RTree::~RTree(void)
{
}


void RTree::paintTreeData(OBFResultDB& dbContext)
{
	box boundaries = calculateBounds();

	double lowX = MapUtils::get31LongitudeX(boundaries.min_corner().get<0>());
	double lowY = MapUtils::get31LatitudeY(boundaries.min_corner().get<1>());

	double hiX = MapUtils::get31LongitudeX(boundaries.max_corner().get<0>());
	double hiY = MapUtils::get31LatitudeY(boundaries.max_corner().get<1>());

	std::vector<value> retVec;
	spaceTree.query(bgi::intersects(boundaries), std::back_inserter(retVec));
	std::vector<__int64> vecIds;
	std::for_each(retVec.begin(), retVec.end(),[&vecIds](value result) { vecIds.push_back(result.second);});

	int dbCode;
	sqlite3* dbCtx = dbContext.dbMapCtx;
	sqlite3_stmt* mapSelStmt;
	dbCode = sqlite3_prepare_v2(dbCtx, "SELECT id, area, coordinates, innerPolygons, types, additionalTypes, name FROM binary_map_objects WHERE id = 1?" , sizeof("SELECT id, area, coordinates, innerPolygons, types, additionalTypes, name FROM binary_map_objects WHERE id = 1?"), &mapSelStmt, NULL);

	dbCode = sqlite3_bind_int64(mapSelStmt, 1, vecIds.pop_front());

	dbCode = sqlite3_step(mapSelStmt);

	if (dbCode != SQLITE_ROW)
	{
		sqlite3_reset(mapSelStmt);
	}

	do 
	{


	}(dbCode == SQLITE_ROW);

}