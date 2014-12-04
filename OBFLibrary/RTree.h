#pragma once
#include <boost\geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/adaptors/query.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/index/detail/rtree/utilities/view.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;
namespace bgm = boost::geometry::model;

namespace boost { namespace geometry { namespace index { namespace detail {

namespace utilities {

namespace dispatch {
	
	template <typename Indexable, typename Valuable, typename Parametrable, typename Boxable, typename Allocable, typename TagName > inline
	void check_leaf(Indexable const& i, rtree::dynamic_node<typename Valuable, typename Parametrable, typename Boxable, typename Allocable, typename TagName>& listValues, bool& isLeaf)
	{
		typedef typename rtree::dynamic_leaf<Valuable, Parametrable, Boxable, Allocable, TagName> leafNode;
		leafNode* nodework = dynamic_cast<leafNode*>(&listValues);
		if (nodework != nullptr)
		{
			isLeaf = true;
		}
	};


}

//class ::RTree;

template <typename Value, typename Options, typename Translator, typename Box, typename Allocators>
struct leaf_node_view : public rtree::visitor<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag, true>::type
{
    typedef typename rtree::internal_node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;
	

    inline leaf_node_view(Translator const& t,
					std::function<void(const Box&, bool, bool)> visitNodeData, 
					std::function<void(const Box&, const Value&, bool, bool)> visitNodeLeafData )
        : tr(t)
		, visitorNodeData (visitNodeData)
		, visitorLeafData (visitNodeLeafData)
		, isLeafNode(false)
    {}

    inline void operator()(internal_node const& parent)
    {
        typedef typename rtree::elements_type<internal_node>::type elements_type;
        elements_type const& elements = rtree::elements(parent);


        for (typename elements_type::const_iterator it = elements.begin();
            it != elements.end(); ++it)
        {
			detail::utilities::dispatch::check_leaf(it->first, *it->second, isLeafNode);
        }
		for (typename elements_type::const_iterator it = elements.begin();
            it != elements.end(); ++it)
		{
			if (visitorNodeData != nullptr)
			{
				visitorNodeData(it->first, true, isLeafNode);
			}
			this->parentBox = (Box*)&it->first;
            rtree::apply_visitor(*this, *it->second);
			if (visitorNodeData != nullptr)
			{
				visitorNodeData(it->first, false, isLeafNode);
			}

		}
		
    }

    inline void operator()(leaf const& n)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        elements_type const& elements = rtree::elements(n);

    	if (visitorLeafData != nullptr)
		{
			visitorLeafData(*parentBox, *elements.begin() ,true, false);
		}

        for (typename elements_type::const_iterator it = elements.begin();
            it != elements.end(); ++it)
        {
    		if (visitorLeafData != nullptr)
			{
				visitorLeafData(*parentBox, *it ,true, true);
			}
            //detail::utilities::gl_draw_indexable(tr(*it));
        }
    	if (visitorLeafData != nullptr)
		{
			visitorLeafData(*parentBox, *elements.begin() ,false, false);
		}

    }

    Translator const& tr;
    size_t level_f;
    size_t level_l;
    typename coordinate_type<Box>::type z_mul;

    size_t level;
	bool isLeafNode;
	Box* parentBox;
	std::function<void(const Box&, bool, bool)> visitorNodeData;
	std::function<void(const Box&, const Value&, bool, bool)> visitorLeafData;
};

} } } } } 

template <typename Value>
class RTree
{
public:
	typedef bg::model::point<int, 2, bg::cs::cartesian> point;
    typedef bg::model::box<point> box;
    typedef std::pair<box, Value> value;
	typedef bgi::rtree<value, bgi::rstar<32>> SI;

	std::list<value> initStore;
	
	SI spaceTree;

public:
	RTree(void){};
	~RTree(void){};

	void insertBox(int a, int b, int c, int d, Value& types)
	{
		box boxPoint(point(a, b), point(c,d));
		initStore.push_back(std::make_pair(boxPoint, types));
		//spaceTree.insert(std::make_pair(boxPoint, types));
	}



	box calculateBounds()
	{
		if (spaceTree.size() == 0)
		{
			//return box(point(0,0), point(0,0));
			spaceTree = SI(initStore.begin(), initStore.end());
			initStore.clear();
		}
		return spaceTree.bounds();
	}

	std::vector<Value> getAllFromBox(int a, int b, int c, int d)
	{
		box boxPoint(point(a, b), point(c,d));
		std::vector<value> retVec;
		spaceTree.query(bgi::intersects(boxPoint), std::back_inserter(retVec));
		std::vector<Value> vecIds;
		std::for_each(retVec.begin(), retVec.end(),[&vecIds](value result) { vecIds.push_back(result.second);});
		return vecIds;
	}


	void getTreeData(std::vector<Value>& vecResults, std::tuple<double, double, double, double>& bounds)
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
		std::for_each(retVec.begin(), retVec.end(),[&vecResults](value result) { vecResults.push_back(result.second);});
	}


	void getTreeDataBox(std::vector<Value>& vecResults, box& bounds, std::tuple<double, double, double, double>& newBounds)
	{
	
		std::vector<value> retVec;
		vecResults.clear();
		//std::vector<__int64> Results;
		spaceTree.query(bgi::intersects(bounds), std::back_inserter(retVec));
		std::for_each(retVec.begin(), retVec.end(),[&vecResults](Value result) { vecResults.push_back(std::make_pair(result.get<1>(), result.get<2>()));});

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


	void getTreeNodes(std::function<void(const box&, bool, bool)> visitData, std::function<void(const box&, const value&, bool, bool)> visitLeafData)
	{
		typedef bgi::detail::rtree::utilities::view<SI> RTV;
		RTV rtv(spaceTree);
	
		bgi::detail::utilities::leaf_node_view<typename RTV::value_type,
			typename RTV::options_type,
			typename RTV::translator_type,
			typename RTV::box_type, 
			typename RTV::allocators_type>
			visData(rtv.translator(), visitData, visitLeafData);


		rtv.apply_visitor(visData);
	}

};

typedef bgm::point<__int64, 2, bg::cs::cartesian> pointL;
typedef bgm::point<int, 2, bg::cs::cartesian> pointI;
typedef bgm::box<pointI> boxI;
typedef bgm::box<pointL> boxL;
typedef bgm::point<double, 2, bg::cs::cartesian> pointD;
typedef bgm::point<float, 2, bg::cs::cartesian> pointF;
typedef bgm::box<pointD> boxD;
typedef boost::geometry::model::box<pointI> AreaI;
typedef boost::geometry::model::box<pointD> AreaD;
typedef boost::geometry::model::polygon<pointI> polyArea;
typedef boost::geometry::model::polygon<pointI, true, false> polyLine;

template <typename boxData> 
int	top(boxData data)
{
	return boxData.max_corner().get<1>();
};


struct areaInt : AreaI
{
	enum EdgeBox
	{
		invalid = -1,
		top = 1,
		bottom = 3,
		left = 0,
		right = 2
	};

	areaInt(AreaI& baseObj)
	{
		max_corner() = baseObj.max_corner();
		min_corner() = baseObj.min_corner();
	}

	int Top()
	{
		return max_corner().get<1>();
	}

	void Top(int inp)
	{
		max_corner().set<1>(inp);
	}


	int Left()
	{
		return max_corner().get<0>();
	}

	void Left(int inp)
	{
		max_corner().set<0>(inp);
	}

	int Bottom()
	{
		return min_corner().get<1>();
	}

	void Bottom(int inp)
	{
		min_corner().set<1>(inp);
	}
	int Right()
	{
		return min_corner().get<0>();
	}
	void Right(int inp)
	{
		min_corner().set<0>(inp);
	}

	EdgeBox onEdge(pointI ptCheck)
	{
		if (ptCheck.get<0>() == max_corner().get<0>())
		{
			return EdgeBox::left;
		}
		if (ptCheck.get<0>() == min_corner().get<0>())
		{
			return EdgeBox::right;
		}
		if (ptCheck.get<1>() == max_corner().get<1>())
		{
			return EdgeBox::top;
		}
		if (ptCheck.get<1>() == min_corner().get<1>())
		{
			return EdgeBox::bottom;
		}
		return EdgeBox::invalid;
	}
};