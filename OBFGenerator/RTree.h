#pragma once
#include <boost\geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/adaptors/query.hpp>
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

	box calculateBounds()
	{
		if (spaceTree.size() == 0)
		{
			return box(point(0,0), point(0,0));
		}
		bgi::rtree<value, bgi::rstar<16>>::bounds_type boundary;
		return spaceTree.bounds();
	}

	std::vector<__int64> getAllFromBox(int a, int b, int c, int d)
	{
		box boxPoint(point(a, b), point(c,d));
		std::vector<value> retVec;
		spaceTree.query(bgi::intersects(boxPoint), std::back_inserter(retVec));
		std::vector<__int64> vecIds;
		std::for_each(retVec.begin(), retVec.end(),[&vecIds](value result) { vecIds.push_back(result.second);});
		return vecIds;
	}

	void paintTreeData(OBFResultDB& dbContext);
	
};

