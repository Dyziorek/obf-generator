#pragma once
#include <boost\geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>

#include <boost/geometry/index/rtree.hpp>


namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

class RTree
{
private:
	typedef bg::model::point<int, 2, bg::cs::cartesian> point;
    typedef bg::model::box<point> box;
    typedef std::pair<box, __int64> value;

	bgi::rtree<value, bgi::rstar<16>> spaceTree;

public:
	RTree(void);
	~RTree(void);

	void insertBox(int a, int b, int c, int d, __int64 id)
	{
		box boxPoint(point(a, b), point(c,d));
		spaceTree.insert(std::make_pair(boxPoint, id));
	}
};

