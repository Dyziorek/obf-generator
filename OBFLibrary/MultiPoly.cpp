#include "stdafx.h"
#include "SkCanvas.h"
#include "SkSurface.h"
#include "SkImage.h"
#include "SkData.h"
#include "SkStream.h"
#include "SkGraphics.h"
#include "RTree.h"
#include "MultiPoly.h"

#pragma push_macro("max")
#undef max
#pragma push_macro("min")
#undef min


//SkAutoGraphics ag;
long MultiPoly::numberCalls = 0;

void Ring::generateImage(SkCanvas* painter, SkColor color, double scale,  double offsetX, double offsetY)
{
	//typedef boost::geometry::model::polygon<boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>> polyD;
	//typedef boost::geometry::model::ring<boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>> ringD;
	//typedef boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian> ptD;
	//ptD pt;
	//pt.set<0>(10.0);
	//pt.set<1>(10.0);
	//ringD inn;
	//inn.push_back(pt);
	//polyD polyData;

	//polyData.inners().push_back(inn);

	

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

bool Ring::isIn(std::shared_ptr<Ring> r) {
		/*
		 * bi-directional check is needed because some concave rings can intersect
		 * and would only fail on one of the checks
		 */
	std::vector<std::shared_ptr<EntityNode>> points = this->nodes;
		
		// r should contain all nodes of this
		for(std::shared_ptr<EntityNode> n : points) {
			if (!r->containsPoint(n->lat, n->lon)) {
				return false;
			}
		}
		
		points = r->nodes;
		
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
	id = -1;
	centerID = -1;
}


MultiPoly::~MultiPoly(void)
{
}

bool MultiPoly::operator==(const MultiPoly &other) const
{
	return polyName == other.polyName
		&& centerID == other.centerID 
		&& polyAltName == other.polyAltName 
		&& polyType == other.polyType;
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

	updateRings();

	if (false)
	{
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
		scale = std::min<SkScalar>(w / (maxX - offsetX)   , h / (maxY - offsetY));
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
	
	

}

std::shared_ptr<EntityWay> MultiPoly::combineTwoWaysIfHasPoints(std::shared_ptr<EntityWay> w1, std::shared_ptr<EntityWay> w2) {
		bool combine = true;
		bool firstReverse = false;
		bool secondReverse = false;
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
			bool nodePresent = w1->nodes.size() != 0;
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

	std::unique_ptr<std::pair<double, double>> MultiPoly::getCenterPoint() {
		std::vector<std::shared_ptr<EntityNode>> points;
		for (std::shared_ptr<Ring> w : inRing) {
			points.insert(points.end(),w->nodes.begin(), w->nodes.end());
		}
		
		for (std::shared_ptr<Ring> w : outRing) {
			points.insert(points.end(),w->nodes.begin(), w->nodes.end());
		}
		
		std::pair<double,double>* center = new std::pair<double,double>(OsmMapUtils::getWeightCenterForNodes(points));
		
		return std::unique_ptr<std::pair<double,double>>(center);
		}

	bool MultiPoly::containsPoint(std::pair<double, double> point)
	{
		// fast check
		if(maxLat + 0.3 < point.first || minLat - 0.3 > point.first || 
			maxLon + 0.3 < point.second || minLon - 0.3 > point.second) {
			return false;
		}
		
		
		pointD ptCheck;
		ptCheck.set<0>(point.first);
		ptCheck.set<1>(point.second);
		polyD polyWork;
		double areaVal;

		bool coveredBoost = false;
		
		ringD containedInner;

		for(polyD polyData:  polygons)
		{
			if (bg::covered_by(ptCheck, polyData.outer()))
			{
				polyWork = polyData;
				areaVal = bg::area(polyData.outer());
				coveredBoost = true;
				break;
			}
		}

		if (!coveredBoost)
			return false;

		for(ringD containedInner:  polyWork.inners())
		{
			if (bg::covered_by(ptCheck, containedInner))
			{
				coveredBoost = false;
				break;
			}
		}

		if (coveredBoost)
		{
			paint(point, "boostTrue");
		}
		return coveredBoost;

		ringD outerRing = polyWork.outer();

		for(ringD inData: polyWork.inners())
		{
			if (bg::covered_by(ptCheck, inData))
			{
				containedInner = inData;
			}
		}

		//if (containedInner.size() > 0)
		//{
		//	return false;
		//}
		//else if (outerRing.size() > 0)
		//{
		//	return true;
		//}
		std::shared_ptr<Ring> containedInOuter;
		// use a sortedset to get the smallest outer containing the point
		for (std::shared_ptr<Ring> outer : outRing) {
			if (outer->containsPoint(point.first, point.second)) {
				containedInOuter = outer;
				break;
			}
		}
		
		if (!containedInOuter) {
			if (coveredBoost)
			{
				bg::correct(outerRing);
				paint(point, "boostTrue");
			}
			return false;
		}

		
		//use a sortedSet to get the smallest inner Ring
		std::shared_ptr<Ring> containedInInner;
		for (std::shared_ptr<Ring> inner : inRing) {
			if (inner->containsPoint(point.first, point.second)) {
				containedInInner = inner;
				break;
			}
		}

		if (!containedInInner && coveredBoost == false)
		{
			bg::correct(outerRing);
			paint(point, "boostFalse");
		}
		

		if (!containedInInner) return true;
		if (outRing.size() == 1) {
			// return immediately false 
			return false;
		}
		
		// if it is both, in an inner and in an outer, check if the inner is indeed the smallest one
		std::set<std::shared_ptr<Ring>> s = containedInnerInOuter[containedInInner];
		if(containedInnerInOuter.find(containedInInner) == containedInnerInOuter.end()) {
			throw std::out_of_range("");
		}
		return containedInnerInOuter.find(containedInOuter) == containedInnerInOuter.end();
	}

	bool MultiPoly::isValid()
	{
		return level > 4 && getCenterPoint()->first != -1000 && polyName != "";
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
				maxLat = std::max<float>(maxLat, n->lat);
				minLat = std::min<float>(minLat, n->lat);
				maxLon = std::max<float>(maxLon, n->lon);
				minLon = std::min<float>(minLon, n->lon);
			}
		}
		// keep sorted
		std::sort(outRing.begin(), outRing.end());
		for (std::shared_ptr<Ring> inner : inRing) {
			std::set<std::shared_ptr<Ring>> outContainingRings;
			for (std::shared_ptr<Ring> out : outRing) {
				if (inner->isIn(out)) {
					outContainingRings.insert(out);
				}
			}
			containedInnerInOuter.insert(std::make_pair(inner, outContainingRings));

		}
		// keep sorted
		std::sort(inRing.begin(), inRing.end());


		polyD polyData;
		

		auto inData = polyData.inners();
		auto outData = polyData.outer();

		for (auto ringData : inRing)
		{
			ringD inRing;
			for (auto ringNode : ringData->getListNodes())
			{
				pointD ptData;
				LatLon coord =  ringNode->getLatLon();
				ptData.set<0>(coord.first);
				ptData.set<1>(coord.second);
				inRing.push_back(ptData);
			}
			inData.push_back(inRing);
		}
		for(auto ringData = outRing.begin(); ringData != outRing.end(); ringData++)
		{
			polyD polyData;
			auto outData = polyData.outer();
			for (auto ringNode : ringData->get()->getListNodes())
			{
				pointD ptData;
				LatLon coord =  ringNode->getLatLon();
				ptData.set<0>(coord.first);
				ptData.set<1>(coord.second);
				outData.push_back(ptData);
			}
			polyData.outer() = outData;
			bg::correct(polyData);
			polygons.push_back(polyData);
		}
		
		
	}

	void MultiPoly::createData(std::shared_ptr<EntityRelation>& relItem)
	{
		inWays.clear();
		outWays.clear();

		for(auto entityItem : relItem->relations)
		{
			if (std::get<0>(entityItem.second) == 1)
			{
				bool inner = std::get<2>(entityItem.second) == "inner";
				std::shared_ptr<EntityWay> wayPtr = std::dynamic_pointer_cast<EntityWay>(std::get<1>(entityItem.second));
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
		updateRings();
	}

	void MultiPoly::paint(std::pair<double, double> point, std::string prefix)
	{
	
		if(numberCalls > 100)
			return;

		SkImage::Info info = {
			800, 600, SkImage::kPMColor_ColorType, SkImage::kPremul_AlphaType
		};
		double scale =0;
	double offx, offy;
	double gscale = 0;
	SkScalar gmaxx = maxLon, gmaxy = maxLat;
	SkScalar goffx = minLon, goffy = minLat;
	
	if (point.first > maxLat)
	{
		gmaxy = point.first;
	}
	else if (point.first < minLat)
	{
		goffy = point.first;
	}
	if (point.second > maxLon)
	{
		gmaxx = point.second;
	}
	else if (point.second < minLon)
	{
		goffx = point.second;
	}
		SkAutoTUnref<SkSurface> imageRender(SkSurface::NewRaster(info));
		SkCanvas* painter = imageRender->getCanvas();
		painter->drawColor(SK_ColorWHITE);
		SkRect limits;
		painter->getClipBounds(&limits);
		gmaxx+=0.001;
		goffx-=0.001;
		gmaxy+=0.001;
		goffy-=0.001;
		SkScalar minScale = std::min(limits.height() / (gmaxy - goffy) , limits.width() / (gmaxx - goffx));
		
		//getScaleOffsets(&scale, &offx, &offy, &goffx, &goffy,limits);
		for(auto outRingData : outRing)
		{
			outRingData->generateImage(painter, SK_ColorBLACK, minScale, goffx, goffy);
		}
		//paintImage(painter,minScale, goffx, goffy);
		
		SkScalar pointX1 = (point.second - goffx) * minScale;
		SkScalar pointY1 = limits.height() - (point.first - goffy) * minScale;

		SkPaint type;
		type.setColor(SK_ColorDKGRAY);
		type.setStrokeWidth(5);
		type.setStyle(SkPaint::Style::kStrokeAndFill_Style);
		painter->drawPoint(pointX1, pointY1, type);

		SkAutoTUnref<SkImage> image(imageRender->newImageSnapshot());
		SkAutoDataUnref data(image->encode());
		if (NULL == data.get()) {
			return ;
		}
		char buff[10];
		_ultoa_s(numberCalls++, buff,10);
		std::string pathImage = "D:\\osmData\\" + prefix + boost::lexical_cast<std::string, int>(numberCalls) + std::string(".png");
		SkFILEWStream stream(pathImage.c_str());

		stream.write(data->data(), data->size());
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
		scale = std::min<double>(w / (maxX - offsetX)   , h / (maxY - offsetY));
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


	void MultiPoly::getScaleOffsets(double* scale, double* offX, double* offy, double* maxx, double* maxy , SkRect limits)
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
			locscale = std::min<double>(w / (maxX - offsetX)   , h / (maxY - offsetY));
		}
		if (bVisited)
		{
			*scale = locscale;
			*offX = offsetX;
			*offy = offsetY;
			*maxx = maxX;
			*maxy = maxY;
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


std::list<MultiPoly> MultiPoly::splitPerRing() {
	std::list<std::shared_ptr<Ring>> listRings = combineRings(inWays);
	std::set<std::shared_ptr<Ring>> inners = std::set<std::shared_ptr<Ring>>(listRings.begin(), listRings.end());
		std::list<std::shared_ptr<Ring>> outers = combineRings(outWays);
		std::list<MultiPoly> multipolygons;
		// loop; start with the smallest outer ring
		for (std::shared_ptr<Ring> outer : outers) {
			std::list<std::shared_ptr<Ring>> innersInsideOuter;
			auto innerIt = inners.begin();
			while (innerIt != inners.end()) {
				std::shared_ptr<Ring> inner = *innerIt;
				if (inner->isIn(outer)) {
					innersInsideOuter.push_back(inner);
					auto innerItRemove = innerIt;
					innerIt++;
					inners.erase(innerItRemove);
				}
				else
				{
					innerIt++;
				}
			}
			multipolygons.push_back(MultiPoly(outer, std::vector<std::shared_ptr<Ring>>(innersInsideOuter.begin(), innersInsideOuter.end()), id));
		}

		if (inners.size() != 0) {
			std::wstring warn(L"Multipolygon ");
			warn += boost::lexical_cast<std::wstring>(id) + L" has a mismatch in outer and inner rings\r\n";
			OutputDebugString(warn.c_str());
		}

		return multipolygons;
	}

#pragma pop_macro("max")
#pragma pop_macro("min")
