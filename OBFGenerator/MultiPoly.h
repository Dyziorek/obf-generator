#pragma once

#include "stdafx.h"
#include "EntityNode.h"
#include "MapUtils.h"
#include "SkCanvas.h"
#include "SkSurface.h"
#include "SkImage.h"
#include "SkData.h"
#include "SkStream.h"

class OBFResultDB;

class Ring : public EntityWay
{
public:
	Ring(EntityWay& way):EntityWay(way) { this->area = OsmMapUtils::getArea(nodes); }
	Ring(void) {area = -1;}
	double area;
	boolean isClosed() {return nodeIDs.front() == nodeIDs.back(); }
	
	boolean containsPoint(EntityNode node)
	{
		return containsPoint(node.lat, node.lon);
	}
	boolean containsPoint(double lat, double lon)
	{
		int intersections = 0;
		if (nodes.size() == 0) return false;
		for(unsigned int idx = 0; idx < nodes.size() - 1; idx++)
		{
			if (OsmMapUtils::ray_intersect_lon(*nodes[idx], *nodes[idx+1],lat, lon) != 360)
				intersections++;
		}

		return intersections %2 == 1;
	}
	boolean isIn(Ring r);

	bool operator<(const Ring& op2) {return area < op2.area;}

	void generateImage(SkCanvas* painter, SkColor color, double scale, double offsetX, double offsetY);
};

class MultiPoly
{
public:
	MultiPoly(void);
	MultiPoly(std::shared_ptr<Ring> outerRing, std::vector<std::shared_ptr<Ring>> innerRings, __int64 oid)
	{
		outRing.push_back(outerRing);
		inRing = innerRings;
		id = oid;
	}
	~MultiPoly(void);

	std::vector<std::shared_ptr<Ring>> inRing;
	std::vector<std::shared_ptr<Ring>> outRing;
	std::map<std::shared_ptr<Ring>, std::set<std::shared_ptr<Ring>>> containedInnerInOuter;

	std::vector<std::shared_ptr<EntityWay>> inWays;
	std::vector<std::shared_ptr<EntityWay>> outWays;

	void build();
	void paint();
	void paintList(std::vector<std::shared_ptr<EntityWay>> wayList, SkCanvas* painter);
	void createData(std::shared_ptr<EntityRelation>& relItem, OBFResultDB& dbContext);
	std::list<std::shared_ptr<Ring>> combineRings(std::vector<std::shared_ptr<EntityWay>> inList);
	std::list<MultiPoly> splitPerRing();
	std::shared_ptr<EntityWay> combineTwoWaysIfHasPoints(std::shared_ptr<EntityWay> w1, std::shared_ptr<EntityWay> w2);
	void mergeWith(std::vector<std::shared_ptr<Ring>> inRing, std::vector<std::shared_ptr<Ring>> outRing);
	void paintImage(SkCanvas* painter, double scale, double offsetX,double offsetY);
	void generateImage(std::shared_ptr<EntityWay> lines, SkCanvas* painter, SkColor color, double scale,  double offsetX, double offsetY);
	void getScaleOffsets(double* scale, double* offX, double* offy, SkRect limits);
	__int64 centerID;
	__int64 id;
	int level;
private:
	float maxLat;
	float minLat;
	float maxLon;
	float minLon;
	void updateRings();
	static long numberCalls;
	static long initialValue;
	static long randomInterval;
	static long long nextRandId();
};

