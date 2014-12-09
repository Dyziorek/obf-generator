#pragma once
#include <boost\geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

struct BinaryMapSection;

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
	std::vector<int> type;
	std::vector<int> addtype;
	std::list<std::tuple<int,int,std::string>> nameTypeString;
	std::weak_ptr<BinaryMapSection> section;
	bool containsTypeSlow( const std::string& tag, const std::string& value, bool checkAdditional = false ) const;
	bool containsType(const uint32_t typeRuleId, bool checkAdditional = false) const;
	int getSimpleLayerValue() const;
	bool isClosedFigure(bool checkInner = false) const;
#ifdef _DEBUG
	bool correctBBox;
	std::vector<pointD> geoPoints;
	AreaD geoBBox;
#endif
	MapObjectData(std::shared_ptr<BinaryMapSection> workSection);
	~MapObjectData(void);
};


struct MapDecodingRule
{
    uint32_t type;

    std::string tag;
    std::string value;
};