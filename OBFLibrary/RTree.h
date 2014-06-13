#pragma once
#include <boost\geometry.hpp>
#include <boost/geometry/geometries/point.hpp>
#include <boost/geometry/geometries/box.hpp>
#include <boost/geometry/index/adaptors/query.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/index/detail/rtree/utilities/view.hpp>

namespace bg = boost::geometry;
namespace bgi = boost::geometry::index;

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
		if (visitorNodeData != nullptr)
		{
			visitorNodeData(elements.begin()->first, false, isLeafNode);
		}
		
        for (typename elements_type::const_iterator it = elements.begin();
            it != elements.end(); ++it)
        {
				
            rtree::apply_visitor(*this, *it->second);
        }
    }

    inline void operator()(leaf const& n)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        elements_type const& elements = rtree::elements(n);

        for (typename elements_type::const_iterator it = elements.begin();
            it != elements.end(); ++it)
        {
			if (visitorNodeData != nullptr)
			{
				visitorNodeData(tr(*it), true, true);
			}
			if (visitorLeafData != nullptr)
			{
				visitorLeafData(tr(*it), *it ,true, true);
			}
            //detail::utilities::gl_draw_indexable(tr(*it));
        }
    }

    Translator const& tr;
    size_t level_f;
    size_t level_l;
    typename coordinate_type<Box>::type z_mul;

    size_t level;
	bool isLeafNode;
	std::function<void(const Box&, bool, bool)> visitorNodeData;
	std::function<void(const Box&, const Value&, bool, bool)> visitorLeafData;
};

} } } } } 

class RTree
{
public:
	typedef bg::model::point<int, 2, bg::cs::cartesian> point;
    typedef bg::model::box<point> box;
    typedef boost::tuple<box, __int64, std::vector<short>> value;
	typedef bgi::rtree<value, bgi::rstar<32>> SI;

	std::list<boost::tuple<box, __int64, std::vector<short>>> initStore;
	
	SI spaceTree;

public:
	RTree(void);
	~RTree(void);

	void insertBox(int a, int b, int c, int d, __int64 id, std::list<long>& types)
	{
		box boxPoint(point(a, b), point(c,d));
		std::vector<short> typesMap;
		std::for_each(types.begin(), types.end(), [&typesMap](long data) { short smallData = data; typesMap.push_back(smallData);});
		initStore.push_back(boost::make_tuple(boxPoint, id, typesMap));
		//spaceTree.insert(boost::make_tuple(boxPoint, id, typesMap));
	}



	box calculateBounds()
	{
		if (spaceTree.size() == 0)
		{
			//return box(point(0,0), point(0,0));
			spaceTree = bgi::rtree<value, bgi::rstar<16>>(initStore.begin(), initStore.end());
			initStore.clear();
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
	void getTreeDataBox(std::vector<std::pair<__int64, std::vector<short>>>&vecRet, box& bounds, std::tuple<double, double, double, double>& newbounds);
	
	void getTreeNodes(std::function<void(const box&, bool, bool)> visitNodeData, std::function<void(const box&, const value&, bool, bool)> visitData);

};

