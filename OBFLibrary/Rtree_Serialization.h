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
			__int64 boxVal = (__int64)&n;
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
class saveRouteMessage
    : public rtree::visitor<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag, true>::type
{
public:
    typedef typename rtree::node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type node;
    typedef typename rtree::internal_node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;

    saveRouteMessage(BinaryMapDataWriter & archive, std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& bounds,OBFrouteDB& work,bool base)
        : m_archive(archive), m_bounds(bounds), m_work(work),  m_base(base)
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
		obf::OsmAndRoutingIndex_RouteDataBlock* dataBlock = nullptr;
		std::vector<__int64> wayMapIds;
		std::vector<__int64> pointMapIds;
		std::unordered_map<std::string, int> tempStringTable;
		std::unordered_map<MapRouteType, std::string, hashMapRoute, equalMapRoute> tempNames;
		__int64 ptrBox = (__int64)parentBox;
		std::unique_ptr<BinaryFileReference> ref = std::move(m_bounds[ptrBox]);
        for (typename elements_type::const_iterator it = elements.begin() ; it != elements.end() ; ++it)
        {
            __int64 callID = it->second.first;
			sqlite3_bind_int64(m_work.selectData, 1, callID);
			int dbResult = sqlite3_step(m_work.selectData);
			if (dbResult == SQLITE_ROW)
			{
				if (dataBlock == nullptr)
				{
					dataBlock = new obf::OsmAndRoutingIndex_RouteDataBlock;
					wayMapIds.clear();
					tempStringTable.clear();
					pointMapIds.clear();
				}
				int cid = m_work.registerID(wayMapIds, callID);

				const unsigned char* columnTextData = sqlite3_column_text(m_work.selectData, 5);
				if (columnTextData != nullptr && columnTextData[0] != 0)
				{
					std::string name(reinterpret_cast<const char*>(sqlite3_column_text(m_work.selectData, 5)));
					m_work.decodeNames(name, tempNames);
				}
				const void* plData = sqlite3_column_blob(m_work.selectData, 0);
				int bloblSize = sqlite3_column_bytes(m_work.selectData, 0);
				std::vector<int> typeUse(bloblSize / 2);
				for (int j = 0; j < bloblSize; j += 2) {
					int ids = parseSmallIntFromBytes(plData, j);
					typeUse[j / 2] = m_work.routingTypes.getTypeByInternalId(ids).getTargetId();
				}
				std::list<__int64> restIDs = m_work.highwayRestrictions[callID];
				if(!m_base && restIDs.size() != 0)
				{
					for(std::list<__int64>::iterator i = restIDs.begin(); i != restIDs.end(); i++)
					{
						obf::RestrictionData restData;
						restData.set_from(cid);
						int tid = m_work.registerID(wayMapIds, *i >> 3);
						restData.set_to(tid);
						restData.set_type(*i & 07);
						dataBlock->add_restrictions()->MergeFrom(restData);
					}
				}
				plData = sqlite3_column_blob(m_work.selectData, 3);
				bloblSize = sqlite3_column_bytes(m_work.selectData, 3);

				int pointList = bloblSize / 8;

				std::shared_ptr<OBFrouteDB::RouteMissingPoints> missPoints;
				if (m_base && m_work.basemapNodesToReinsert.find(callID) != m_work.basemapNodesToReinsert.end())
				{
					missPoints = m_work.basemapNodesToReinsert[callID];
					missPoints->buildPointsToInsert(pointList);
				}

				 std::vector<std::tuple<int, int, int, std::vector<int>>> TPpoints;
				 for (int ip = 0; ip < pointList; ip++)
				 {
					 if (missPoints)
					 {
						 auto data = missPoints->pointsXToInsert[ip];
						 auto pointMap = missPoints->pointsMap.size();
						 if (pointMap > 0 && data.get() != nullptr)
						 {
							 for(int k = 0; k < missPoints->pointsXToInsert[ip]->size(); k++) {

									std::tuple<int, int, int, std::vector<int>> TPpoint;
									std::get<1>(TPpoint) = data->operator[](k);
									std::get<2>(TPpoint) = missPoints->pointsYToInsert[ip]->operator[](k);
									TPpoints.push_back(TPpoint);
								}
						 }
					 }
					 std::tuple<int, int, int, std::vector<int>> pt;
					 std::get<1>(pt) = parseIntFromBytes(plData,ip * 8);
					 std::get<2>(pt) = parseIntFromBytes(plData, ip * 8 + 4);
					 // pointTypes
					plData = sqlite3_column_blob(m_work.selectData, 1);
					bloblSize = sqlite3_column_bytes(m_work.selectData, 1);
					int pointTypeVal = 0;
					if (bloblSize != 0) {
						for (int j = 0; j < bloblSize; j += 2) {
							pointTypeVal = parseSmallIntFromBytes(plData, j);
							if (pointTypeVal != 0)
							{
								std::get<3>(pt).push_back(m_work.routingTypes.getTypeByInternalId(pointTypeVal).getTargetId());
							}
							else
							{
								break;
							}
						}
					}
					TPpoints.push_back(pt);
				 }
				 				
					
				obf::RouteData routeData = m_archive.writeRouteData(cid, parentBox->min_corner().get<0>(), parentBox->min_corner().get<1>(), 
						typeUse, TPpoints, tempNames, tempStringTable, *dataBlock, true, false);
				if(routeData.IsInitialized()) {
					dataBlock->add_dataobjects()->MergeFrom(routeData);
				}

			}
			sqlite3_reset(m_work.selectData);
        }
		if (dataBlock != nullptr)
		{
			obf::IdTable idTable;
			__int64 prev = 0;
			for (int i = 0; i < wayMapIds.size(); i++) {
				idTable.add_routeid(wayMapIds[i] - prev);
				prev = wayMapIds[i];
			}
			dataBlock->mutable_idtable()->MergeFrom(idTable);
			m_archive.writeRouteDataBlock(*dataBlock, tempStringTable, *ref);
			delete dataBlock;
		}
    }

private:
    BinaryMapDataWriter & m_archive;
	Box* parentBox;
	OBFrouteDB& m_work;
	std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& m_bounds;
	bool m_base;
};

template <typename Value, typename Options, typename Translator, typename Box, typename Allocators>
class saveRouteMessageTree
    : public rtree::visitor<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag, true>::type
{
public:
    typedef typename rtree::node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type node;
    typedef typename rtree::internal_node<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type internal_node;
    typedef typename rtree::leaf<Value, typename Options::parameters_type, Box, Allocators, typename Options::node_tag>::type leaf;

    saveRouteMessageTree(BinaryMapDataWriter & archive, std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& bounds, Box* rootBounds,bool baseMap )
        : m_archive(archive), m_bounds(bounds), parentBox(rootBounds), isBaseMap(baseMap)
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
		std::unique_ptr<BinaryFileReference> ref = m_archive.startRouteTreeElement(parentBox->min_corner().get<0>(),parentBox->max_corner().get<0>(), parentBox->min_corner().get<1>(), parentBox->max_corner().get<1>(), isLeafNode, isBaseMap);
		if (ref)
		{
			__int64 boxVal = (__int64)&n;
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
		m_archive.endRouteTreeElement();
    }

    inline void operator()(leaf const& l)
    {
        typedef typename rtree::elements_type<leaf>::type elements_type;
        typedef typename elements_type::size_type elements_size;
        elements_type const& elements = rtree::elements(l);

        // change to elements_type::size_type or size_type?
        size_t s = elements.size();
        //m_archive << boost::serialization::make_nvp("s", s);
		std::unique_ptr<BinaryFileReference> ref = m_archive.startRouteTreeElement(parentBox->min_corner().get<0>(),parentBox->max_corner().get<0>(), parentBox->min_corner().get<1>(), parentBox->max_corner().get<1>(), true, 0);
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
		m_archive.endRouteTreeElement();
    }

private:
    BinaryMapDataWriter & m_archive;
	std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& m_bounds;
	Box* parentBox;
	bool isBaseMap;
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
    template <typename CodedInputMessage> inline static
    node_pointer apply(CodedInputMessage & msg, unsigned int version, size_type leafs_level, size_type & values_count, parameters_type const& parameters, Translator const& translator, Allocators & allocators)
    {
        values_count = 0;
        return raw_apply(msg, version, leafs_level, values_count, parameters, translator, allocators);
    }

private:
    template <typename CodedInputMessage> inline static
    node_pointer raw_apply(CodedInputMessage & cis, unsigned int version, size_type leafs_level, size_type & values_count, parameters_type const& parameters, Translator const& translator, Allocators & allocators, size_type current_level = 0)
    {
        //BOOST_GEOMETRY_INDEX_ASSERT(current_level <= leafs_level, "invalid parameter");

        // change to elements_type::size_type or size_type?
        size_t elements_count;
        //ar >> boost::serialization::make_nvp("s", elements_count);
		
        /*if ( elements_count < parameters.get_min_elements() || parameters.get_max_elements() < elements_count )
            BOOST_THROW_EXCEPTION(std::runtime_error("rtree loading error"));*/



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
void saveMapOBF(Archive & ar, boost::geometry::index::rtree<V, P, I, E, A> const& rt, std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& bounds,OBFMapDB& work, B* rootBounds, MapZooms::MapZoomPair level)
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
void saveRouteOBFTree(Archive & ar, boost::geometry::index::rtree<V, P, I, E, A> const& rt, std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& bounds,B* rootBounds, bool basemap)
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

		detail::rtree::visitors::saveRouteMessageTree<value_type, options_type, translator_type, box_type, allocators_type> save_vT(ar, bounds, rootBounds, basemap );
        detail::rtree::apply_visitor(save_vT, *tree.members().root);

    }
}

template<class Archive, typename V, typename P, typename I, typename E, typename A>
void saveRouteOBFData(Archive & ar, boost::geometry::index::rtree<V, P, I, E, A> const& rt, std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& bounds,OBFrouteDB& work,  bool base)
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

        detail::rtree::visitors::saveRouteMessage<value_type, options_type, translator_type, box_type, allocators_type> save_vM(ar, bounds, work, base);
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

	parameters_type params = rt.parameters();

    size_type values_count, leafs_level;
    

    node_pointer n(0);
    if ( /*0 < values_count*/ true )
    {
        size_type loaded_values_count = 0;
        n = detail::rtree::loadMessage<value_type, options_type, translator_type, box_type, allocators_type>
            ::apply(ar, version, leafs_level, loaded_values_count, params, tree.members().translator(), tree.members().allocators());                                        // MAY THROW

        node_auto_ptr remover(n, tree.members().allocators());
        //if ( loaded_values_count != values_count )
        //    BOOST_THROW_EXCEPTION(std::runtime_error("unexpected number of values")); // TODO change exception type
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
