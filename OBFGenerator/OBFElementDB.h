#pragma once
#include "OBFResultDB.h"
#include "MapRoutingTypes.h"
#include "RTree.h"

class OBFpoiDB :
	public OBFResultDB
{
	OBFRenderingTypes renderer;
	std::map<long long, boost::unordered_map<std::string, std::string>> propagatedTags;
	std::list<Amenity> tempAmenityList;
public:
	OBFpoiDB(void);
	virtual ~OBFpoiDB(void);
	void indexRelations(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext);
	void iterateMainEntity(std::shared_ptr<EntityBase>& relItem, OBFResultDB& dbContext);
	void insertAmenityIntoPoi(Amenity amenity, OBFResultDB& dbContext);
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
private:
	static long SHIFT_INSERT_AT;
	static long SHIFT_ORIGINAL;
	static long SHIFT_ID;

	static long getBaseId(int x31, int y31) {
		__int64 x = x31;
		__int64 y = y31;
		return (x << 31) + y;
	}
	 class GeneralizedWay {
	 public:
		 __int64 id;
		 int mainType;
		boost::unordered_set<int> addtypes;
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
		typedef boost::unordered_map<__int64, boost::container::list<GeneralizedWay>>::iterator mapIt;
		boost::unordered_map<__int64, boost::container::list<GeneralizedWay>> map;
		
		
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
	RTree routeTree;
	RTree baserouteTree;
	OBFrouteDB(void);
	virtual ~OBFrouteDB(void);
	void indexHighwayRestrictions(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext);
	void indexRelations(std::shared_ptr<EntityRelation> entry, OBFResultDB& dbContext);
	void iterateMainEntity(std::shared_ptr<EntityBase>& item, OBFResultDB& dbContext);
	void processLowLevelWays(OBFResultDB& dbContext);
	void addWayToIndex(long long id, std::vector<std::shared_ptr<EntityNode>>& nodes, OBFResultDB& dbContext, RTree rTree,  bool base);
	void registerBaseIntersectionPoint(long long pointLoc, bool registerId, long long wayId, int insertAt, int originalInd);
	std::string encodeNames(std::map<MapRouteType, std::string> tempNames);
	void putIntersection(long long  point, long long wayNodeId);
	boost::unordered_map<__int64, std::list<__int64>> highwayRestrictions;
	boost::unordered_map<__int64, __int64> basemapRemovedNodes;
	boost::unordered_map<__int64, RouteMissingPoints> basemapNodesToReinsert;
	std::map<long long, boost::unordered_map<std::string, std::string>> propagatedTags;
	OBFRenderingTypes renderer;
	MapRoutingTypes routingTypes;

};

class OBFAddresStreetDB :
	public OBFResultDB
{

public:
	TileManager<CityObj> cityManager;
	TileManager<CityObj> townManager;
	std::map<std::shared_ptr<EntityNode>, CityObj> cities;

	std::set<__int64> visitedBoundaryWays;
	std::set<std::shared_ptr<MultiPoly>> boundaries;
	OBFAddresStreetDB(void);
	virtual ~OBFAddresStreetDB(void);
	void indexBoundary(std::shared_ptr<EntityBase>& baseItem, OBFResultDB& dbContext);
	void iterateOverCity(std::shared_ptr<EntityNode>& cityNode);
	void storeCities(OBFResultDB& dbContext);
};