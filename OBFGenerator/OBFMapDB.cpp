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


long long OBFMapDB::notUsedId = - 1 << 40; // million million
int OBFMapDB::numberCalls = 0;

OBFMapDB::OBFMapDB(void)
{
	mapZooms = MapZooms::getDefault();
}


OBFMapDB::~OBFMapDB(void)
{
}

void OBFMapDB::indexMapAndPolygonRelations(std::shared_ptr<EntityRelation>& relItem, OBFResultDB& dbContext)
{
	if (relItem->getTag("type") != "multipolygon" || relItem->getTag("admin_level") != "")
	{
		return;
	}

	std::shared_ptr<MultiPoly> polyline(new MultiPoly);
	polyline->createData(relItem, dbContext);
	polyLines.push_back(polyline);


	
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
	}

}

void OBFMapDB::iterateMainEntityPost(std::shared_ptr<EntityBase>& e) 
{
	std::shared_ptr<EntityNode> nodeItem = std::dynamic_pointer_cast<EntityNode, EntityBase>(e);
		for (int level = 0; level < mapZooms.size(); level++) {
			boolean area = renderEncoder.encodeEntityWithType(nodeItem, 
					e.getTags(), mapZooms.getLevel(level).getMaxZoom(), typeUse, addtypeUse, namesUse,
					tempNameUse);
			if (typeUse.isEmpty()) {
				continue;
			}
			boolean hasMulti = e instanceof Way && multiPolygonsWays.containsKey(e.getId());
			if (hasMulti) {
				TIntArrayList set = multiPolygonsWays.get(e.getId());
				typeUse.removeAll(set);
			}
			if (typeUse.isEmpty()) {
				continue;
			}
			long id = convertBaseIdToGeneratedId(e.getId(), level);
			List<Node> res = null;
			if (e instanceof Node) {
				res = Collections.singletonList((Node) e);
			} else {
				id |= 1;

				// simplify route id>>1
				boolean mostDetailedLevel = level == 0;
				if (!mostDetailedLevel) {
					int zoomToSimplify = mapZooms.getLevel(level).getMaxZoom() - 1;
					boolean cycle = ((Way) e).getFirstNodeId() == ((Way) e).getLastNodeId();
					if (cycle) {
						res = simplifyCycleWay(((Way) e).getNodes(), zoomToSimplify, zoomWaySmothness);
					} else {
						String ename = namesUse.get(renderingTypes.getNameRuleType());
						insertLowLevelMapBinaryObject(level, zoomToSimplify, typeUse, addtypeUse, id, ((Way) e).getNodes(), ename);
					}
				} else {
					res = ((Way) e).getNodes();
				}
			}
			if (res != null) {
				insertBinaryMapRenderObjectIndex(mapTree[level], res, null, namesUse, id, area, typeUse, addtypeUse, true);
			}
		}
	}