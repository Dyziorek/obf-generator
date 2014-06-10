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
	
	template <typename Indexable> inline
	void view_indexable(Indexable const& i, bool& isLeaf)
	{
		isLeaf = false;
	};
}
template <typename Value, typename Options, typename Translator, typename Box, typename Allocators>
struct leaf_node_view : public rtree::visitor<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag, true>::type
{
    typedef typename rtree::internal_node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;

    inline leaf_node_view(Translator const& t,
					std::function<int (void)> visitData,
                   size_t level_first = 0,
                   size_t level_last = (std::numeric_limits<size_t>::max)() )
        : tr(t)
        , level_f(level_first)
        , level_l(level_last)
        , level(0)
		, visitorData (visitData)
    {}

    inline void operator()(internal_node const& n)
    {
        typedef typename rtree::elements_type<internal_node>::type elements_type;
        elements_type const& elements = rtree::elements(n);


        if ( level_f <= level )
        {
			bool isLeaf = false;

            for (typename elements_type::const_iterator it = elements.begin();
                it != elements.end(); ++it)
            {
				detail::utilities::dispatch::view_indexable(it->first, isLeaf);
                //detail::utilities::gl_draw_indexable(it->first, level_rel);
				if (isLeaf)
				{
					visitorData();
				}
            }
        }
        
        size_t level_backup = level;
        ++level;

        if ( level < level_l )
        {
            for (typename elements_type::const_iterator it = elements.begin();
                it != elements.end(); ++it)
            {
				
                rtree::apply_visitor(*this, *it->second);
            }
        }

        level = level_backup;
    }

    inline void operator()(leaf const& n)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        elements_type const& elements = rtree::elements(n);

        if ( level_f <= level )
        {
            size_t level_rel = level - level_f;

            //glColor3f(0.25f, 0.25f, 0.25f);

            for (typename elements_type::const_iterator it = elements.begin();
                it != elements.end(); ++it)
            {
				visitorData();
                //detail::utilities::gl_draw_indexable(tr(*it));
            }
        }
    }

    Translator const& tr;
    size_t level_f;
    size_t level_l;
    typename coordinate_type<Box>::type z_mul;

    size_t level;
	std::function<int (void)> visitorData;
};

} } } } } 

class RTree
{
public:
	typedef bg::model::point<int, 2, bg::cs::cartesian> point;
    typedef bg::model::box<point> box;
    typedef boost::tuple<box, __int64, std::vector<short>> value;
	typedef bgi::rtree<value, bgi::rstar<16>> SI;

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
	
	void getTreeNodes(std::function<int(void)> visitData);

};

