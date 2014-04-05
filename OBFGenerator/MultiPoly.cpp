#include "stdafx.h"
#include "MultiPoly.h"
#include "SkCanvas.h"
#include "SkSurface.h"
#include "SkImage.h"
#include "SkData.h"
#include "SkStream.h"
#include "SkGraphics.h"

SkAutoGraphics ag;
long MultiPoly::numberCalls = 0;

void Ring::generateImage(SkCanvas* painter, SkColor color, double scale,  double offsetX, double offsetY)
{
	SkRect limits;
	painter->getClipBounds(&limits);
	SkPaint paint;
	paint.setColor(color);
	paint.setStyle(SkPaint::Style::kStrokeAndFill_Style);
	SkScalar w = limits.width();
	SkScalar h = limits.height();

	std::shared_ptr<EntityNode> prevNode = nullptr;

	for (std::shared_ptr<EntityNode> nData : nodes)
	{
		if (prevNode == nullptr)
		{
			prevNode = nData;
		}
		else
		{
			SkScalar pointX1 = (prevNode->lon - offsetX) * scale;
			SkScalar pointY1 = h - (prevNode->lat - offsetY) * scale;
			SkScalar pointX2 = (nData->lon - offsetX) * scale;
			SkScalar pointY2 = h - (nData->lat - offsetY) * scale;
			painter->drawLine(pointX1, pointY1, pointX2, pointY2, paint);
			prevNode = nData;
		}
	}
	SkScalar pointX1 = (nodes[nodes.size()-1]->lat - offsetX) * scale;
	SkScalar pointY1 = h - (nodes[nodes.size()-1]->lon - offsetY) * scale;
	SkScalar pointX2 = (nodes[0]->lat - offsetX) * scale;
	SkScalar pointY2 = h - (nodes[0]->lon - offsetY) * scale;
	painter->drawLine(pointX1, pointY1, pointX2, pointY2, paint);
	

}


std::list<std::shared_ptr<Ring>> MultiPoly::combineRings(std::vector<std::shared_ptr<EntityWay>> inList)
{
	std::list<std::shared_ptr<EntityWay>> multiLines;
		for (std::shared_ptr<EntityWay> toAdd : inList) {
			if (toAdd->nodeIDs.size() < 2) {
				continue;
			}
			// iterate over the multiLines, and add the way to the correct one
			std::shared_ptr<EntityWay> changedWay = toAdd;
			std::shared_ptr<EntityWay> newWay;
			do {
				newWay = NULL;
				if(changedWay != NULL) {
					std::list<std::shared_ptr<EntityWay>>::iterator it = multiLines.begin();
					while(it != multiLines.end()){
						std::shared_ptr<EntityWay> w = *it;
						newWay = combineTwoWaysIfHasPoints(changedWay, w);
						if(newWay != NULL) {
							changedWay = newWay;
							multiLines.erase(it);
							break;
						}
						it++;
					}
				}
			} while(newWay != NULL);
			multiLines.push_back(changedWay);
			
		}
		std::list<std::shared_ptr<Ring>> result;
		for (std::shared_ptr<EntityWay> multiLine : multiLines) {
			result.push_back(std::shared_ptr<Ring>(new Ring(*multiLine)));
		}
		return result;
}

boolean Ring::isIn(Ring r) {
		/*
		 * bi-directional check is needed because some concave rings can intersect
		 * and would only fail on one of the checks
		 */
	std::vector<std::shared_ptr<EntityNode>> points = this->nodes;
		
		// r should contain all nodes of this
		for(std::shared_ptr<EntityNode> n : points) {
			if (!r.containsPoint(n->lat, n->lon)) {
				return false;
			}
		}
		
		points = r.nodes;
		
		// this should not contain a node from r
		for(std::shared_ptr<EntityNode> n : points) {
			if (this->containsPoint(n->lat, n->lon)) {
				return false;
			}
		}
		
		return true;
		
	}

MultiPoly::MultiPoly(void)
{
	level = -1;
	maxLat = -90;
	minLat = 90;
	maxLon = -180;
	minLon = 180;
}


MultiPoly::~MultiPoly(void)
{
}

void MultiPoly::build()
{
	std::list<std::shared_ptr<Ring>> innerRings = combineRings(inWays);
	if (innerRings.size() > 0)
	{
		inRing.reserve(innerRings.size());
		inRing.insert(inRing.begin(), innerRings.begin(), innerRings.end());
	}
	std::list<std::shared_ptr<Ring>> outRings = combineRings(outWays);
	if (outRings.size() > 0)
	{
		outRing.reserve(outRings.size());
		outRing.insert(outRing.begin(), outRings.begin(), outRings.end());
	}

	SkImage::Info info = {
        800, 600, SkImage::kPMColor_ColorType, SkImage::kPremul_AlphaType
    };
	SkAutoTUnref<SkSurface> imageRender(SkSurface::NewRaster(info));
	SkCanvas* painter = imageRender->getCanvas();
	painter->drawColor(SK_ColorWHITE);
	SkRect limits;
	painter->getClipBounds(&limits);
	SkScalar w = limits.width();
	SkScalar h = limits.height();

	double maxY = -1000, maxX = -1000;
	double minY = 1000, minX = 1000;
	for (std::shared_ptr<Ring> ring : outRings)
	{
		for (std::shared_ptr<EntityNode> nData : ring->nodes)
		{
			if (nData->lat > maxY)
			{
				maxY = nData->lat;
			}
			if (nData->lon > maxX)
			{
				maxX = nData->lon;
			}
			if (nData->lat < minY)
			{
				minY = nData->lat;
			}
			if (nData->lon < minX)
			{
				minX = nData->lon;
			}
		}
	}

	double offsetX = minX, offsetY = minY;



	double scale = 1.0;
	if (maxX - offsetX > w || maxY - offsetY > h)
	{
		if ((maxX - offsetX - w) > (maxY - offsetY - h))
		{
			scale = w / (maxX - offsetX);
		}
		else
		{
			scale = w / (maxY - offsetY);
		}
	}
	else if (maxX - offsetX < w && maxY - offsetY < h)
	{
		scale = min(w / (maxX - offsetX)   , h / (maxY - offsetY));
	}



	int colorIndex = 0;
	for(std::shared_ptr<Ring> ringPaint : outRings)
	{
		if (colorIndex % 5 == 0)
		{
			generateImage(ringPaint, painter, SK_ColorBLACK, scale, offsetX,offsetY);
		}
		if (colorIndex % 5 == 1)
		{
			generateImage(ringPaint, painter, SK_ColorBLUE, scale*1.05, offsetX,offsetY);
		}
		if (colorIndex % 5 == 2)
		{
			generateImage(ringPaint, painter, SK_ColorDKGRAY, scale, offsetX,offsetY);
		}
		if (colorIndex % 5 == 3)
		{
			generateImage(ringPaint, painter, SK_ColorGREEN, scale, offsetX,offsetY);
		}
		if (colorIndex % 5 == 4)
		{
			generateImage(ringPaint, painter, SK_ColorRED, scale, offsetX,offsetY);
		}
		colorIndex++;
	}
	SkAutoTUnref<SkImage> image(imageRender->newImageSnapshot());
    SkAutoDataUnref data(image->encode());
    if (NULL == data.get()) {
        return ;
    }
	char buff[10];
	_ultoa_s(numberCalls++, buff,10);
	std::string pathImage = "D:\\osmData\\resultImage" + std::string(buff) + std::string(".png");
    SkFILEWStream stream(pathImage.c_str());
    stream.write(data->data(), data->size());
	
	

}

std::shared_ptr<EntityWay> MultiPoly::combineTwoWaysIfHasPoints(std::shared_ptr<EntityWay> w1, std::shared_ptr<EntityWay> w2) {
		boolean combine = true;
		boolean firstReverse = false;
		boolean secondReverse = false;
		if (w1->getFirstNodeId() == w2->getFirstNodeId()) {
			firstReverse = true;
			secondReverse = false;
		} else if (w1->getLastNodeId() == w2->getFirstNodeId()) {
			firstReverse = false;
			secondReverse = false;
		} else if (w1->getLastNodeId() == w2->getLastNodeId()) {
			firstReverse = false;
			secondReverse = true;
		} else if (w1->getFirstNodeId() == w2->getLastNodeId()) {
			firstReverse = true;
			secondReverse = true;
		} else {
			combine = false;
		}
		if (combine) {
			std::shared_ptr<EntityWay> newWay(new EntityWay(MultiPoly::nextRandId()));
			boolean nodePresent = w1->nodes.size() != 0;
			int w1size = nodePresent ? w1->nodes.size() : w1->nodeIDs.size();
			for (int i = 0; i < w1size; i++) {
				int ind = firstReverse ? (w1size - 1 - i) : i;
				if (nodePresent) {
					newWay->nodes.push_back(w1->nodes[ind]);
					newWay->nodeIDs.push_back(w1->nodeIDs[ind]);
				} else {
					newWay->nodeIDs.push_back(w1->nodeIDs[ind]);
				}
			}
			int w2size = nodePresent ? w2->nodes.size() : w2->nodeIDs.size();
			for (int i = 1; i < w2size; i++) {
				int ind = secondReverse ? (w2size - 1 - i) : i;
				if (nodePresent) {
					newWay->nodes.push_back(w2->nodes[ind]);
					newWay->nodeIDs.push_back(w2->nodeIDs[ind]);
				} else {
					newWay->nodeIDs.push_back(w2->nodeIDs[ind]);
				}
			}
			return newWay;
		}
		return NULL;

	}


 long MultiPoly::initialValue = -1000;
 long MultiPoly::randomInterval = 5000;

	/**
	 * get a random long number
	 * 
	 * @return
	 */
	long long MultiPoly::nextRandId() {
		// exclude duplicates in one session (!) and be quazirandom every run
		long val = initialValue - rand() * randomInterval;
		initialValue = val;
		return val;
	}

	void MultiPoly::mergeWith(std::vector<std::shared_ptr<Ring>> iRing, std::vector<std::shared_ptr<Ring>> oRing)
	{
		inRing.insert(inRing.end(),iRing.begin(), iRing.end());
		outRing.insert(outRing.end(),oRing.begin(), oRing.end());
	}

	void MultiPoly::updateRings() {
		maxLat = -90;
		minLat = 90;
		maxLon = -180;
		minLon = 180;
		for (std::shared_ptr<Ring> r : outRing) {
			for (std::shared_ptr<EntityNode> n : r->nodes) {
				maxLat = (float) max(maxLat, n->lat);
				minLat = (float) min(minLat, n->lat);
				maxLon = (float) max(maxLon, n->lon);
				minLon = (float) min(minLon, n->lon);
			}
		}
		// keep sorted
		std::sort(outRing.begin(), outRing.end());
		for (std::shared_ptr<Ring> inner : inRing) {
			std::set<std::shared_ptr<Ring>> outContainingRings;
			for (std::shared_ptr<Ring> out : outRing) {
				if (inner->isIn(*out)) {
					outContainingRings.insert(out);
				}
			}
			containedInnerInOuter.insert(std::make_pair(inner, outContainingRings));

		}
		// keep sorted
		std::sort(inRing.begin(), inRing.end());
	}

	void MultiPoly::createData(std::shared_ptr<EntityRelation>& relItem, OBFResultDB& dbContext)
	{
		inWays.clear();
		outWays.clear();

		dbContext.loadRelationMembers(relItem.get());
		dbContext.loadNodesOnRelation(relItem.get());
		for(auto entityItem : relItem->relations)
		{
			if (entityItem.first.first == 1)
			{
				boolean inner = (entityItem.second == "inner");
				std::shared_ptr<EntityWay> wayPtr = std::dynamic_pointer_cast<EntityWay>(entityItem.first.second);
				if (inner)
				{
					inWays.push_back(wayPtr);
				}
				else
				{
					outWays.push_back(wayPtr);
				}
			}
		}
	}



	void MultiPoly::paint()
	{
		
		SkImage::Info info = {
			800, 600, SkImage::kPMColor_ColorType, SkImage::kPremul_AlphaType
		};
		SkAutoTUnref<SkSurface> imageRender(SkSurface::NewRaster(info));
		SkCanvas* painter = imageRender->getCanvas();
		painter->drawColor(SK_ColorWHITE);
		SkRect limits;
		paintList(outWays, painter);
		paintList(inWays, painter);
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

	void MultiPoly::paintImage(SkCanvas* painter, double scale, double offsetX,double offsetY)
	{
		int colorIndex = 0;
		for(std::shared_ptr<EntityWay> ringPaint : inWays)
		{
			if (colorIndex % 5 == 0)
			{
				generateImage(ringPaint, painter, SK_ColorBLACK, scale, offsetX,offsetY);
			}
			if (colorIndex % 5 == 1)
			{
				generateImage(ringPaint, painter, SK_ColorBLUE, scale, offsetX,offsetY);
			}
			if (colorIndex % 5 == 2)
			{
				generateImage(ringPaint, painter, SK_ColorDKGRAY, scale, offsetX,offsetY);
			}
			if (colorIndex % 5 == 3)
			{
				generateImage(ringPaint, painter, SK_ColorGREEN, scale, offsetX,offsetY);
			}
			if (colorIndex % 5 == 4)
			{
				generateImage(ringPaint, painter, SK_ColorRED, scale, offsetX,offsetY);
			}
			colorIndex++;
		}
		for(std::shared_ptr<EntityWay> ringPaint : outWays)
		{
			if (colorIndex % 5 == 0)
			{
				generateImage(ringPaint, painter, SK_ColorBLACK, scale, offsetX,offsetY);
			}
			if (colorIndex % 5 == 1)
			{
				generateImage(ringPaint, painter, SK_ColorBLUE, scale, offsetX,offsetY);
			}
			if (colorIndex % 5 == 2)
			{
				generateImage(ringPaint, painter, SK_ColorDKGRAY, scale, offsetX,offsetY);
			}
			if (colorIndex % 5 == 3)
			{
				generateImage(ringPaint, painter, SK_ColorGREEN, scale, offsetX,offsetY);
			}
			if (colorIndex % 5 == 4)
			{
				generateImage(ringPaint, painter, SK_ColorRED, scale, offsetX,offsetY);
			}
			colorIndex++;
		}
	}
	void MultiPoly::paintList(std::vector<std::shared_ptr<EntityWay>> wayList, SkCanvas* painter)
	{
		SkRect limits;
		painter->getClipBounds(&limits);
		SkScalar w = limits.width();
		SkScalar h = limits.height();
	double maxY = -1000, maxX = -1000;
	double minY = 1000, minX = 1000;
	for (std::shared_ptr<EntityWay> ring : wayList)
	{
		for (std::shared_ptr<EntityNode> nData : ring->nodes)
		{
			if (nData->lat > maxY)
			{
				maxY = nData->lat;
			}
			if (nData->lon > maxX)
			{
				maxX = nData->lon;
			}
			if (nData->lat < minY)
			{
				minY = nData->lat;
			}
			if (nData->lon < minX)
			{
				minX = nData->lon;
			}
		}
	}

	double offsetX = minX, offsetY = minY;



	double scale = 1.0;
	if (maxX - offsetX > w || maxY - offsetY > h)
	{
		if ((maxX - offsetX - w) > (maxY - offsetY - h))
		{
			scale = w / (maxX - offsetX);
		}
		else
		{
			scale = w / (maxY - offsetY);
		}
	}
	else if (maxX - offsetX < w && maxY - offsetY < h)
	{
		scale = min(w / (maxX - offsetX)   , h / (maxY - offsetY));
	}



	int colorIndex = 0;
	for(std::shared_ptr<EntityWay> ringPaint : wayList)
	{
		if (colorIndex % 5 == 0)
		{
			generateImage(ringPaint, painter, SK_ColorBLACK, scale, offsetX,offsetY);
		}
		if (colorIndex % 5 == 1)
		{
			generateImage(ringPaint, painter, SK_ColorBLUE, scale, offsetX,offsetY);
		}
		if (colorIndex % 5 == 2)
		{
			generateImage(ringPaint, painter, SK_ColorDKGRAY, scale, offsetX,offsetY);
		}
		if (colorIndex % 5 == 3)
		{
			generateImage(ringPaint, painter, SK_ColorGREEN, scale, offsetX,offsetY);
		}
		if (colorIndex % 5 == 4)
		{
			generateImage(ringPaint, painter, SK_ColorRED, scale, offsetX,offsetY);
		}
		colorIndex++;
	}
	

	}


	void MultiPoly::getScaleOffsets(double* scale, double* offX, double* offy, SkRect limits)
	{
		SkScalar w = limits.width();
		SkScalar h = limits.height();
		double maxY = -1000, maxX = -1000;
		double minY = 1000, minX = 1000;
		bool bVisited = false;
		for (std::shared_ptr<EntityWay> ring : inWays)
		{
			bVisited = true;
			for (std::shared_ptr<EntityNode> nData : ring->nodes)
			{
				if (nData->lat > maxY)
				{
					maxY = nData->lat;
				}
				if (nData->lon > maxX)
				{
					maxX = nData->lon;
				}
				if (nData->lat < minY)
				{
					minY = nData->lat;
				}
				if (nData->lon < minX)
				{
					minX = nData->lon;
				}
			}
		}

		for (std::shared_ptr<EntityWay> ring : outWays)
		{
			bVisited = true;
			for (std::shared_ptr<EntityNode> nData : ring->nodes)
			{
				if (nData->lat > maxY)
				{
					maxY = nData->lat;
				}
				if (nData->lon > maxX)
				{
					maxX = nData->lon;
				}
				if (nData->lat < minY)
				{
					minY = nData->lat;
				}
				if (nData->lon < minX)
				{
					minX = nData->lon;
				}
			}
		}

		double offsetX = minX, offsetY = minY;

		double locscale = 1.0;
		if (maxX - offsetX > w || maxY - offsetY > h)
		{
			if ((maxX - offsetX - w) > (maxY - offsetY - h))
			{
				locscale = w / (maxX - offsetX);
			}
			else
			{
				locscale = w / (maxY - offsetY);
			}
		}
		else if (maxX - offsetX < w && maxY - offsetY < h)
		{
			locscale = min(w / (maxX - offsetX)   , h / (maxY - offsetY));
		}
		if (bVisited)
		{
			*scale = locscale;
			*offX = offsetX;
			*offy = offsetY;
		}

	}

void MultiPoly::generateImage(std::shared_ptr<EntityWay> lines, SkCanvas* painter, SkColor color, double scale,  double offsetX, double offsetY)
{
	if (lines->nodes.size() < 1)
		return;

	SkRect limits;
	painter->getClipBounds(&limits);
	SkPaint paint;
	paint.setColor(color);
	paint.setStyle(SkPaint::Style::kStrokeAndFill_Style);
	SkScalar w = limits.width();
	SkScalar h = limits.height();

	std::shared_ptr<EntityNode> prevNode = nullptr;

	for (std::shared_ptr<EntityNode> nData : lines->nodes)
	{
		if (prevNode == nullptr)
		{
			prevNode = nData;
		}
		else
		{
			SkScalar pointX1 = (prevNode->lon - offsetX) * scale;
			SkScalar pointY1 = h - (prevNode->lat - offsetY) * scale;
			SkScalar pointX2 = (nData->lon - offsetX) * scale;
			SkScalar pointY2 = h - (nData->lat - offsetY) * scale;
			painter->drawLine(pointX1, pointY1, pointX2, pointY2, paint);
			prevNode = nData;
		}
	}
	SkScalar pointX1 = (lines->nodes[lines->nodes.size()-1]->lat - offsetX) * scale;
	SkScalar pointY1 = h - (lines->nodes[lines->nodes.size()-1]->lon - offsetY) * scale;
	SkScalar pointX2 = (lines->nodes[0]->lat - offsetX) * scale;
	SkScalar pointY2 = h - (lines->nodes[0]->lon - offsetY) * scale;
	painter->drawLine(pointX1, pointY1, pointX2, pointY2, paint);
	

}
