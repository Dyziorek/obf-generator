#include "stdafx.h"
#include <boost\geometry.hpp>
#include "RTree.h"
//#include "ArchiveIO.h"



RTree::RTree(void)
{
}


RTree::~RTree(void)
{
}



void RTree::getTreeData(std::vector<__int64>& vecResults, std::tuple<double, double, double, double>& bounds)
{
	box boundaries = calculateBounds();

	double lowX = MapUtils::get31LongitudeX(boundaries.min_corner().get<0>());
	double lowY = MapUtils::get31LatitudeY(boundaries.min_corner().get<1>());

	double hiX = MapUtils::get31LongitudeX(boundaries.max_corner().get<0>());
	double hiY = MapUtils::get31LatitudeY(boundaries.max_corner().get<1>());

	std::get<0>(bounds) = lowX;
	std::get<1>(bounds) = lowY;
	std::get<2>(bounds) = hiX;
	std::get<3>(bounds) = hiY;

	std::vector<value> retVec;
	//std::vector<__int64> Results;
	spaceTree.query(bgi::intersects(boundaries), std::back_inserter(retVec));
	std::for_each(retVec.begin(), retVec.end(),[&vecResults](value result) { vecResults.push_back(result.second);});

}

	