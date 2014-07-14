#pragma once
#include <boost\geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>


struct MapObjectData
{
	typedef boost::geometry::model::point<int, 2, boost::geometry::cs::cartesian> pointI;
	typedef boost::geometry::model::box<pointI> boxI;
	typedef boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian> pointD;
	typedef boost::geometry::model::box<pointD> boxD;
	typedef boost::geometry::model::box<pointI> AreaI;
	typedef boost::geometry::model::box<pointD> AreaD;
	typedef boost::geometry::model::polygon<pointI> polyArea;
	typedef boost::geometry::model::polygon<pointI, true, false> polyLine;

	std::vector<pointI> points;
	std::list<std::vector<pointI>> innerpolypoints;
	int64_t localId;
	bool isArea;
	AreaI bbox;
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