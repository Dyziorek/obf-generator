#pragma once
#include <boost\geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>


class MapObjectData
{

	typedef boost::geometry::model::point<int, 2, boost::geometry::cs::cartesian> pointI;
	typedef boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian> pointD;
	typedef boost::geometry::model::box<pointI> AreaI;
	typedef boost::geometry::model::box<pointD> AreaD;
	

	std::vector<pointI> points;

public:
	MapObjectData(void);
	~MapObjectData(void);
};


struct MapDecodingRule
{
    uint32_t type;

    std::string tag;
    std::string value;
};