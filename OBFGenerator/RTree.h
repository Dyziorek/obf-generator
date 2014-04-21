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
    typedef boost::tuple<box, __int64, std::vector<short>> value;

	bgi::rtree<value, bgi::rstar<16>> spaceTree;

public:
	RTree(void);
	~RTree(void);

	void insertBox(int a, int b, int c, int d, __int64 id, std::list<long>& types)
	{
		box boxPoint(point(a, b), point(c,d));
		std::vector<short> typesMap;
		std::for_each(types.begin(), types.end(), [&typesMap](long data) { short smallData = data; typesMap.push_back(smallData);});
		spaceTree.insert(boost::make_tuple(boxPoint, id, typesMap));
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
		std::for_each(retVec.begin(), retVec.end(),[&vecIds](value result) { vecIds.push_back(result.get<1>());});
		return vecIds;
	}

	void getTreeData(std::vector<std::pair<__int64, std::vector<short>>>&vecRet, std::tuple<double, double, double, double>& bounds);
	
};

