// Boost.Geometry Index
//
// Copyright (c) 2011-2013 Adam Wulkiewicz, Lodz, Poland.
//
// Use, modification and distribution is subject to the Boost Software License,
// Version 1.0. (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#ifndef OBFLIBRARY_RTREEE_SERIALIZATION
#define OBFLIBRARY_RTREEE_SERIALIZATION

//#include <boost/serialization/serialization.hpp>
//#include <boost/serialization/split_member.hpp>
//#include <boost/serialization/version.hpp>
//#include <boost/serialization/nvp.hpp>


// TODO - move to index/detail/rtree/visitors/save.hpp
namespace boost { namespace geometry { namespace index { namespace detail { namespace rtree { namespace visitors {

// TODO move saving and loading of the rtree outside the rtree, this will require adding some kind of members_view

template <typename Value, typename Options, typename Translator, typename Box, typename Allocators>
class saveMessageTree
    : public rtree::visitor<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag, true>::type
{
public:
    typedef typename rtree::node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type node;
    typedef typename rtree::internal_node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;

    saveMessageTree(BinaryMapDataWriter & archive, std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& bounds, Box* rootBounds )
        : m_archive(archive), m_bounds(bounds), parentBox(rootBounds)
    {}

    inline void operator()(internal_node const& n)
    {
        typedef typename rtree::elements_type<internal_node>::type elements_type;
        elements_type const& elements = rtree::elements(n);
		
        // change to elements_type::size_type or size_type?
        size_t s = elements.size();
		bool isLeafNode = false;
		for (typename elements_type::const_iterator it = elements.begin() ; it != elements.end() ; ++it)
        {
			boost::geometry::index::detail::utilities::dispatch::check_leaf(it->first, *it->second, isLeafNode);
			if (isLeafNode)
				break;
		}
		std::unique_ptr<BinaryFileReference> ref = m_archive.startMapTreeElement(parentBox->min_corner().get<0>(),parentBox->max_corner().get<0>(), parentBox->min_corner().get<1>(), parentBox->max_corner().get<1>(), isLeafNode, 0);
		if (ref)
		{
			__int64 boxVal = (__int64)parentBox;
			ref->l = parentBox->min_corner().get<0>();
			ref->r = parentBox->max_corner().get<0>();
			ref->t = parentBox->min_corner().get<1>();
			ref->b = parentBox->max_corner().get<1>();
			if (m_bounds.find(boxVal) == m_bounds.end())
			{
				m_bounds.insert(std::make_pair(boxVal, std::move(ref)));
			}
			else
			{
				m_bounds[boxVal].swap(ref);
			}
			
		}
        for (typename elements_type::const_iterator it = elements.begin() ; it != elements.end() ; ++it)
        {
            //serialization_save(it->first, "b", m_archive);
			this->parentBox = (Box*)&it->first;
            rtree::apply_visitor(*this, *it->second);
        }
		m_archive.endWriteMapTreeElement();
    }

    inline void operator()(leaf const& l)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        typedef typename elements_type::size_type elements_size;
        elements_type const& elements = rtree::elements(l);

        // change to elements_type::size_type or size_type?
        size_t s = elements.size();
        //m_archive << boost::serialization::make_nvp("s", s);
		std::unique_ptr<BinaryFileReference> ref = m_archive.startMapTreeElement(parentBox->min_corner().get<0>(),parentBox->max_corner().get<0>(), parentBox->min_corner().get<1>(), parentBox->max_corner().get<1>(), true, 0);
        //for (typename elements_type::const_iterator it = elements.begin() ; it != elements.end() ; ++it)
        //{
            //serialization_save(*it, "v", m_archive);
        //}
		if (ref)
		{
			__int64 boxVal = (__int64)parentBox;
			ref->l = parentBox->min_corner().get<0>();
			ref->r = parentBox->max_corner().get<0>();
			ref->t = parentBox->min_corner().get<1>();
			ref->b = parentBox->max_corner().get<1>();
			if (m_bounds.find(boxVal) == m_bounds.end())
			{
				m_bounds.insert(std::make_pair(boxVal, std::move(ref)));
			}
			else
			{
				m_bounds[boxVal].swap(ref);
			}
			
		}
		m_archive.endWriteMapTreeElement();
    }

private:
    BinaryMapDataWriter & m_archive;
	std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& m_bounds;
	Box* parentBox;
};

template <typename Value, typename Options, typename Translator, typename Box, typename Allocators>
class saveMessage
    : public rtree::visitor<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag, true>::type
{
public:
    typedef typename rtree::node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type node;
    typedef typename rtree::internal_node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;

    saveMessage(BinaryMapDataWriter & archive, std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& bounds,OBFMapDB& work, Box* rootBounds , MapZooms::MapZoomPair level)
        : m_archive(archive), m_bounds(bounds), m_work(work), parentBox(rootBounds), m_level(level)
    {}

    inline void operator()(internal_node const& n)
    {
        typedef typename rtree::elements_type<internal_node>::type elements_type;
        elements_type const& elements = rtree::elements(n);
		
        // change to elements_type::size_type or size_type?
        size_t s = elements.size();
		//bool isLeafNode;
		//for (typename elements_type::const_iterator it = elements.begin() ; it != elements.end() ; ++it)
  //      {
		//	boost::geometry::index::detail::utilities::dispatch::check_leaf(it->first, *it->second, isLeafNode);
		//}
		
        for (typename elements_type::const_iterator it = elements.begin() ; it != elements.end() ; ++it)
        {
            //serialization_save(it->first, "b", m_archive);
			this->parentBox = (Box*)&it->first;
            rtree::apply_visitor(*this, *it->second);
        }
		
    }

    inline void operator()(leaf const& l)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        typedef typename elements_type::size_type elements_size;
        elements_type const& elements = rtree::elements(l);

        // change to elements_type::size_type or size_type?
        size_t s = elements.size();
        //m_archive << boost::serialization::make_nvp("s", s);
		obf::MapDataBlock* dataBlock = nullptr;
		__int64 baseId = 0;
		std::unordered_map<std::string, int> tempStringTable;
		std::map<MapRulType, std::string> tempNames;
		__int64 ptrBox = (__int64)parentBox;
		std::unique_ptr<BinaryFileReference> ref = std::move(m_bounds[ptrBox]);
        for (typename elements_type::const_iterator it = elements.begin() ; it != elements.end() ; ++it)
        {
            __int64 callID = it->second.first;
			sqlite3_bind_int64(m_work.selectData, 1, callID);
			int dbResult = sqlite3_step(m_work.selectData);
			if (dbResult == SQLITE_ROW)
			{
				__int64 cid = m_work.convertGeneratedIdToObfWrite(callID);
				if (dataBlock == nullptr)
				{
					baseId = cid;
					dataBlock = m_archive.createWriteMapDataBlock(baseId);
				}
				tempNames.clear();
				const unsigned char* columnTextData = sqlite3_column_text(m_work.selectData, 5);
				if (columnTextData != nullptr && columnTextData[0] != 0)
				{
					std::string name(reinterpret_cast<const char*>(sqlite3_column_text(m_work.selectData, 5)));
					m_work.decodeNames(name, tempNames);
				}
				const void* plData = sqlite3_column_blob(m_work.selectData, 3);
				int bloblSize = sqlite3_column_bytes(m_work.selectData, 3);
				std::vector<int> typeUse(bloblSize / 2);
				for (int j = 0; j < bloblSize; j += 2) {
					int ids = parseSmallIntFromBytes(plData, j);
					typeUse[j / 2] = m_work.renderEncoder.getTypeByInternalId(ids).getTargetId();
				}
				plData = sqlite3_column_blob(m_work.selectData, 4);
				bloblSize = sqlite3_column_bytes(m_work.selectData, 4);
				std::vector<int> addtypeUse;
				if (bloblSize != 0) {
					addtypeUse.resize(bloblSize / 2);
					for (int j = 0; j < bloblSize; j += 2) {
						int ids = parseSmallIntFromBytes(plData, j);
						addtypeUse[j / 2] = m_work.renderEncoder.getTypeByInternalId(ids).getTargetId();
					}
				}
					
				obf::MapData mapData = m_archive.writeMapData(cid - baseId, parentBox->min_corner().get<0>(), parentBox->min_corner().get<1>(), m_work.selectData,
						typeUse, addtypeUse, tempNames, tempStringTable, dataBlock, m_level.getMaxZoom() > 15);
				if(mapData.IsInitialized()) {
					obf::MapData* mapDataToAdd = dataBlock->add_dataobjects();
					mapDataToAdd->MergeFrom(mapData);
				}

			}
			sqlite3_reset(m_work.selectData);
        }
		if (dataBlock != nullptr)
		{
			m_archive.writeMapDataBlock(dataBlock, tempStringTable, *ref);
		}
    }

private:
    BinaryMapDataWriter & m_archive;
	Box* parentBox;
	OBFMapDB& m_work;
	std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& m_bounds;
	MapZooms::MapZoomPair m_level;
};

}}}}}} // boost::geometry::index::detail::rtree::visitors

// TODO - move to index/detail/rtree/load.hpp
namespace boost { namespace geometry { namespace index { namespace detail { namespace rtree {

template <typename Value, typename Options, typename Translator, typename Box, typename Allocators>
class loadMessage
{
    typedef typename rtree::node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type node;
    typedef typename rtree::internal_node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;

    typedef typename Options::parameters_type parameters_type;

    typedef typename Allocators::node_pointer node_pointer;
    typedef rtree::node_auto_ptr<Value, Options, Translator, Box, Allocators> node_auto_ptr;
    typedef typename Allocators::size_type size_type;

public:
    template <typename Message> inline static
    node_pointer apply(Message & msg, unsigned int version, size_type leafs_level, size_type & values_count, parameters_type const& parameters, Translator const& translator, Allocators & allocators)
    {
        values_count = 0;
        return raw_apply(msg, version, leafs_level, values_count, parameters, translator, allocators);
    }

private:
    template <typename Message> inline static
    node_pointer raw_apply(Message & msg, unsigned int version, size_type leafs_level, size_type & values_count, parameters_type const& parameters, Translator const& translator, Allocators & allocators, size_type current_level = 0)
    {
        //BOOST_GEOMETRY_INDEX_ASSERT(current_level <= leafs_level, "invalid parameter");

        // change to elements_type::size_type or size_type?
        size_t elements_count;
        //ar >> boost::serialization::make_nvp("s", elements_count);

        if ( elements_count < parameters.get_min_elements() || parameters.get_max_elements() < elements_count )
            BOOST_THROW_EXCEPTION(std::runtime_error("rtree loading error"));

        if ( current_level < leafs_level )
        {
            node_pointer n = rtree::create_node<Allocators, internal_node>::apply(allocators);              // MAY THROW (A)
            node_auto_ptr auto_remover(n, allocators);    
            internal_node & in = rtree::get<internal_node>(*n);

            typedef typename rtree::elements_type<internal_node>::type elements_type;
            typedef typename elements_type::value_type element_type;
            typedef typename elements_type::size_type elements_size;
            elements_type & elements = rtree::elements(in);

            elements.reserve(parameters.get_max_elements());                                                               // MAY THROW (A)

            for ( size_t i = 0 ; i < elements_count ; ++i )
            {
                typedef typename elements_type::value_type::first_type box_type;
                
                node_pointer n = raw_apply(ar, version, leafs_level, values_count, parameters, translator, allocators, current_level+1); // recursive call
                elements.push_back(element_type(b, n));
            }

            auto_remover.release();
            return n;
        }
        else
        {
            BOOST_GEOMETRY_INDEX_ASSERT(current_level == leafs_level, "unexpected value");

            node_pointer n = rtree::create_node<Allocators, leaf>::apply(allocators);                       // MAY THROW (A)
            node_auto_ptr auto_remover(n, allocators);
            leaf & l = rtree::get<leaf>(*n);

            typedef typename rtree::elements_type<leaf>::type elements_type;
            typedef typename elements_type::value_type element_type;
            elements_type & elements = rtree::elements(l);

            values_count += elements_count;

            elements.reserve(elements_count);                                                               // MAY THROW (A)

            for ( size_t i = 0 ; i < elements_count ; ++i )
            {
                element_type el = serialization_load<element_type>("v", ar);                                     // MAY THROW (C)
                elements.push_back(el);                                                                     // MAY THROW (C)
            }

            auto_remover.release();
            return n;
        }
    }
};

}}}}} // boost::geometry::index::detail::rtree


// TODO - move to index/serialization/rtree.hpp
namespace boost { namespace serializationOBF {

template<class Archive, typename V, typename P, typename I, typename E, typename A, typename B>
void saveOBF(Archive & ar, boost::geometry::index::rtree<V, P, I, E, A> const& rt, std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& bounds,OBFMapDB& work, B* rootBounds, MapZooms::MapZoomPair level)
{
    namespace detail = boost::geometry::index::detail;

    typedef boost::geometry::index::rtree<V, P, I, E, A> rtree;
    typedef detail::rtree::const_private_view<rtree> view;
    typedef typename view::translator_type translator_type;
    typedef typename view::value_type value_type;
    typedef typename view::options_type options_type;
    typedef typename view::box_type box_type;
    typedef typename view::allocators_type allocators_type;

    view tree(rt);
	
	size_t valueTree = tree.members().values_count;
	size_t leafLevel = tree.members().leafs_level;
	
    if ( tree.members().values_count )
    {
        BOOST_GEOMETRY_INDEX_ASSERT(tree.members().root, "root shouldn't be null_ptr");

        detail::rtree::visitors::saveMessageTree<value_type, options_type, translator_type, box_type, allocators_type> save_vT(ar, bounds, rootBounds );
        detail::rtree::apply_visitor(save_vT, *tree.members().root);
        detail::rtree::visitors::saveMessage<value_type, options_type, translator_type, box_type, allocators_type> save_vM(ar, bounds, work, rootBounds, level);
        detail::rtree::apply_visitor(save_vM, *tree.members().root);

    }
}

template<class Archive, typename V, typename P, typename I, typename E, typename A, typename B>
void loadOBF(Archive & ar, boost::geometry::index::rtree<V, P, I, E, A> & rt, unsigned int version)
{
    namespace detail = boost::geometry::index::detail;

    typedef boost::geometry::index::rtree<V, P, I, E, A> rtree;
    typedef detail::rtree::private_view<rtree> view;
    typedef typename view::size_type size_type;
    typedef typename view::translator_type translator_type;
    typedef typename view::value_type value_type;
    typedef typename view::options_type options_type;
    typedef typename view::box_type box_type;
    typedef typename view::allocators_type allocators_type;

    typedef typename options_type::parameters_type parameters_type;
    typedef typename allocators_type::node_pointer node_pointer;
    typedef detail::rtree::node_auto_ptr<value_type, options_type, translator_type, box_type, allocators_type> node_auto_ptr;

    view tree(rt);

    parameters_type params = detail::serialization_load<parameters_type>("parameters", ar);

    size_type values_count, leafs_level;
    ar >> boost::serialization::make_nvp("values_count", values_count);
    ar >> boost::serialization::make_nvp("leafs_level", leafs_level);

    node_pointer n(0);
    if ( 0 < values_count )
    {
        size_type loaded_values_count = 0;
        n = detail::rtree::load<value_type, options_type, translator_type, box_type, allocators_type>
            ::apply(ar, version, leafs_level, loaded_values_count, params, tree.members().translator(), tree.members().allocators());                                        // MAY THROW

        node_auto_ptr remover(n, tree.members().allocators());
        if ( loaded_values_count != values_count )
            BOOST_THROW_EXCEPTION(std::runtime_error("unexpected number of values")); // TODO change exception type
        remover.release();
    }

    tree.members().parameters() = params;
    tree.members().values_count = values_count;
    tree.members().leafs_level = leafs_level;

    node_auto_ptr remover(tree.members().root, tree.members().allocators());
    tree.members().root = n;
}

template<class Archive, typename V, typename P, typename I, typename E, typename A> inline
void serialize(Archive & ar, boost::geometry::index::rtree<V, P, I, E, A> & rt, unsigned int version)
{
    split_free(ar, rt, version);
}

template<class Archive, typename V, typename P, typename I, typename E, typename A> inline
void serialize(Archive & ar, boost::geometry::index::rtree<V, P, I, E, A> const& rt, unsigned int version)
{
    split_free(ar, rt, version);
}

}} // boost::serializationOBF

#endif // OBFLIBRARY_RTREEE_SERIALIZATION
