#pragma once
#include "OBFResultDB.h"
#include "MapRoutingTypes.h"
#include "RTree.h"

class OBFpoiDB :
	public OBFResultDB
{
public:
	typedef boost::geometry::model::box<boost::geometry::model::point<int, 2, boost::geometry::cs::cartesian>> boxI;

	//struct poiEqual;

	
private:
	struct POITree
	{
		POIBox node;
		std::list<std::shared_ptr<POITree>> subNodes;
		void collectDataFromLevel(std::list<POIBox>& data, int level)
		{
			if (level == 0)
			{
				data.push_back(node);
			}
			if (level > 0)
			{
				if (!subNodes.empty())
				{
					for(std::shared_ptr<POITree> subNode : subNodes)
					{
						subNode->collectDataFromLevel(data, level - 1);
					}
				}
			}
		}

		int getSubTreesOnLevel(int level) {
			if (level == 0) {
				if (subNodes.empty()) {
					return 0;
				} else {
					return subNodes.size();
				}
			} else {
				int sum = 0;
				if (!subNodes.empty()) {
					for (std::shared_ptr<POITree> t : subNodes) {
						sum += t->getSubTreesOnLevel(level - 1);
					}
				}
				return sum;
			}
		}
		void extractChildrenFromLevel(int level) {
			std::list<std::shared_ptr<POITree>> list;
			collectChildrenFromLevel(list, level);
			subNodes = list;
		}

		void collectChildrenFromLevel(std::list<std::shared_ptr<POITree>>& list, int level) {
			if (level == 0) {
				if (!subNodes.empty()) {
					list.insert(list.begin(),subNodes.begin(), subNodes.end());
				}
			} else if (!subNodes.empty()) {
				for (std::shared_ptr<POITree> sub : subNodes) {
					sub->collectChildrenFromLevel(list, level - 1);
				}

			}

		}
	};

	OBFRenderingTypes renderer;
	std::map<long long, std::unordered_map<std::string, std::string>> propagatedTags;
	std::list<Amenity> tempAmenityList;
public:
	OBFpoiDB(void);
	virtual ~OBFpoiDB(void);
	void indexRelations(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext);
	void iterateMainEntity(std::shared_ptr<EntityBase>& relItem, OBFResultDB& dbContext);
	void insertAmenityIntoPoi(Amenity amenity, OBFResultDB& dbContext);
	void writePoiDataIndex(BinaryMapDataWriter& writer, OBFResultDB& dbContext, std::string poiTableName);
	void processPOIIntoTree(OBFResultDB& dbCtx, POITree& treeData, int zoomLevel, boxI& bbox, std::unordered_map<std::string, std::unordered_set<POIBox>>& nameIndex);
	void writePoiBoxes(BinaryMapDataWriter& writer, std::shared_ptr<POITree> tree, 
			__int64 startFpPoiIndex, std::unordered_map<POIBox,  std::list<std::shared_ptr<BinaryFileReference>>>& fpToWriteSeeks,
			POICategory& globalCategories);


private:
	void addNamePrefix(std::unordered_map<MapRulType, std::string>::iterator& name, std::unordered_map<MapRulType, std::string>::iterator& nameEn, POIBox data, std::unordered_map<std::string, std::unordered_set<POIBox>>& poiData);
	void parsePrefix(std::string name, POIBox data, std::unordered_map<std::string, std::unordered_set<POIBox>>& poiData);
	void decodeAdditionalType(const unsigned char* addTypeChar, std::unordered_map<MapRulType, std::string>&  typeMap);
};




class OBFtransportDB :
	public OBFResultDB
{
public:
	OBFtransportDB(void);
	virtual ~OBFtransportDB(void);
};


class OBFrouteDB :
	public OBFResultDB
{
public:
	typedef RTree<std::pair<__int64, std::vector<short>>> RTreeValued;
private:
	static long SHIFT_INSERT_AT;
	static long SHIFT_ORIGINAL;
	static long SHIFT_ID;
	static int CLUSTER_ZOOM;
	static std::string CONFLICT_NAME;
	static float DOUGLAS_PEUKER_DISTANCE;
	static __int64 getBaseId(int x31, int y31) {
		__int64 x = x31;
		__int64 y = y31;
		return (x << 31) + y;
	}
public:
	 class GeneralizedWay {
	 public:
		 __int64 id;
		 int mainType;
		std::unordered_set<int> addtypes;
		std::vector<int> px;
		std::vector<int> py;
		
		std::map<MapRouteType, std::string> names;

	 public:
		 GeneralizedWay(__int64 id) {
			this->id = id;
		}
		
		 bool operator==(GeneralizedWay const& op2) const
		 {
			 return id == op2.id && mainType == op2.mainType;
		 }

		double getDistance() {
			double dx = 0;
			for (int i = 1; i < px.size(); i++) {
				dx += MapUtils::getDistance(MapUtils::get31LatitudeY(py[i - 1]), MapUtils::get31LongitudeX(px[i - 1]),
						MapUtils::get31LatitudeY(py[i]), MapUtils::get31LongitudeX(px[i]));
			}
			return dx;
		}
		
		__int64 getLocation(int ind) {
			return getBaseId(px[ind], py[ind]);
		}
		
		int size(){
			return px.size();
		}
		
		// Gives route direction of EAST degrees from NORTH ]-PI, PI]
		double directionRoute(int startPoint, boolean plus) {
			float dist = 5;
			int x = this->px[startPoint];
			int y = this->py[startPoint];
			int nx = startPoint;
			int px = x;
			int py = y;
			double total = 0;
			do {
				if (plus) {
					nx++;
					if (nx >= size()) {
						break;
					}
				} else {
					nx--;
					if (nx < 0) {
						break;
					}
				}
				px = this->px[nx];
				py = this->py[nx];
				// translate into meters
				total += abs(px - x) * 0.011l + abs(py - y) * 0.01863l;
			} while (total < dist);
			return atan2( x - px, y - py );
		}
	};


	 struct hashWay : std::unary_function<GeneralizedWay, size_t>
	 {
		 template<typename hasherWay>
		 size_t operator()(hasherWay const &hashVal) const
		 {
			size_t seed = 0;
			boost::hash_combine(seed, hashVal.id);
			boost::hash_combine(seed, hashVal.mainType);
		 }
	 };

	 struct equalWay : std::binary_function<GeneralizedWay, GeneralizedWay, bool>
	 {
		 template<typename Gen1, typename Gen2>
		 bool operator()(Gen1 const &op1, Gen2 const &op2) const
		 {
			 return op1 == op2;
		 }
	 };

	  class GeneralizedCluster {
	  public:
		  int x;
		  int y;
		  int zoom;
		
		GeneralizedCluster(int x, int y, int z){
			this->x = x;
			this->y = y;
			this->zoom = z;
		}
		
		boost::unordered::unordered_set<GeneralizedWay, hashWay, equalWay> ways;
		// either LinkedList<GeneralizedWay> or GeneralizedWay
		typedef std::unordered_map<__int64, boost::container::list<GeneralizedWay>>::iterator mapIt;
		std::unordered_map<__int64, boost::container::list<GeneralizedWay>> map;
		
		
		void replaceWayFromLocation(GeneralizedWay deleteWay, int ind, GeneralizedWay toReplace){
			ways.erase(deleteWay);
			__int64 loc = deleteWay.getLocation(ind);
			mapIt o = map.find(loc);
			if(o != map.end() && o->second.size() == 1){
				if(deleteWay == o->second.front()) {
					boost::container::list<GeneralizedWay> intData;
					intData.push_back(toReplace);
					map.insert(std::make_pair(loc,intData));
				} else if(!(toReplace ==  o->second.front())){
					addWay(toReplace, loc);
				}
			} else if(o != map.end() && o->second.size() > 1){
				o->second.remove(deleteWay);
				auto oRes = std::find(o->second.begin(), o->second.end(), toReplace);
				if( oRes == o->second.end()){
					o->second.push_back(toReplace);
				}
			} else {
				boost::container::list<GeneralizedWay> intData;
				intData.push_back(toReplace);
				map.insert(std::make_pair(loc, intData));
			}
		}
		
		 void removeWayFromLocation(GeneralizedWay deleteWay, int ind){
			removeWayFromLocation(deleteWay, ind, false);
		}
		
		void removeWayFromLocation(GeneralizedWay deleteWay, int ind, boolean deleteAll) {
			__int64 loc = deleteWay.getLocation(ind);
			boolean ex = false;
			if (!deleteAll) {
				for (int t = 0; t < deleteWay.size(); t++) {
					if (t != ind && map.find(deleteWay.getLocation(t)) != map.end() ) {
						ex = true;
						break;
					}
				}
			}
			if (!ex || deleteAll) {
				ways.erase(deleteWay);
			}

			mapIt o = map.find(loc);
			if (o != map.end() && o->second.size() == 1) {
				if(deleteWay == o->second.front()) {
					map.erase(loc);
				}
			} else if (o != map.end() && o->second.size() > 1) {
				o->second.remove(deleteWay);
				if (o->second.size() == 1) {
					boost::container::list<GeneralizedWay> intData;
					intData.push_back(o->second.front());
					map.insert(std::make_pair(loc, intData));
				} else if (o->second.size() == 0) {
					map.erase(loc);
				}
			}
		}
		
		void addWayFromLocation(GeneralizedWay w, int i) {
			ways.insert(w);
			__int64 loc = w.getLocation(i);
			addWay(w, loc);
		}

	  private:
		void addWay(GeneralizedWay w, long loc) {
			mapIt o = map.find(loc);
			if (o != map.end()) {
				if ( o->second.size() > 1) {
					auto oRes = std::find(o->second.begin(), o->second.end(), w);
					if(oRes == o->second.end() ){
						o->second.push_back(w);
					}
				} else if(!(o->second.front() == w)){
					boost::container::list<GeneralizedWay> intData;
					intData.push_back(o->second.front());
					intData.push_back(w);
					map.insert(std::make_pair(loc, intData));
				}
			} else {
				boost::container::list<GeneralizedWay> intData;
				intData.push_back(w);
				map.insert(std::make_pair(loc, intData));
			}
		}
	};

	class RouteMissingPoints 
	{
	public:
		std::map<int, __int64> pointsMap;
		std::vector<std::shared_ptr<std::vector<int>>> pointsXToInsert;
		std::vector<std::shared_ptr<std::vector<int>>> pointsYToInsert;
		
		void buildPointsToInsert(int targetLength){
			for(auto p : pointsMap) {
				int insertAfter = p.first & ((1 << SHIFT_INSERT_AT) -1);
				if(pointsXToInsert[insertAfter]) {
					pointsXToInsert[insertAfter].swap(std::shared_ptr<std::vector<int>>(new std::vector<int>()));
					pointsYToInsert[insertAfter].swap(std::shared_ptr<std::vector<int>>(new std::vector<int>()));
				}
				__int64 x = p.second >> 31;
				__int64 y = p.second - (x << 31);
				pointsXToInsert[insertAfter]->push_back((int) x);
				pointsYToInsert[insertAfter]->push_back((int) y);
			}
		}
	};
public:
	std::vector<int> outTypes;
	std::map<MapRouteType, std::string> names;
	std::map<__int64, std::vector<int>> pointTypes;
	RTreeValued routeTree;
	RTreeValued baserouteTree;
	sqlite3_stmt* selectData;
	OBFrouteDB(void);
	virtual ~OBFrouteDB(void);
	void indexHighwayRestrictions(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext);
	void indexRelations(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext);
	void iterateMainEntity(std::shared_ptr<EntityBase>& item, OBFResultDB& dbContext);
	void processLowLevelWays(OBFResultDB& dbContext);
	int countAdjacentRoads(GeneralizedCluster& gcluster, GeneralizedWay& gw, int i);
	void addWayToIndex(long long id, std::vector<std::shared_ptr<EntityNode>>& nodes, OBFResultDB& dbContext, RTreeValued& rTree,  bool base);
	void registerBaseIntersectionPoint(long long pointLoc, bool registerId, long long wayId, int insertAt, int originalInd);
	std::string encodeNames(std::map<MapRouteType, std::string> tempNames);
	void putIntersection(long long  point, long long wayNodeId);
	std::unordered_map<__int64, std::list<__int64>> highwayRestrictions;
	std::unordered_map<__int64, __int64> basemapRemovedNodes;
	std::unordered_map<__int64, RouteMissingPoints> basemapNodesToReinsert;
	std::unordered_map<__int64, GeneralizedCluster> generalClusters;
	std::map<long long, std::unordered_map<std::string, std::string>> propagatedTags;
	OBFRenderingTypes renderer;
	MapRoutingTypes routingTypes;

	void processRoundabouts(std::vector<GeneralizedCluster>& clusters);
	void removeWayAndSubstituteWithPoint(GeneralizedWay& gw, GeneralizedCluster& gcluster);
	GeneralizedCluster getCluster(GeneralizedWay& gw, int ind, GeneralizedCluster& helper);
	double orthogonalDistance(GeneralizedWay& gn, int st, int end, int px, int py, bool returnNanIfNoProjection);
	void douglasPeukerSimplificationStep(std::vector<GeneralizedCluster>& clusters);
	void simplifyDouglasPeucker(GeneralizedWay& gw, float epsilon, std::set<int>& ints, int start, int end);
	void removeGeneratedWay(GeneralizedWay& gw, GeneralizedCluster& gcluster);
	void attachWays(GeneralizedWay& gw, bool first);
	void mergeAddTypes(GeneralizedWay& from, GeneralizedWay& to);
	void mergeName(MapRouteType rt, GeneralizedWay& from, GeneralizedWay& to);
	std::unique_ptr<GeneralizedWay> selectBestWay(GeneralizedCluster& cluster, GeneralizedWay& gw, int ind);
	void replacePointWithAnotherPoint(GeneralizedCluster& gcluster, GeneralizedWay& gw, int pxc, int pyc, int i, GeneralizedWay& next);
	void writeBinaryRouteIndex(BinaryMapDataWriter& writer, OBFResultDB& ctx, std::string regionName);
	std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>  writeBinaryRouteIndexHeader(BinaryMapDataWriter& writer,  
			RTreeValued& rte, bool basemap);
	void writeBinaryRouteTree(RTreeValued& parent, RTreeValued::box& re, BinaryMapDataWriter& writer,
			std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& bounds, bool basemap);
	void writeBinaryRouteIndexBlocks(BinaryMapDataWriter& writer, RTreeValued& tree, bool isbase, std::unordered_map<__int64, std::unique_ptr<BinaryFileReference>>& bounds);
	bool compareRefs(GeneralizedWay& gw, GeneralizedWay& gn);
	int registerID(std::vector<__int64> listID, __int64 id);
};

class OBFAddresStreetDB :
	public OBFResultDB
{
public:
	struct cityhash
        : std::unary_function<CityObj, std::size_t>
    {
        cityhash() {}

        std::size_t operator()(CityObj const& xobj) const
        {
            std::size_t seed = 0;
			CityObj& x = const_cast<CityObj&>(xobj);
			boost::hash_combine(seed, x.getName());
			boost::hash_combine(seed, x.getEnName());
			boost::hash_combine(seed, x.getType());
			boost::hash_combine(seed, x.getID());
			boost::hash_combine(seed, x.getLatLon().first);
			boost::hash_combine(seed, x.getLatLon().second);

            return seed;
        }
    };

	struct cityeqal : std::binary_function<CityObj, CityObj, bool>
	{
		
        bool operator()(CityObj const& x1, CityObj const& x2) const
        {
			CityObj& obj1 = const_cast<CityObj&>(x1);
			CityObj& obj2 = const_cast<CityObj&>(x2);
            return obj1.getName() == obj2.getName() && obj1.getID() == obj2.getID();
        }
	};
public:
	TileManager<CityObj> cityManager;
	TileManager<CityObj> townManager;
	std::map<std::shared_ptr<EntityNode>, CityObj> cities;
	CityObj createMissingCity(std::shared_ptr<EntityBase>& cityNode, std::string t);
	std::set<__int64> visitedBoundaryWays;
	std::vector<std::shared_ptr<EntityRelation>> postalCodeRelations;
	std::unordered_map<CityObj, std::shared_ptr<MultiPoly>, cityhash, cityeqal> cityBoundaries;
	std::unordered_map<std::shared_ptr<MultiPoly>, std::list<CityObj>> boundaryToContainingCities;
	std::set<std::shared_ptr<MultiPoly>> boundaries;
	std::set<std::shared_ptr<MultiPoly>> notAssignedBoundaries;
	OBFAddresStreetDB(void);
	virtual ~OBFAddresStreetDB(void);
	std::shared_ptr<MultiPoly> extractBoundary(std::shared_ptr<EntityBase>& baseItem, OBFResultDB& dbContext);
	void indexBoundary(std::shared_ptr<EntityBase>& baseItem, OBFResultDB& dbContext);
	void iterateOverCity(std::shared_ptr<EntityNode>& cityNode);
	void tryToAssignBoundaryToFreeCities();
	void storeCities(OBFResultDB& dbContext);
	void storeCity(std::shared_ptr<EntityNode>& cityNode, CityObj objData, OBFResultDB& dbContext);
	std::shared_ptr<MultiPoly> putCityBoundary(std::shared_ptr<MultiPoly> boundary, CityObj cityFound);
	int getCityBoundaryImportance(std::shared_ptr<MultiPoly> b, CityObj c);
	void indexAddressRelation(std::shared_ptr<EntityRelation>& i, OBFResultDB& dbContext);
	std::unordered_set<long long> getStreetInCity(std::unordered_set<std::string> isInNames, std::string name, std::string nameEn, std::pair<double,double> location, OBFResultDB& dbContext);
	std::string findCityPart(LatLon location, CityObj city);
	std::string findNearestCityOrSuburb(std::shared_ptr<MultiPoly> greatestBoundary, LatLon location);
	void iterateMainEntity(std::shared_ptr<EntityBase>& baseItem, OBFResultDB& dbContext);
	void writeAddresMapIndex(BinaryMapDataWriter& writer, std::string regionName, OBFResultDB& dbContext);
	void putNamedMapObject(std::map<std::string, std::list<std::shared_ptr<MapObject>>>& namesIndex, std::shared_ptr<MapObject> o, __int64 fileOffset);
	void readStreetsAndBuildingsForCity(sqlite3_stmt* streetBuildingsStat, CityObj city,
			sqlite3_stmt* waynodesStat, std::unordered_map<Street, std::list<EntityNode>>& streetNodes, std::unordered_map<__int64, Street>& visitedStreets,
			std::unordered_map<std::string, std::vector<Street>>& uniqueNames);
	double getDistance(Street s, Street c, std::unordered_map<Street, std::list<EntityNode>>& streetNodes);
	std::list<Street> readStreetsBuildings(sqlite3_stmt* streetBuildingsStat, CityObj city, sqlite3_stmt*  waynodesStat,
			std::unordered_map<Street, std::list<EntityNode>>&  streetNodes, std::vector<CityObj> citySuburbs);
	std::vector<EntityNode> loadStreetNodes(__int64 streetId, sqlite3_stmt* waynodesStat);
	void writeCityBlockIndex(BinaryMapDataWriter& writer, std::string citytype, sqlite3_stmt* streetstat, sqlite3_stmt* waynodesStat,
			std::list<CityObj>& suburbs, std::list<CityObj>& cities, std::map<std::string, CityObj>& postcodes, std::map<std::string, std::list<std::shared_ptr<MapObject>>>& namesIndex);
	std::unordered_map<std::string, std::list<CityObj>> readCities(OBFResultDB& dbContext);
};