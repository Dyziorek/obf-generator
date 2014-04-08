#include "stdafx.h"
#include "TileManager.h"
#include "EntityBase.h"
#include "EntityNode.h"
#include "MapObject.h"
#include "OBFRenderingTypes.h"
#include "OBFStreeDB.h"
#include "MapUtils.h"
#include "MultiPoly.h"
#include "OBFMapDB.h"
#include "SkCanvas.h"
#include "SkSurface.h"
#include "SkImage.h"
#include "SkData.h"
#include "SkStream.h"
#include "SkGraphics.h"
#include <boost/archive/binary_oarchive.hpp>

#include "ArchiveIO.h"


namespace io = boost::iostreams;
namespace ar = boost::archive;

long long OBFMapDB::notUsedId = - 1 << 40; // million million
int OBFMapDB::numberCalls = 0;

int OBFMapDB::MAP_LEVELS_POWER = 3;
int OBFMapDB::MAP_LEVELS_MAX = 1 << OBFMapDB::MAP_LEVELS_POWER;

OBFMapDB::OBFMapDB(void)
{
	mapZooms = *MapZooms::getDefault();
	zoomWaySmothness = 2;
	mapTree.reserve(MAP_LEVELS_MAX);
}


OBFMapDB::~OBFMapDB(void)
{
}

void OBFMapDB::excludeFromMainIteration(std::vector<std::shared_ptr<EntityWay>> l) {
		for(std::shared_ptr<EntityWay> w : l) {
			if(multiPolygonsWays.find(w->id) != multiPolygonsWays.end()) {
				multiPolygonsWays.insert(std::make_pair(w->id, std::vector<long>()));
			}
			multiPolygonsWays.at(w->id).insert(multiPolygonsWays.at(w->id).end(), typeUse.begin(), typeUse.end());
		}
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
	polyline->createData(relItem, dbContext);
	excludeFromMainIteration(polyline->inWays);
	excludeFromMainIteration(polyline->outWays);

	polyline->splitPerRing();
	
	
	polyLines.push_back(polyline);
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
	SkRect limits =  SkRect::MakeWH(800, 600);
	std::list<std::shared_ptr<MultiPoly>>::iterator polit;
	for (polit = polyLines.begin(); polit != polyLines.end(); polit++)
	{
		(*polit)->getScaleOffsets(&scale, &offx, &offy, limits);
		goffx = min(goffx,offx);
		goffy = min(goffy,offy);
		gscale = min(gscale, scale);
	}

	SkImage::Info info = {
			800, 600, SkImage::kPMColor_ColorType, SkImage::kPremul_AlphaType
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
		std::list<std::map<std::string, std::string>>  splitter = renderEncoder.splitTagsIntoDifferentObjects(baseItem->tags);
		if (splitter.size() > 1)
		{
			auto latLon = baseItem->getLatLon();
				boolean first = true;
				for(auto inst : splitter) {
					if(first) {
						baseItem->tags = inst;
						first = false;
						iterateMainEntityPost(baseItem);
					} else {
						std::shared_ptr<EntityNode> ns(new EntityNode(latLon.first, latLon.second, notUsedId--));
						ns->tags = inst;
						iterateMainEntityPost(std::static_pointer_cast<EntityBase, EntityNode>(ns));
					}
				}
		}
		else
		{
			iterateMainEntityPost(baseItem);
		}
	}

}

void OBFMapDB::iterateMainEntityPost(std::shared_ptr<EntityBase>& e) 
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
			boolean hasMulti = (wayItem) && multiPolygonsWays.find(e->id) != multiPolygonsWays.end();
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
				boolean mostDetailedLevel = level == 0;
				if (!mostDetailedLevel) {
					int zoomToSimplify = mapZooms.getLevel(level).getMaxZoom() - 1;
					boolean cycle = wayItem->getFirstNodeId() == wayItem->getLastNodeId();
					if (cycle) {
						res = OsmMapUtils::simplifyCycleWay(wayItem->nodes, zoomToSimplify, zoomWaySmothness);
					} else {
						if (namesUse.find(renderEncoder.nameRule) != namesUse.end())
						{
							std::string ename = namesUse.at(renderEncoder.nameRule);
							insertLowLevelMapBinaryObject(level, zoomToSimplify, typeUse, addtypeUse, id, wayItem->nodes, ename);
						}
					}
				} else {
					res = wayItem->getListNodes();
				}
			}
			if (!res.empty()) {
				insertBinaryMapRenderObjectIndex(mapTree[level], res, std::list<std::list<std::shared_ptr<EntityNode>>>(), namesUse, id, area, typeUse, addtypeUse, true);
			}
		}
	}



void OBFMapDB::insertLowLevelMapBinaryObject(int level, int zoom, std::list<long> types, std::list<long> addTypes, long id, std::vector<std::shared_ptr<EntityNode>> in, std::string name)
{
		
		std::vector<std::shared_ptr<EntityNode>> nodes;
		OsmMapUtils::simplifyDouglasPeucker(in, zoom + 8 + zoomWaySmothness, 3, nodes, false);
		boolean first = true;
		long firstId = -1;
		long lastId = -1;
		
		typedef io::basic_array<std::string> arrData;
		typedef io::stream<arrData> arrStrm;

		std::string bar;

		//arrStrm bNodes(bar);

		std::stringstream bNodeData;
		std::stringstream bTypesData;
		std::stringstream bAddtTypesData;

		portable_binary_oarchive bNodes(bNodeData);
		portable_binary_oarchive bTypes(bTypesData);
		portable_binary_oarchive bAddtTypes(bAddtTypesData);
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
					
					writeInt(bNodes, n->lat);
					writeInt(bNodes, n->lon);
					

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
				writeSmallInt(bTypes, *typesIt);
			} catch (std::exception e) {
			}
		}
		for (int j = 0; j < addTypes.size(); j++) {
			try {
				std::list<long>::iterator typesIt = types.begin();
				std::advance(typesIt, j);
				writeSmallInt(bAddtTypes, *typesIt);
			} catch (std::exception e) {
			}
		}
		/*mapLowLevelBinaryStat.setLong(1, id);
		mapLowLevelBinaryStat.setLong(2, firstId);
		mapLowLevelBinaryStat.setLong(3, lastId);
		mapLowLevelBinaryStat.setString(4, name);
		mapLowLevelBinaryStat.setBytes(5, bNodes.toByteArray());
		mapLowLevelBinaryStat.setBytes(6, bTypes.toByteArray());
		mapLowLevelBinaryStat.setBytes(7, bAddtTypes.toByteArray());
		mapLowLevelBinaryStat.setShort(8, (short) level);

		addBatch(mapLowLevelBinaryStat);
		*/
		
		addBatch(id, firstId, lastId, name, bNodeData, bTypesData, bAddtTypesData, level);

	}

void  OBFMapDB::insertBinaryMapRenderObjectIndex(RTree mapTree, std::list<std::shared_ptr<EntityNode>>& nodes, std::list<std::list<std::shared_ptr<EntityNode>>>& innerWays,
			std::map<MapRulType, std::string>& names, long id, bool area, std::list<long>& types, std::list<long>& addTypes, bool commit)
			{
		boolean init = false;
		int minX = INT_MAX;
		int maxX = 0;
		int minY = INT_MAX;
		int maxY = 0;

		std::stringstream bCoordData;
		std::stringstream bInCoordData;
		std::stringstream bTypesData;
		std::stringstream bAddtTypesData;

		portable_binary_oarchive bcoordinates(bCoordData);
		portable_binary_oarchive binnercoord(bInCoordData);
		portable_binary_oarchive btypes(bTypesData);
		portable_binary_oarchive badditionalTypes(bAddtTypesData);

		/*		 bcoordinates = new ByteArrayOutputStream();
		ByteArrayOutputStream binnercoord = new ByteArrayOutputStream();
		ByteArrayOutputStream btypes = new ByteArrayOutputStream();
		ByteArrayOutputStream badditionalTypes = new ByteArrayOutputStream();

		*/

			for (int j = 0; j < types.size(); j++) {
				std::list<long>::iterator typesIt = types.begin();
				std::advance(typesIt, j);
				writeSmallInt(btypes, *typesIt);
			}
			for (int j = 0; j < addTypes.size(); j++) 
			{
				std::list<long>::iterator atypesIt = addTypes.begin();
				std::advance(atypesIt, j);
				writeSmallInt(badditionalTypes, *atypesIt);
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
					writeInt(bcoordinates, x);
					writeInt(bcoordinates, y);
				}
			}

			if (innerWays.size()) {
				for (std::list<std::shared_ptr<EntityNode>> ws : innerWays) {
					boolean exist = false;
					if (ws.size()) {
						for (std::shared_ptr<EntityNode> n : ws) {
							if (n) {
								exist = true;
								int y = MapUtils::get31TileNumberY(n->lat);
								int x = MapUtils::get31TileNumberX(n->lon);
								writeInt(binnercoord, x);
								writeInt(binnercoord, y);
							}
						}
					}
					if (exist) {
						writeInt(binnercoord, 0);
						writeInt(binnercoord, 0);
					}
				}
			}
		
			addBatch(id, area, bCoordData, bInCoordData, bTypesData, bAddtTypesData, encodeNames(names));
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
	}

	std::string OBFMapDB::encodeNames(std::map<MapRulType, std::string> tempNames) {
		std::string returnText;
		std::stringstream strm(returnText);
		for (std::pair<MapRulType, std::string> e : tempNames) {
			if (e.second.size()) {
				strm << "~";
				strm << e.first.getInternalId();
				strm << e.second;
			}
		}
		return returnText;
	}
