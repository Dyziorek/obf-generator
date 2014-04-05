#include "stdafx.h"
#include "EntityNode.h"
#include "MapUtils.h"

EntityNode::EntityNode(void)
{
	lat = -1000;
	lon = -1000;
}


EntityNode::~EntityNode(void)
{
}


EntityRelation::EntityRelation(void)
{

}


EntityRelation::~EntityRelation(void)
{
}

EntityWay::EntityWay(void)
{

}


EntityWay::~EntityWay(void)
{
}

std::pair<double, double> EntityWay::getLatLon()
{
	return OsmMapUtils::getWeightCenterForNodes(nodes);
}
