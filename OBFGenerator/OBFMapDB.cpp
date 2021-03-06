#include "stdafx.h"
#include "TileManager.h"
#include "EntityBase.h"
#include "EntityNode.h"
#include "MapObject.h"
#include "OBFRenderingTypes.h"
#include "OBFResultDB.h"
#include "MapUtils.h"
#pragma push_macro("realloc")
#undef realloc
#include "SkCanvas.h"
#include "SkSurface.h"
#include "SkImage.h"
#include "SkData.h"
#include "SkStream.h"
#include "SkGraphics.h"
#pragma pop_macro("realloc")
#include "RTree.h"

#include "MultiPoly.h"
#include <google\protobuf\io\coded_stream.h>
#include <google\protobuf\io\zero_copy_stream_impl_lite.h>
#include <google\protobuf\io\zero_copy_stream_impl.h>
#include <google\protobuf\wire_format_lite.h>
#include <boost\container\slist.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "..\..\..\..\core\protos\OBF.pb.h"
#include "OBFElementDB.h"
#include "RandomAccessFileWriter.h"
#include "BinaryMapDataWriter.h"
#include "OBFMapDB.h"
#include <boost/archive/binary_oarchive.hpp>
#include <boost/unordered_map.hpp>
#include <boost/container/list.hpp>
#include "boost/multi_array.hpp"
#include "ArchiveIO.h"
#include "Rtree_Serialization.h"

namespace io = boost::iostreams;
namespace ar = boost::archive;

long long OBFMapDB::notUsedId = - 1LL << 40; // million million
int OBFMapDB::numberCalls = 0;

int OBFMapDB::MAP_LEVELS_POWER = 3;
int OBFMapDB::MAP_LEVELS_MAX = 1 << OBFMapDB::MAP_LEVELS_POWER;

OBFMapDB::OBFMapDB(void)
{
	mapZooms = *MapZooms::getDefault();
	zoomWaySmothness = 2;
	mapTree.reserve(MAP_LEVELS_MAX);
	for (int i =0; i < mapZooms.size(); i++)
	{
		mapTree.push_back(RTreeValued());
	}
}


OBFMapDB::~OBFMapDB(void)
{
}

void OBFMapDB::excludeFromMainIteration(std::vector<std::shared_ptr<EntityWay>> l) {
		for(std::shared_ptr<EntityWay> w : l) {
			if(multiPolygonsWays.find(w->id) == multiPolygonsWays.end()) {
				multiPolygonsWays.insert(std::make_pair(w->id, std::vector<long>()));
			}
			multiPolygonsWays.at(w->id).insert(multiPolygonsWays.at(w->id).end(), typeUse.begin(), typeUse.end());
		}
	}

 
std::vector<std::shared_ptr<EntityNode>> OBFMapDB::simplifyCycleWay(std::vector<std::shared_ptr<EntityNode>> ns, int zoom, int zoomWaySmothness) 
{
		if (OsmMapUtils::checkForSmallAreas(ns, zoom + min(zoomWaySmothness / 2, 3), 2, 4)) {
			return std::vector<std::shared_ptr<EntityNode>>();
		}
		std::vector<std::shared_ptr<EntityNode>>  res;
		// simplification
		OsmMapUtils::simplifyDouglasPeucker(ns, zoom + 8 + zoomWaySmothness, 3, res, false);
		if (res.size() < 2) {
			return std::vector<std::shared_ptr<EntityNode>>();
		}
		return res;
	}

void OBFMapDB::indexMultiPolygon(std::shared_ptr<EntityRelation>& relItem, OBFResultDB& dbContext)
{
	if (!relItem)
		return;

	if (relItem->getTag("type") != "multipolygon" || relItem->getTag("admin_level") != "")
	{
		return;
	}

	renderEncoder.encodeEntityWithType(relItem, mapZooms.getLevel(0).getMaxZoom(), typeUse, addtypeUse, namesUse, tempNameUse);

	if (typeUse.size() == 0)
	{
		return;
	}
	std::shared_ptr<MultiPoly> polyline(new MultiPoly);
	dbContext.loadRelationMembers(relItem.get());
	dbContext.loadNodesOnRelation(relItem.get());
	polyline->createData(relItem);
	excludeFromMainIteration(polyline->inWays);
	excludeFromMainIteration(polyline->outWays);

	std::list<MultiPoly> polys = polyline->splitPerRing();
	for (MultiPoly m : polys) {
	
		auto out = m.outRing[0];
		if(out->nodes.size() == 0) {
				// don't index this
				continue;
			}

			// innerWays are new closed ways 
			std::vector<std::vector<std::shared_ptr<EntityNode>>> innerWays;

			for (std::shared_ptr<Ring> r : m.inRing) {
				innerWays.push_back(r->nodes);
			}

			// don't use the relation ids. Create new onesgetInnerRings
			long baseId = notUsedId --;
			for (int level = 0; level < mapZooms.size(); level++) {
				renderEncoder.encodeEntityWithType(relItem, mapZooms.getLevel(level).getMaxZoom(), typeUse, addtypeUse, namesUse,
						tempNameUse);
				if (typeUse.size() == 0) {
					continue;
				}
				long id = convertBaseIdToGeneratedId(baseId, level);
				// simplify route
				std::vector<std::shared_ptr<EntityNode>> outerWay = out->nodes;
				int zoomToSimplify = mapZooms.getLevel(level).getMaxZoom() - 1;
				if (zoomToSimplify < 15) {
					outerWay = simplifyCycleWay(outerWay, zoomToSimplify, zoomWaySmothness);
					if (outerWay.size() == 0) {
						continue;
					}
					std::vector<std::vector<std::shared_ptr<EntityNode>>> newinnerWays;
					for (std::vector<std::shared_ptr<EntityNode>> ls : innerWays) {
						ls = simplifyCycleWay(ls, zoomToSimplify, zoomWaySmothness);
						if (ls.size() != 0) {
							newinnerWays.push_back(ls);
						}
					}
					innerWays = newinnerWays;
				}
				std::list<std::shared_ptr<EntityNode>> outerWayList(outerWay.begin(), outerWay.end());
				insertBinaryMapRenderObjectIndex(mapTree[level], outerWayList, innerWays, namesUse, id, true, typeUse, addtypeUse, true, dbContext);

			}
		}
	
	//polyLines.push_back(polyline);
}

void OBFMapDB::indexMapAndPolygonRelations(std::shared_ptr<EntityRelation>& relItem, OBFResultDB& dbContext)
{
	indexMultiPolygon(relItem, dbContext);
	
	std::map<MapRulType, std::string> propogated = renderEncoder.getRelationPropogatedTags(*relItem);
	if(propogated.size() > 0) {
		dbContext.loadNodesOnRelation(relItem.get());
		for(std::pair<long long,  std::pair<int,std::string>> id : relItem->entityIDs ) {
			if(propagatedTags.find(id.first) == propagatedTags.end()) {
				propagatedTags.insert(std::make_pair(id.first, std::map<std::string, std::string>()));
			}
			std::map<std::string, std::string> map = propagatedTags.find(id.first)->second;
			std::map<MapRulType, std::string>::iterator it = propogated.begin();
			while(it != propogated.end()) {
				std::pair<MapRulType, std::string> es = *it;
				it++;
				std::string key = es.first.getTag();
				if(es.first.isText() && map.find(key) != map.end()) {
					map.insert(std::make_pair(key, map.find(key)->second + ", " +  es.second));
				} else {
					map.insert(std::make_pair(key, es.second));
				}
			}
		}
	}
		

}


void OBFMapDB::paintPolys()
{
	double scale;
	double offx, offy;
	double gscale = 0;
	double goffx = 0, goffy = 0;
	double goffmx = -1000, goffmy = -1000;
	SkRect limits =  SkRect::MakeWH(800, 600);
	std::list<std::shared_ptr<MultiPoly>>::iterator polit;
	for (polit = polyLines.begin(); polit != polyLines.end(); polit++)
	{
		(*polit)->getScaleOffsets(&scale, &offx, &offy, &goffmx, &goffmy, limits);
		goffx = min(goffx,offx);
		goffy = min(goffy,offy);
		gscale = min(gscale, scale);
	}

	SkImage::Info info = {
			1600, 1000, SkImage::kPMColor_ColorType, SkImage::kPremul_AlphaType
		};
		SkAutoTUnref<SkSurface> imageRender(SkSurface::NewRaster(info));
		SkCanvas* painter = imageRender->getCanvas();
		
		painter->drawColor(SK_ColorWHITE);
		
		SkPaint paint;
		paint.setColor(SK_ColorBLUE);
		paint.setStyle(SkPaint::Style::kStrokeAndFill_Style);
		painter->drawText("x",1,0,0,paint);

		for (polit = polyLines.begin(); polit != polyLines.end(); polit++)
		{
			(*polit)->paintImage(painter, gscale, goffx, goffy);
		}
		SkAutoTUnref<SkImage> image(imageRender->newImageSnapshot());
		SkAutoDataUnref data(image->encode());
		if (NULL == data.get()) {
			return ;
		}
		char buff[10];
		_ultoa_s(numberCalls++, buff,10);
		std::string pathImage = "D:\\osmData\\resultImageSingle" + std::string(buff) + std::string(".png");
		SkFILEWStream stream(pathImage.c_str());

		stream.write(data->data(), data->size());



}

void OBFMapDB::iterateMainEntity(std::shared_ptr<EntityBase>& baseItem, OBFResultDB& dbContext)
{
	std::shared_ptr<EntityNode> nodeItem = std::dynamic_pointer_cast<EntityNode, EntityBase>(baseItem);
	std::shared_ptr<EntityWay> wayItem = std::dynamic_pointer_cast<EntityWay, EntityBase>(baseItem);

	if (nodeItem || wayItem)
	{
		if (propagatedTags.find(baseItem->id) != propagatedTags.end())
		{
			std::map<std::string, std::string> proptags = propagatedTags.at(baseItem->id);
			for (auto tagPair : proptags)
			{
				if (baseItem->getTag(tagPair.first) == "")
				{
					baseItem->putTag(tagPair.first, tagPair.second);
				}
			}
		}
		std::list<std::unordered_map<std::string, std::string>>  splitter = renderEncoder.splitTagsIntoDifferentObjects(baseItem->tags);
		if (splitter.size() > 1)
		{
			auto latLon = baseItem->getLatLon();
				bool first = true;
				for(auto inst : splitter) {
					if(first) {
						baseItem->tags = inst;
						first = false;
						iterateMainEntityPost(baseItem, dbContext);
					} else {
						std::shared_ptr<EntityNode> ns(new EntityNode(latLon.first, latLon.second, notUsedId--));
						ns->tags = inst;
						iterateMainEntityPost(std::dynamic_pointer_cast<EntityBase, EntityNode>(ns), dbContext);
					}
				}
		}
		else
		{
			iterateMainEntityPost(baseItem, dbContext);
		}
	}

}

void OBFMapDB::iterateMainEntityPost(std::shared_ptr<EntityBase>& e, OBFResultDB& dbContext) 
{
	std::shared_ptr<EntityWay> wayItem = std::dynamic_pointer_cast<EntityWay, EntityBase>(e);
	std::shared_ptr<EntityNode> nodeItem = std::dynamic_pointer_cast<EntityNode, EntityBase>(e);
	
		for (int level = 0; level < mapZooms.size(); level++) {
			bool instNode = (nodeItem);
			bool area = renderEncoder.encodeEntityWithType(instNode, 
				e->tags, mapZooms.getLevel(level).getMaxZoom(), typeUse, addtypeUse, namesUse,
					tempNameUse);
			if (typeUse.empty()) {
				continue;
			}
			bool hasMulti = (wayItem) && multiPolygonsWays.find(e->id) != multiPolygonsWays.end();
			if (hasMulti) {
				auto set = multiPolygonsWays.at(e->id);
				std::for_each(set.begin(), set.end(), [&](long iterVec){
					typeUse.remove(iterVec);
				});
			}
			if (typeUse.empty()) {
				continue;
			}
			long id = convertBaseIdToGeneratedId(e->id, level);
			std::list<std::shared_ptr<EntityNode>> res;
			if (nodeItem) {
				res.push_back(nodeItem);
			} else {
				id |= 1;

				// simplify route id>>1
				bool mostDetailedLevel = level == 0;
				if (!mostDetailedLevel) {
					int zoomToSimplify = mapZooms.getLevel(level).getMaxZoom() - 1;
					bool cycle = wayItem->getFirstNodeId() == wayItem->getLastNodeId();
					if (cycle) {
						res = OsmMapUtils::simplifyCycleWay(wayItem->nodes, zoomToSimplify, zoomWaySmothness);
					} else {
						std::string ename = "";
						if (namesUse.find(*renderEncoder.nameRule) != namesUse.end())
						{
							ename = namesUse.at(*renderEncoder.nameRule);
						}
						insertLowLevelMapBinaryObject(level, zoomToSimplify, typeUse, addtypeUse, id, wayItem->nodes, ename, dbContext);
					}
				} else {
					res = wayItem->getListNodes();
				}
			}
			if (!res.empty()) {
				insertBinaryMapRenderObjectIndex(mapTree[level], res, std::vector<std::vector<std::shared_ptr<EntityNode>>>(), namesUse, id, area, typeUse, addtypeUse, true, dbContext);
			}
		}
	}



void OBFMapDB::insertLowLevelMapBinaryObject(int level, int zoom, std::list<long> types, std::list<long> addTypes, __int64 id, std::vector<std::shared_ptr<EntityNode>> in, std::string name,OBFResultDB& dbContext)
{
		
		std::vector<std::shared_ptr<EntityNode>> nodes;
		OsmMapUtils::simplifyDouglasPeucker(in, zoom + 8 + zoomWaySmothness, 3, nodes, false);
		bool first = true;
		__int64 firstId = -1;
		__int64 lastId = -1;
		
		typedef io::basic_array<std::string> arrData;
		typedef io::stream<arrData> arrStrm;

		std::string bar;

		//arrStrm bNodes(bar);

		std::stringstream bNodeData;
		std::stringstream bTypesData;
		std::stringstream bAddtTypesData;

		//portable_binary_oarchive bNodes(bNodeData, boost::archive::no_header);
		//portable_binary_oarchive bTypes(bTypesData, boost::archive::no_header);
		//portable_binary_oarchive bAddtTypes(bAddtTypesData, boost::archive::no_header);
		//arrStrm bTypes(std::vector<byte>());
		//arrStrm bAddtTypes(std::vector<byte>());

		//io::array_sink bNodes(std::vector<BYTE>()); //bNodes = new ByteArrayOutputStream();
		//io::array_sink bTypes(std::vector<BYTE>());// = new ByteArrayOutputStream();
		//io::array_sink bAddtTypes(std::vector<BYTE>());// = new ByteArrayOutputStream();
		try {
			for (std::shared_ptr<EntityNode> n : nodes) {
				if (n) {
					if (first) {
						firstId = n->id;
						first = false;
					}
					lastId = n->id;
					
					writeInt(bNodeData, (float)n->lat);
					writeInt(bNodeData, (float)n->lon);
					

					//Algorithms.writeInt(bNodes, Float.floatToRawIntBits((float) n.getLatitude()));
					//Algorithms.writeInt(bNodes, Float.floatToRawIntBits((float) n.getLongitude()));
				}
			}
		} catch (std::exception e) {
			
		}
		if (firstId == -1) {
			return;
		}
		for (int j = 0; j < types.size(); j++) {
			try {
				std::list<long>::iterator typesIt = types.begin();
				std::advance(typesIt, j);
				writeSmallInt(bTypesData, *typesIt);
			} catch (std::exception e) {
			}
		}
		for (int j = 0; j < addTypes.size(); j++) {
			try {
				std::list<long>::iterator typesIt = addTypes.begin();
				std::advance(typesIt, j);
				writeSmallInt(bAddtTypesData, *typesIt);
			} catch (std::exception e) {
			}
		}
		
		
		dbContext.addBatch(id, firstId, lastId, name, bNodeData, bTypesData, bAddtTypesData, level);

	}

bool  OBFMapDB::insertBinaryMapRenderObjectIndex(RTreeValued& mapTree, std::list<std::shared_ptr<EntityNode>>& nodes, std::vector<std::vector<std::shared_ptr<EntityNode>>>& innerWays,
			std::map<MapRulType, std::string>& names, __int64 id, bool area, std::list<long>& types, std::list<long>& addTypes, bool commit, OBFResultDB& dbContext)
			{
		bool init = false;
		int minX = INT_MAX;
		int maxX = 0;
		int minY = INT_MAX;
		int maxY = 0;

		std::stringstream bCoordData;
		std::stringstream bInCoordData;
		std::stringstream bTypesData;
		std::stringstream bAddtTypesData;

		//portable_binary_oarchive bcoordinates(bCoordData, boost::archive::no_header);
		//portable_binary_oarchive binnercoord(bInCoordData, boost::archive::no_header);
		//portable_binary_oarchive btypes(bTypesData, boost::archive::no_header);
		//portable_binary_oarchive badditionalTypes(bAddtTypesData, boost::archive::no_header);

		/*		 bcoordinates = new ByteArrayOutputStream();
		ByteArrayOutputStream binnercoord = new ByteArrayOutputStream();
		ByteArrayOutputStream btypes = new ByteArrayOutputStream();
		ByteArrayOutputStream badditionalTypes = new ByteArrayOutputStream();

		*/

			for (int j = 0; j < types.size(); j++) {
				std::list<long>::iterator typesIt = types.begin();
				std::advance(typesIt, j);
				writeSmallInt(bTypesData, *typesIt);
			}
			for (int j = 0; j < addTypes.size(); j++) 
			{
				std::list<long>::iterator atypesIt = addTypes.begin();
				std::advance(atypesIt, j);
				writeSmallInt(bAddtTypesData, *atypesIt);
			}

			for (std::shared_ptr<EntityNode> n : nodes) {
				if (n) {
					int y = MapUtils::get31TileNumberY(n->lat);
					int x = MapUtils::get31TileNumberX(n->lon);
					minX = min(minX, x);
					maxX = max(maxX, x);
					minY = min(minY, y);
					maxY = max(maxY, y);
					init = true;
					writeInt(bCoordData, x);
					writeInt(bCoordData, y);
				}
			}

			if (innerWays.size()) {
				for (std::vector<std::shared_ptr<EntityNode>> ws : innerWays) {
					bool exist = false;
					if (ws.size()) {
						for (std::shared_ptr<EntityNode> n : ws) {
							if (n) {
								exist = true;
								int y = MapUtils::get31TileNumberY(n->lat);
								int x = MapUtils::get31TileNumberX(n->lon);
								writeInt(bInCoordData, x);
								writeInt(bInCoordData, y);
							}
						}
					}
					if (exist) {
						writeInt(bInCoordData, 0);
						writeInt(bInCoordData, 0);
					}
				}
			}
		
			if (init)
			{
				dbContext.addBatch(id, area, bCoordData, bInCoordData, bTypesData, bAddtTypesData, encodeNames(names));
				std::vector<short> shorted;
				std::for_each(types.begin(), types.end(), [&shorted](long typeVal) {shorted.push_back((short)typeVal); });
				mapTree.insertBox(minX, minY, maxX, maxY, std::make_pair(id, shorted));
			}
		/*
		if (init) {
			// conn.prepareStatement("insert into binary_map_objects(id, area, coordinates, innerPolygons, types, additionalTypes, name) values(?, ?, ?, ?, ?, ?, ?)");
			mapBinaryStat.setLong(1, id);
			mapBinaryStat.setBoolean(2, area);
			mapBinaryStat.setBytes(3, bcoordinates.toByteArray());
			mapBinaryStat.setBytes(4, binnercoord.toByteArray());
			mapBinaryStat.setBytes(5, btypes.toByteArray());
			mapBinaryStat.setBytes(6, badditionalTypes.toByteArray());
			mapBinaryStat.setString(7, encodeNames(names));

			addBatch(mapBinaryStat, commit);
			try {
				mapTree.insert(new LeafElement(new Rect(minX, minY, maxX, maxY), id));
			} catch (RTreeInsertException e1) {
				throw new IllegalArgumentException(e1);
			} catch (IllegalValueException e1) {
				throw new IllegalArgumentException(e1);
			}
		}
		*/
			return init;
	}

	std::string OBFMapDB::encodeNames(std::map<MapRulType, std::string> tempNames) {
		std::stringstream strm;
		for (std::pair<MapRulType, std::string> e : tempNames) {
			if (e.second.size()) {
				strm << "~";
				strm << e.first.getInternalId();
				strm << "~";
				strm << e.second;
			}
		}
		return  strm.str();
	}
	
	void OBFMapDB::decodeNames(std::string name, std::map<MapRulType, std::string>& tempNames)
	{
		int i = name.find("~");
		while (i != name.npos) {
			int n = name.find("~", i + 1);
			std::string ch = name.substr(i + 1, n-(i+1));
			int n2 = name.find("~", n + 1);
			MapRulType rt = renderEncoder.getTypeByInternalId(boost::lexical_cast<int>(ch));
			if (n2 == -1) {
				tempNames.insert(std::make_pair(rt, name.substr(n+1)));
			} else {
				tempNames.insert(std::make_pair(rt, name.substr(n+1, n2-(n+1))));
			}
			i = n2;
		}
	}

	void OBFMapDB::processLowLevelWays(OBFResultDB& dbContext)
	{
		sqlite3* dbCtx = dbContext.dbMapCtx;
		sqlite3_stmt* lowLevelWayItStart;
		int dbCode = SQLITE_OK;
		char* errMsg;
		dbCode = sqlite3_prepare_v2(dbCtx, "SELECT id, start_node, end_node, nodes, name, type, addType FROM low_level_map_objects WHERE start_node = ?1 AND level = ?2" , sizeof("SELECT id, start_node, end_node, nodes, name, type, addType FROM low_level_map_objects WHERE start_node = ?1 AND level = ?2"), &lowLevelWayItStart, NULL);
		sqlite3_stmt* lowLevelWayItEnd;
		dbCode = sqlite3_prepare_v2(dbCtx, "SELECT id, start_node, end_node, nodes, name, type, addType FROM low_level_map_objects WHERE end_node = ?1 AND level = ?2", sizeof("SELECT id, start_node, end_node, nodes, name, type, addType FROM low_level_map_objects WHERE end_node = ?1 AND level = ?2"), &lowLevelWayItEnd, NULL);
		sqlite3_stmt* lowLevelWayIt;
		dbCode = sqlite3_prepare_v2(dbCtx, "SELECT id, start_node, end_node, nodes, name, type, addType, level FROM low_level_map_objects", sizeof("SELECT id, start_node, end_node, nodes, name, type, addType, level FROM low_level_map_objects"), &lowLevelWayIt, NULL);


		int counterInvoked = 0, counterCopied = 0;

		dbCode = sqlite3_step(lowLevelWayIt);

		std::set<__int64> visited;


		if (dbCode != SQLITE_ROW)
		{
			sqlite3_reset(lowLevelWayIt);
		}

		do 
		{
			__int64 id = sqlite3_column_int64(lowLevelWayIt, 0);
			if (visited.find(id) != visited.end())
			{
				dbCode = sqlite3_step(lowLevelWayIt);
				continue;
			}
			visited.insert(id);

			__int64 nodeStart = sqlite3_column_int64(lowLevelWayIt, 1);
			__int64 nodeEnd = sqlite3_column_int64(lowLevelWayIt, 2);
			const void* pData = sqlite3_column_blob(lowLevelWayIt, 3);
			int blobSize = sqlite3_column_bytes(lowLevelWayIt, 3);
			
			std::string name((const char*)(sqlite3_column_text(lowLevelWayIt, 4)));
			int level = sqlite3_column_int(lowLevelWayIt, 7);
			
			int maxZoom = mapZooms.getLevel(level).getMaxZoom();

			parseAndSort(sqlite3_column_blob(lowLevelWayIt, 5), sqlite3_column_bytes(lowLevelWayIt, 5), typeUse);
			parseAndSort(sqlite3_column_blob(lowLevelWayIt, 6), sqlite3_column_bytes(lowLevelWayIt, 6), addtypeUse);

			std::stringstream nodeStream;
			nodeStream.rdbuf()->sputn((const char*)pData, blobSize);
			std::vector<float> nodeData;
			readInts(nodeStream, nodeData);

			std::list<long> temp;
			std::list<long> tempAdd;

			bool combineWay = true;
			while (combineWay)
			{
				combineWay = false;
				dbCode = sqlite3_bind_int64(lowLevelWayItEnd, 1, nodeStart);
				dbCode = sqlite3_bind_int(lowLevelWayItEnd, 2, level);
				dbCode = sqlite3_step(lowLevelWayItEnd);
				if (dbCode != SQLITE_ROW && !combineWay)
				{
					sqlite3_reset(lowLevelWayItEnd);
				}
				else
				{
					do 
					{
						if (visited.find(sqlite3_column_int64(lowLevelWayItEnd, 0)) == visited.end())
						{
							parseAndSort(sqlite3_column_blob(lowLevelWayItEnd, 5), sqlite3_column_bytes(lowLevelWayItEnd, 5), temp);
							parseAndSort(sqlite3_column_blob(lowLevelWayItEnd, 6), sqlite3_column_bytes(lowLevelWayItEnd, 6), tempAdd);
							if ((temp.size() == typeUse.size() && std::equal(temp.begin(), temp.end(), typeUse.begin())) &&
								(tempAdd.size() == addtypeUse.size() && std::equal(tempAdd.begin(), tempAdd.end(), addtypeUse.begin())))
							{
								combineWay = true;
								__int64 lid = sqlite3_column_int64(lowLevelWayItEnd, 0);
								nodeStart = sqlite3_column_int64(lowLevelWayItEnd, 1);
								visited.insert(lid);
								const void* plData = sqlite3_column_blob(lowLevelWayItEnd, 3);
								int bloblSize = sqlite3_column_bytes(lowLevelWayItEnd, 3);
								std::stringstream nodelStream;
								nodelStream.rdbuf()->sputn((const char*)plData, bloblSize);
								std::vector<float> linodeData;
								readInts(nodelStream, linodeData);
								std::string iname(reinterpret_cast<const char*>(sqlite3_column_text(lowLevelWayItEnd, 4)));
								if(name != iname){
									name = "";
								}
							
								// remove first lat/lon point
								nodeData.erase(nodeData.begin());
								nodeData.erase(nodeData.begin());
								linodeData.insert(linodeData.end(), nodeData.begin(), nodeData.end());
								nodeData = linodeData;
							}
						}
						dbCode = sqlite3_step(lowLevelWayItEnd);
					}
					while(dbCode == SQLITE_ROW && !combineWay);
				}
				sqlite3_reset(lowLevelWayItEnd);
			}

			combineWay = true;

			while (combineWay)
			{
				combineWay = false;
				dbCode = sqlite3_bind_int64(lowLevelWayItStart, 1, nodeEnd);
				dbCode = sqlite3_bind_int(lowLevelWayItStart, 2, level);
				dbCode = sqlite3_step(lowLevelWayItStart);
				if (dbCode != SQLITE_ROW)
				{
					sqlite3_reset(lowLevelWayItStart);
				}
				else
				{
					do 
					{
						if (visited.find(sqlite3_column_int64(lowLevelWayItStart, 0)) == visited.end())
						{
							parseAndSort(sqlite3_column_blob(lowLevelWayItStart, 5), sqlite3_column_bytes(lowLevelWayItStart, 5), temp);
							parseAndSort(sqlite3_column_blob(lowLevelWayItStart, 6), sqlite3_column_bytes(lowLevelWayItStart, 6), tempAdd);
							if ((temp.size() == typeUse.size() && std::equal(temp.begin(), temp.end(), typeUse.begin())) &&
								(tempAdd.size() == addtypeUse.size() && std::equal(tempAdd.begin(), tempAdd.end(), addtypeUse.begin())))
							{
								combineWay = true;
								__int64 lid = sqlite3_column_int64(lowLevelWayItStart, 0);
								nodeEnd = sqlite3_column_int64(lowLevelWayItStart, 2);
								visited.insert(lid);
								const void* plData = sqlite3_column_blob(lowLevelWayItStart, 3);
								int bloblSize = sqlite3_column_bytes(lowLevelWayItStart, 3);
								std::stringstream nodelStream;
								nodelStream.rdbuf()->sputn((const char*)plData, bloblSize);
								std::vector<float> linodeData;
								readInts(nodelStream, linodeData);
								std::string iname(reinterpret_cast<const char*>(sqlite3_column_text(lowLevelWayItStart, 4)));
								if(name != iname){
									name = "";
								}
								auto itBegin = linodeData.begin();
								itBegin++;
								itBegin++;
								nodeData.insert(nodeData.end(), itBegin, linodeData.end());
							}
						}
						dbCode = sqlite3_step(lowLevelWayItStart);
					}
					while(dbCode == SQLITE_ROW && !combineWay);
				}
				sqlite3_reset(lowLevelWayItStart);
			}
			
			std::vector<std::shared_ptr<EntityNode>> wnodeList;

			for (int i=0; i < nodeData.size(); i+=2)
			{
				wnodeList.push_back(std::unique_ptr<EntityNode>(new EntityNode(nodeData[i], nodeData[i+1], i==0?nodeStart:nodeEnd)));
			}
			bool skip = false;
			bool cycle = nodeStart == nodeEnd;
			if (cycle) {
				skip = checkForSmallAreas(wnodeList, maxZoom  + min(zoomWaySmothness / 2, 3), 3, 4);
			} else {
				// coastline
				if(std::find(typeUse.begin(),typeUse.end(),(renderEncoder.coastlineRule->getInternalId())) == typeUse.end()) {
					skip = checkForSmallAreas(wnodeList, maxZoom  + min(zoomWaySmothness / 2, 3), 2, 8);
				}
			}
			if (!skip) {
				std::vector<std::shared_ptr<EntityNode>> res;
				OsmMapUtils::simplifyDouglasPeucker(wnodeList, maxZoom - 1 + 8 + zoomWaySmothness, 3, res, false);
				if (res.size() > 0) {
					namesUse.clear();
					if (name != "") {
						namesUse.insert(std::make_pair(*renderEncoder.nameRule, name));
					}
					std::list<std::shared_ptr<EntityNode>> resList(res.begin(), res.end());
					if (insertBinaryMapRenderObjectIndex(mapTree[level], resList, std::vector<std::vector<std::shared_ptr<EntityNode>>>(), namesUse, id, false, typeUse, addtypeUse, false, dbContext))
					{
						counterCopied++;
					}
					counterInvoked++;
				}
			}
			dbCode = sqlite3_step(lowLevelWayIt);
		}
		while (dbCode == SQLITE_ROW);
#ifdef _DEBUG
		CString strFormat;
		strFormat.Format(L"Low level objects processed into binary objects: %d, copied: %d", counterInvoked, counterCopied);
		OutputDebugString(strFormat.GetBuffer());
#endif
	}


	void OBFMapDB::parseAndSort(const void* blobData, int blobSize, std::list<long>& toData)
	{
		toData.clear();
		std::stringstream dataStream;
		dataStream.rdbuf()->sputn((const char*)blobData, blobSize);
		std::vector<long> localLoad;
		readSmallInts(dataStream, localLoad);
		if(localLoad.size() > 0)
		{
			//boost::container::list<long> arrData;
			//std::for_each(localLoad.begin(), localLoad.end(), [&arrData](long value) {arrData.push_back(value);});
			//arrData.sort();
			std::for_each(localLoad.begin(), localLoad.end(), [&toData](long value) {toData.push_back(value);});
			toData.sort();
		}
	}

	bool OBFMapDB::checkForSmallAreas(std::vector<std::shared_ptr<EntityNode>> nodes, int zoom, int minz, int maxz) {
		int minX = INT_MAX;
		int maxX = INT_MIN;
		int minY = INT_MAX;
		int maxY = INT_MIN;
		int c = 0;
		int nsize = nodes.size();
		for (auto i = nodes.begin(); i != nodes.end(); i++) {
			if (*i) {
				c++;
				int x = (int) (MapUtils::getTileNumberX(zoom, (*i)->lon) * 256.0);
				int y = (int) (MapUtils::getTileNumberY(zoom, (*i)->lat) * 256.0);
				minX = min(minX, x);
				maxX = max(maxX, x);
				minY = min(minY, y);
				maxY = max(maxY, y);
			}
		}
		if (c < 2) {
			return true;
		}
		return ((maxX - minX) <= minz && (maxY - minY) <= maxz) || ((maxX - minX) <= maxz && (maxY - minY) <= minz);

	}

typedef std::tuple<__int64, int> tpl;

bool operator==(tpl const& op1 , tpl const& op2)
{
	return std::get<0>(op1) == std::get<0>(op2) && std::get<1>(op1) == std::get<1>(op2);
}

struct tpl_hash
    : std::unary_function<tpl, std::size_t>
{
	std::size_t operator()(tpl const& tplVal) const
	{
		std::size_t seed = 0;
		boost::hash_combine(seed,std::get<0>(tplVal));
		boost::hash_combine(seed,std::get<1>(tplVal));
		return seed;
	}
};

void OBFMapDB::paintTreeData(OBFResultDB& dbContext, std::set<std::shared_ptr<MultiPoly>>& boundsLines, std::map<std::shared_ptr<EntityNode>, CityObj>& cities)
{

	std::vector<std::pair<__int64, std::vector<short>>> treeIds;
	std::vector<std::pair<__int64, std::vector<short>>> treeSearchIds;
	 std::tuple<double, double, double, double> bounds;
	 
	 int dbCode;
	sqlite3* dbCtx = dbContext.dbMapCtx;
	sqlite3_stmt* mapSelStmt;
	dbCode = sqlite3_prepare_v2(dbCtx, "SELECT id, area, coordinates, innerPolygons, types, additionalTypes, name FROM binary_map_objects WHERE id = ?1" , sizeof("SELECT id, area, coordinates, innerPolygons, types, additionalTypes, name FROM binary_map_objects WHERE id = ?1"), &mapSelStmt, NULL);


	
	std::unordered_map<tpl, std::shared_ptr<std::vector<std::pair<float, float>>>, tpl_hash> lines;
	std::unordered_map<tpl, std::shared_ptr<std::vector<std::pair<float, float>>>, tpl_hash> inlines;
	 
	for (int indexMapTree = mapTree.size()-1; indexMapTree >= 0; indexMapTree--)
	{
		mapTree[indexMapTree].getTreeData(treeIds, bounds);
		for (std::pair<__int64, std::vector<short>> iidPair : treeIds) 
		{
			__int64 iid = iidPair.first;

			std::vector<short> typesUse = iidPair.second;

			bool useData = false;
			if (typesUse.size() > 0)
			{
				short mainType = typesUse[0];
				if (renderEncoder.rules.size() > mainType)
				{
					MapRulType ruleData = renderEncoder.rules[mainType];
					if (ruleData.tagValuePattern.tag == "highway")
						useData = true;
				}
			}
			else
			{
				useData = true;
			}

			if (!useData)
				continue;

			dbCode = sqlite3_reset(mapSelStmt);
			dbCode = sqlite3_bind_int64(mapSelStmt, 1, iid);
		

			dbCode = sqlite3_step(mapSelStmt);


			if (dbCode != SQLITE_ROW)
			{
				sqlite3_reset(mapSelStmt);
				continue;
			}


			int closed = sqlite3_column_int(mapSelStmt, 1);
			const void* plData = sqlite3_column_blob(mapSelStmt, 2);
			int blobSize = sqlite3_column_bytes(mapSelStmt, 2);
			std::stringstream buffer;
			std::shared_ptr<std::vector<std::pair<float, float>>> coords(new std::vector<std::pair<float, float>>());
			std::shared_ptr<std::vector<std::pair<float, float>>> Incoords(new std::vector<std::pair<float, float>>());
			if (blobSize>0)
			{
				buffer.rdbuf()->sputn((const char*)plData, blobSize);
				std::vector<int> linodeData;
				readInts(buffer, linodeData);
				for (auto floatIt = linodeData.begin(); floatIt != linodeData.end(); floatIt +=2)
				{
					float lon = MapUtils::get31LongitudeX(*floatIt);
					float lat = MapUtils::get31LatitudeY(*(floatIt+1));
					coords->push_back(std::make_pair(lon, lat));
				}
				buffer.str(std::string());
				if (closed)
				{
					lines.emplace(std::make_tuple(iid<<2, typesUse[0]), std::move(coords));
				}
				else
				{
					lines.emplace(std::make_tuple(iid<<2+1, typesUse[0]), std::move(coords));
				}
			}
			plData = sqlite3_column_blob(mapSelStmt, 3);
			blobSize = sqlite3_column_bytes(mapSelStmt, 3);
			if (blobSize>0)
			{
				buffer.rdbuf()->sputn((const char*)plData, blobSize);
				std::vector<int> linodeData;
				readInts(buffer, linodeData);
				for (auto floatIt = linodeData.begin(); floatIt != linodeData.end(); floatIt +=2)
				{
					float lon = MapUtils::get31LongitudeX(*floatIt);
					float lat = MapUtils::get31LatitudeY(*(floatIt+1));
					Incoords->push_back(std::make_pair(lon, lat));
				}
				buffer.str(std::string());
				if (closed)
				{
					inlines.emplace(std::make_tuple(iid<<2, typesUse[0]), std::move(Incoords));
				}
				else
				{
					inlines.emplace(std::make_tuple(iid<<2+1, typesUse[0]), std::move(Incoords));
				}
			}
		}


		SkImage::Info info = {
			1600, 1000, SkImage::kPMColor_ColorType, SkImage::kPremul_AlphaType
		};
		SkAutoTUnref<SkSurface> imageRender(SkSurface::NewRaster(info));
		SkCanvas* painter = imageRender->getCanvas();
		painter->drawColor(SK_ColorWHITE);
		SkRect limits;
		painter->getClipBounds(&limits);
		SkScalar w = limits.width();
		SkScalar h = limits.height();
	
	
		double scale;
		double offx, offy, offmx, offmy;
		double gscale = DBL_MAX;
		double goffx = 1000, goffy = 1000;
		double goffmx = -1000, goffmy = -1000;
		
		std::set<std::shared_ptr<MultiPoly>>::iterator polit;
		for (polit = boundsLines.begin(); polit != boundsLines.end(); polit++)
		{
			(*polit)->getScaleOffsets(&scale, &offx, &offy,&offmx, &offmy, limits);
			goffx = min(goffx,offx);
			goffy = min(goffy,offy);
			goffmx = max(goffx,offmx);
			goffmy = max(goffy,offmy);

			gscale = min(gscale, scale);
		}


		double offsetX = goffx, offsetY = goffy;

		scale = gscale;

		SkPaint paint;
		paint.setTextSize(paint.getTextSize() + 15);
		paint.setColor(SK_ColorBLACK);
		paint.setStyle(SkPaint::Style::kStrokeAndFill_Style);
	
		for (polit = boundsLines.begin(); polit != boundsLines.end(); polit++)
		{
			(*polit)->paintImage(painter, gscale, goffx, goffy);
		}

		bool closed = true;
		for (auto nData : lines)
		{
			if ((*nData.second).size() > 1)
			{
				if (std::get<0>(nData.first) % 2 == 1)
				{
					closed = false;
				}
				else
				{
					closed = true;
				}


				tpl data = nData.first;

				MapRulType ruleData = renderEncoder.rules[std::get<1>(data)];
				paint.setStrokeWidth(1);

				if (ruleData.getValue() == "motorway")
				{
					paint.setStrokeWidth(3);
					paint.setColor(SK_ColorRED);
				}
				else if (ruleData.getValue() == "primary")
				{
					paint.setStrokeWidth(2);
					paint.setColor(SK_ColorGRAY);
				}
				else
				{
					continue;
				}
				std::pair<float, float> prevNode(0.0f,0.0f);
				for (auto coordData : *nData.second)
				{
					if (prevNode.first == 0.0f)
					{
						prevNode = coordData;
					}
					else
					{
						SkScalar pointX1 = (prevNode.first - offsetX) * scale;
						SkScalar pointY1 = h - (prevNode.second - offsetY) * scale;
						pointX1 = abs(pointX1);
						pointY1 = abs(pointY1);
						SkScalar pointX2 = (coordData.first - offsetX) * scale;
						SkScalar pointY2 = h - (coordData.second - offsetY) * scale;
						pointX2 = abs(pointX2);
						pointY2 = abs(pointY2);
						prevNode = coordData;
						painter->drawLine(pointX1,pointY1, pointX2, pointY2, paint);
					}
				}
				//if (closed)
				//{
				//	pathData.close();
				//}

			
			}
		/*		else
			{
				std::pair<float, float> Node = (*(nData.second))[0];
				SkScalar pointX1 = (Node.first - offsetX) * scale;
				SkScalar pointY1 = (Node.second - offsetY) * scale;
					pointX1 = abs(pointX1);
					pointY1 = abs(pointY1);
				painter->drawCircle(pointX1, pointY1, 2, paint);
			}
		*/
		}
	
		for (std::pair<std::shared_ptr<EntityNode>, CityObj> cityObj : cities)
		{
			if ( cityObj.second.getType() == "CITY")
			{
			std::pair<double, double> Node = cityObj.second.getLatLon();
			SkScalar pointX1 = (Node.second - offsetX) * scale;
				SkScalar pointY1 = h - (Node.first - offsetY) * scale;
				painter->drawCircle(pointX1, pointY1, 2, paint);
				painter->drawText(cityObj.second.getName().c_str(), cityObj.second.getName().size(), pointX1, pointY1+10,paint);
			}
		}


		SkAutoTUnref<SkImage> image(imageRender->newImageSnapshot());
		SkAutoDataUnref data(image->encode());
		if (NULL == data.get()) {
			return ;
		}
				
		std::string buffText("");
		buffText+= "_";
		buffText+= boost::lexical_cast<std::string>(indexMapTree);
		std::string pathImage = "D:\\osmData\\resultPolyImage" + buffText + std::string(".png");
		SkFILEWStream stream(pathImage.c_str());
		stream.write(data->data(), data->size());



	}
}

void OBFMapDB::writeBinaryMapIndex(BinaryMapDataWriter& writer, std::string regionName, OBFResultDB& dbContext)
{
	writer.writeStartMapIndex(regionName);


	writer.writeMapEncodingRules(renderEncoder.getRuleTypes());

	char* errMsg;
	sqlite3_stmt* pSelector = nullptr;
	int sqlResult = sqlite3_prepare_v2(dbContext.dbMapCtx, "SELECT area, coordinates, innerPolygons, types, additionalTypes, name FROM binary_map_objects WHERE id = ?", sizeof("SELECT area, coordinates, innerPolygons, types, additionalTypes, name FROM binary_map_objects WHERE id = ?"), &pSelector,(const char**)&errMsg);
	selectData = pSelector;
	boost::tuple<obf::MapDataBlock*,BinaryFileReference*> tpLa;

	namespace detail = boost::geometry::index::detail;

	
	
	std::unordered_map<__int64, boost::tuple<obf::MapDataBlock*,BinaryFileReference*>> treeHeaders;
	std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>> bouncer;
	for (int i = 0; i < mapZooms.size(); i++)
	{
		RTreeValued rtree = mapTree[i];
		RTreeValued::box rootBounds = rtree.calculateBounds();
		
		if (rootBounds.max_corner().get<0>() != 0) {
			writer.startWriteMapLevelIndex(mapZooms.getLevel(i).getMinZoom(), mapZooms.getLevel(i).getMaxZoom(),
				rootBounds.min_corner().get<0>(), rootBounds.max_corner().get<0>(), rootBounds.min_corner().get<1>(), rootBounds.max_corner().get<1>());

			boost::serializationOBF::saveMapOBF<BinaryMapDataWriter, RTreeValued::SI::value_type, RTreeValued::SI::parameters_type, RTreeValued::SI::indexable_getter, RTreeValued::SI::value_equal, RTreeValued::SI::allocator_type>(writer, rtree.spaceTree ,bouncer, *this,&rootBounds, mapZooms.getLevel(i));
			//writeBinaryMapTree(rtree, rootBounds, writer, treeHeaders);
					
			//writeBinaryMapBlock(rtree,  rootBounds,  writer, pSelector, treeHeaders, std::unordered_map<std::string, int>(), std::map<MapRulType, std::string>(), mapZooms.getLevel(i));

			writer.endWriteMapLevelIndex();
		}

	}
	writer.endWriteMapIndex();
	sqlite3_finalize(pSelector);
	selectData = nullptr;
	auto itBegin = bouncer.begin();
	std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>::iterator previt = itBegin;
	while (itBegin != bouncer.end())
	{
		if (previt->second.get() != nullptr)
		{
			std::wstringstream wstr;
			wstr << L"Bouncer unused : id" << itBegin->first << std::endl;
			OutputDebugString(wstr.str().c_str());
		}
		previt = itBegin;
		itBegin++;
	}
	/*for(auto bounce : bouncer)
	{
		if (!(bounce.second.operator int std::_Bool_struct<std::unique_ptr<BinaryFileReference, std::default_delete<BinaryFileReference>>>::*()))
		{
			std::wstringstream wstr;
			wstr << L"Bouncer unused : id" << bounce.first << std::endl;
			OutputDebugString(wstr.str().c_str());
		}
	}*/
}

void OBFMapDB::callNodeBox(const RTreeValued::box& boxParam, bool begin, bool isLeaf, BinaryMapDataWriter& writer, std::unordered_map<__int64, boost::tuple<obf::MapDataBlock*,BinaryFileReference*>>& bounds)
{
	if (begin)
	{
		std::unique_ptr<BinaryFileReference> ref = writer.startMapTreeElement(boxParam.min_corner().get<0>(), boxParam.max_corner().get<0>(), boxParam.min_corner().get<1>(), boxParam.max_corner().get<1>(), isLeaf, 0);
		if (ref) {
			const RTreeValued::box* intPtr = &boxParam;
			__int64 boxVal = (__int64)intPtr;
			if (bounds.find(boxVal) == bounds.end())
			{
  			 boost::tuple<obf::MapDataBlock*, BinaryFileReference*>& boxRef = bounds[boxVal];
			 boxRef.get<1>() = ref.release();
			 boxRef.get<0>() = nullptr;

			 obf::MapDataBlock* dataBlock = nullptr;
			 dataBlock = boxRef.get<0>();
			 if (dataBlock != nullptr)
			 {
				 boxRef.get<0>() = nullptr;
			 }
			 else
			 {
				  boxRef.get<0>() = nullptr;
			 }
			}
			else
			{
				boost::tuple<obf::MapDataBlock*, BinaryFileReference*>& boxRef = bounds[boxVal];
				boxRef.get<1>() = ref.release();
				boxRef.get<0>() = nullptr;
			}
		}
	}
	else
	{
		writer.endWriteMapTreeElement();
	}
}

void OBFMapDB::writeBinaryMapTree(RTreeValued& treeMap, RTreeValued::box& re, BinaryMapDataWriter& writer, std::unordered_map<__int64, boost::tuple<obf::MapDataBlock*,BinaryFileReference*>>& bounds)
{

		int countLeafs = 0;
		std::unique_ptr<BinaryFileReference> ref = writer.startMapTreeElement(re.min_corner().get<0>(), re.max_corner().get<0>(), re.min_corner().get<1>(), re.max_corner().get<1>(), false, 0);

		treeMap.getTreeNodes( [this,&writer, &bounds](const RTreeValued::box& boxParam, bool begin, bool isLeaf)
		{
			callNodeBox(boxParam, begin, begin, writer, bounds);
		}, nullptr);

		writer.endWriteMapTreeElement();
}


void OBFMapDB::callNodeBoxBlock(const RTreeValued::box& boxParam,const RTreeValued::value& valData, bool startBlock, bool inside, BinaryMapDataWriter& writer, std::unordered_map<__int64, boost::tuple<obf::MapDataBlock*,BinaryFileReference*>>& bounds,
						sqlite3_stmt* selectData, std::unordered_map<std::string, int>& tempStringTable, std::map<MapRulType, std::string>& tempNames, MapZooms::MapZoomPair level)
{
		obf::MapDataBlock* dataBlock = nullptr;
		const RTreeValued::box* intPtr = &boxParam;
		BinaryFileReference* ref;
		__int64 boxVal = (__int64)intPtr;
		if (!inside)
		{
			if (startBlock)
			{
				if (bounds.find(boxVal) != bounds.end())
				{
					ref = bounds[boxVal].get<1>();
					if (bounds[boxVal].get<0>() != nullptr)
					{
						dataBlock = bounds[boxVal].get<0>();
					}
				}
			}
			else
			{
				if (bounds.find(boxVal) != bounds.end())
				{
					ref = bounds[boxVal].get<1>();
					if (bounds[boxVal].get<0>() != nullptr)
					{
						dataBlock = bounds[boxVal].get<0>();
						writer.writeMapDataBlock(dataBlock, tempStringTable, *ref);
						delete dataBlock;
						delete ref;
						bounds.erase(bounds.find(boxVal));
					}
				}
			}
		}
		else
		{
			if (bounds.find(boxVal) != bounds.end())
			{
				if (bounds[boxVal].get<0>() != nullptr)
				{
					dataBlock = bounds[boxVal].get<0>();
				}
			}

		__int64 baseId = 0;
		//for (int i = 0; i < parent.getTotalElements(); i++) {
		//	if (e[i].getElementType() == rtree.Node.LEAF_NODE) {
				//long id = e[i].getPtr();
		__int64 callID = valData.second.first;
				sqlite3_bind_int64(selectData, 1, callID);
				// selectData = mapConnection.prepareStatement("SELECT area, coordinates, innerPolygons, types, additionalTypes, name FROM binary_map_objects WHERE id = ?");
				int dbResult = sqlite3_step(selectData); //ResultSet rs = selectData.executeQuery();
				if (dbResult == SQLITE_ROW) /*rs.next()*/ {
					__int64 cid = convertGeneratedIdToObfWrite(callID);
					if (dataBlock == nullptr) {
						baseId = cid;
						dataBlock = writer.createWriteMapDataBlock(baseId);
						if (bounds.find(boxVal) == bounds.end())
						{
							bounds[boxVal].get<0>() = dataBlock;
						}
						else
						{
							bounds[boxVal].get<0>() = dataBlock;
						}
						tempStringTable.clear();
					}
					tempNames.clear();
					const unsigned char* columnTextData = sqlite3_column_text(selectData, 5);
					if (columnTextData != nullptr && columnTextData[0] != 0)
					{
						std::string name(reinterpret_cast<const char*>(sqlite3_column_text(selectData, 5)));
						decodeNames(name, tempNames);
					}
					const void* plData = sqlite3_column_blob(selectData, 3);
					int bloblSize = sqlite3_column_bytes(selectData, 3);
					std::vector<int> typeUse(bloblSize / 2);
					for (int j = 0; j < bloblSize; j += 2) {
						int ids = parseSmallIntFromBytes(plData, j);
						typeUse[j / 2] = renderEncoder.getTypeByInternalId(ids).getTargetId();
					}
					plData = sqlite3_column_blob(selectData, 4);
					bloblSize = sqlite3_column_bytes(selectData, 4);
					std::vector<int> addtypeUse;
					if (bloblSize != 0) {
						addtypeUse.resize(bloblSize / 2);
						for (int j = 0; j < bloblSize; j += 2) {
							int ids = parseSmallIntFromBytes(plData, j);
							addtypeUse[j / 2] = renderEncoder.getTypeByInternalId(ids).getTargetId();
						}
					}
					

					
					obf::MapData mapData = writer.writeMapData(cid - baseId, boxParam.min_corner().get<0>(), boxParam.min_corner().get<1>(), selectData,
							typeUse, addtypeUse, tempNames, tempStringTable, dataBlock, level.getMaxZoom() > 15);
					if(mapData.IsInitialized()) {
						obf::MapData* mapDataToAdd = dataBlock->add_dataobjects();
						mapDataToAdd->MergeFrom(mapData);
					}
					
				} else {
					//logMapDataWarn.error("Something goes wrong with id = " + id); //$NON-NLS-1$
				}
				sqlite3_reset(selectData);
		}
}

void OBFMapDB::writeBinaryMapBlock(RTreeValued& treeMap, RTreeValued::box& parentBounds, BinaryMapDataWriter& writer, sqlite3_stmt* selectData,
			std::unordered_map<__int64, boost::tuple<obf::MapDataBlock*,BinaryFileReference*>>& bounds, std::unordered_map<std::string, int>& tempStringTable, std::map<MapRulType, std::string>& tempNames, MapZooms::MapZoomPair level)
{
		
	treeMap.getTreeNodes( nullptr, [this,&writer, &bounds, &selectData, &tempStringTable, &tempNames, &level](const RTreeValued::box& boxParam, const RTreeValued::value& valData, bool startBlock, bool inside)
		{
			callNodeBoxBlock(boxParam, valData, startBlock, inside, writer, bounds, selectData, tempStringTable, tempNames, level);
		});

	int numberEntry = 0;
	for(std::pair<__int64, boost::tuple<obf::MapDataBlock*,BinaryFileReference*>> boundPair : bounds)
	{
		numberEntry++;
		if (boundPair.second.get<0>() != nullptr && boundPair.second.get<1>() != nullptr) {
			obf::MapDataBlock* block = boundPair.second.get<0>();
			BinaryFileReference* binref = boundPair.second.get<1>();
		}
		else if (boundPair.second.get<0>() != nullptr)
		{
			delete boundPair.second.get<0>();
		}
		else if (boundPair.second.get<1>() != nullptr)
		{
			delete boundPair.second.get<1>();
		}
	}


		//MapDataBlock.Builder dataBlock = null;
		//BinaryFileReference ref = bounds.get(parent.getNodeIndex());
		//long baseId = 0;
		//for (int i = 0; i < parent.getTotalElements(); i++) {
		//	if (e[i].getElementType() == rtree.Node.LEAF_NODE) {
		//		long id = e[i].getPtr();
		//		selectData.setLong(1, id);
		//		// selectData = mapConnection.prepareStatement("SELECT area, coordinates, innerPolygons, types, additionalTypes, name FROM binary_map_objects WHERE id = ?");
		//		ResultSet rs = selectData.executeQuery();
		//		if (rs.next()) {
		//			long cid = convertGeneratedIdToObfWrite(id);
		//			if (dataBlock == null) {
		//				baseId = cid;
		//				dataBlock = writer.createWriteMapDataBlock(baseId);
		//				tempStringTable.clear();

		//			}
		//			tempNames.clear();
		//			decodeNames(rs.getString(6), tempNames);
		//			byte[] types = rs.getBytes(4);
		//			int[] typeUse = new int[types.length / 2];
		//			for (int j = 0; j < types.length; j += 2) {
		//				int ids = Algorithms.parseSmallIntFromBytes(types, j);
		//				typeUse[j / 2] = renderingTypes.getTypeByInternalId(ids).getTargetId();
		//			}
		//			byte[] addTypes = rs.getBytes(5);
		//			int[] addtypeUse = null ;
		//			if (addTypes != null) {
		//				addtypeUse = new int[addTypes.length / 2];
		//				for (int j = 0; j < addTypes.length; j += 2) {
		//					int ids = Algorithms.parseSmallIntFromBytes(addTypes, j);
		//					addtypeUse[j / 2] = renderingTypes.getTypeByInternalId(ids).getTargetId();
		//				}
		//			}
		//			
		//			
		//			MapData mapData = writer.writeMapData(cid - baseId, parentBounds.getMinX(), parentBounds.getMinY(), rs.getBoolean(1), rs.getBytes(2), rs.getBytes(3),
		//					typeUse, addtypeUse, tempNames, tempStringTable, dataBlock, level.getMaxZoom() > 15);
		//			if(mapData != null) {
		//				dataBlock.addDataObjects(mapData);
		//			}
		//		} else {
		//			logMapDataWarn.error("Something goes wrong with id = " + id); //$NON-NLS-1$
		//		}
		//	}
		//}
		//if (dataBlock != null) {
		//	writer.writeMapDataBlock(dataBlock, tempStringTable, ref);
		//}
		//for (int i = 0; i < parent.getTotalElements(); i++) {
		//	if (e[i].getElementType() != rtree.Node.LEAF_NODE) {
		//		long ptr = e[i].getPtr();
		//		rtree.Node ns = r.getReadNode(ptr);
		//		writeBinaryMapBlock(ns, e[i].getRect(), r, writer, selectData, bounds, tempStringTable, tempNames,level);
		//	}
		//}
	}
