#include "stdafx.h"
#include <boost\geometry.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include "RTree.h"
//#include "ArchiveIO.h"



RTree::RTree(void)
{
}


RTree::~RTree(void)
{
}



void RTree::getTreeData(std::vector<std::pair<__int64, std::vector<short>>>& vecResults, std::tuple<double, double, double, double>& bounds)
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
	vecResults.clear();
	std::for_each(retVec.begin(), retVec.end(),[&vecResults](value result) { vecResults.push_back(std::make_pair(result.get<1>(), result.get<2>()));});

	

}

void RTree::getTreeDataBox(std::vector<std::pair<__int64, std::vector<short>>>& vecResults, box& bounds, std::tuple<double, double, double, double>& newBounds)
{
	
	std::vector<value> retVec;
	vecResults.clear();
	//std::vector<__int64> Results;
	spaceTree.query(bgi::intersects(bounds), std::back_inserter(retVec));
	std::for_each(retVec.begin(), retVec.end(),[&vecResults](value result) { vecResults.push_back(std::make_pair(result.get<1>(), result.get<2>()));});

	box boundaries = bounds;

	double lowX = MapUtils::get31LongitudeX(boundaries.min_corner().get<0>());
	double lowY = MapUtils::get31LatitudeY(boundaries.min_corner().get<1>());

	double hiX = MapUtils::get31LongitudeX(boundaries.max_corner().get<0>());
	double hiY = MapUtils::get31LatitudeY(boundaries.max_corner().get<1>());

	std::get<0>(newBounds) = lowX;
	std::get<1>(newBounds) = lowY;
	std::get<2>(newBounds) = hiX;
	std::get<3>(newBounds) = hiY;

}

void RTree::getTreeNodes(std::function<int(void)> visitData)
{
	typedef bgi::detail::rtree::utilities::view<SI> RTV;
	RTV rtv(spaceTree);
	
	bgi::detail::utilities::leaf_node_view<typename RTV::value_type,
		typename RTV::options_type,
		typename RTV::translator_type,
		typename RTV::box_type, 
		typename RTV::allocators_type>
		visData(rtv.translator(), visitData);


	rtv.apply_visitor(visData);

}