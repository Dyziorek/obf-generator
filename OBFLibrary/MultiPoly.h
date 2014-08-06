#pragma once





class OBFResultDB;

class Ring : public EntityWay
{
public:
	Ring(EntityWay& way):EntityWay(way) { this->area = OsmMapUtils::getArea(nodes); }
	Ring(void) {area = -1;}
	virtual ~Ring(void) {}

	double area;
	bool isClosed() {return nodeIDs.front() == nodeIDs.back(); }
	
	bool containsPoint(EntityNode node)
	{
		return containsPoint(node.lat, node.lon);
	}
	bool containsPoint(double lat, double lon)
	{
		int intersections = 0;
		if (nodes.size() == 0) return false;
		for(unsigned int idx = 0; idx < nodes.size() - 2; idx++)
		{
			if (OsmMapUtils::ray_intersect_lon(nodes[idx], nodes[idx+1],lat, lon) != 360)
				intersections++;
		}

		return intersections %2 == 1;
	}
	bool isIn(std::shared_ptr<Ring> r);

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
		centerID = -1;
		updateRings();
	}
	virtual ~MultiPoly(void);
	bool operator==(const MultiPoly &other) const;

	std::vector<std::shared_ptr<Ring>> inRing;
	std::vector<std::shared_ptr<Ring>> outRing;
	std::map<std::shared_ptr<Ring>, std::set<std::shared_ptr<Ring>>> containedInnerInOuter;

	std::vector<std::shared_ptr<EntityWay>> inWays;
	std::vector<std::shared_ptr<EntityWay>> outWays;

	void build();
	void paint();
	void paint(std::pair<double, double> point, std::string);
	void paintList(std::vector<std::shared_ptr<EntityWay>> wayList, SkCanvas* painter);
	void createData(std::shared_ptr<EntityRelation>& relItem);
	std::list<std::shared_ptr<Ring>> combineRings(std::vector<std::shared_ptr<EntityWay>> inList);
	std::list<MultiPoly> splitPerRing();
	std::shared_ptr<EntityWay> combineTwoWaysIfHasPoints(std::shared_ptr<EntityWay> w1, std::shared_ptr<EntityWay> w2);
	std::unique_ptr<std::pair<double, double>> getCenterPoint();
	bool isValid();
	void mergeWith(std::vector<std::shared_ptr<Ring>> inRing, std::vector<std::shared_ptr<Ring>> outRing);
	void paintImage(SkCanvas* painter, double scale, double offsetX,double offsetY);
	bool containsPoint(std::pair<double, double> point);
	void generateImage(std::shared_ptr<EntityWay> lines, SkCanvas* painter, SkColor color, double scale,  double offsetX, double offsetY);
	void getScaleOffsets(double* scale, double* offX, double* offy, double* offmx, double* offmy, SkRect limits);
	__int64 centerID;
	__int64 id;
	int level;
	std::string polyName;
	std::string polyAltName;
	std::string polyType;
	void updateRings();

	typedef bg::model::point<double, 2, bg::cs::cartesian> pointD;
	typedef bg::model::box<pointD> boxD;
	typedef bg::model::linestring<pointD> lineD;
	typedef bg::model::segment<pointD> segmD;
	typedef bg::model::ring<pointD, true> ringD;
	typedef bg::model::polygon<pointD, true> polyD;
	

	std::vector<polyD> polygons;

private:
	float maxLat;
	float minLat;
	float maxLon;
	float minLon;
	
	static long numberCalls;
	static long initialValue;
	static long randomInterval;
	static long long nextRandId();
};

namespace boost
{
	template<>
	struct hash<MultiPoly>
	{
		std::size_t operator()(const MultiPoly& k) const
		{
			size_t seed;
			hash_combine(seed, k.centerID);
			hash_combine(seed, k.polyName);
			hash_combine(seed, k.polyAltName);
			hash_combine(seed, k.polyType);
			hash_combine(seed, k.level);
		};
	};
}
