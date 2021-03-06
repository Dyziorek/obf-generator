#pragma once

#include "MapObject.h"
#include <boost\lexical_cast.hpp>

static char intToBase64[] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
        'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
        'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '_', '@'
    };

class MapUtils
{
public:
	MapUtils(void);
	~MapUtils(void);
	static double getPowZoom(float zoom);


	static bool fuzzyCompare(double p1, double p2);
	
	static bool fuzzyCompare(float p1, float p2);
		
	static double scalarMultiplication(double xA, double yA, double xB, double yB, double xC, double yC);

	 static double getOrthogonalDistance(double lat, double lon, double fromLat, double fromLon, double toLat, double toLon);
	
	 static std::pair<double, double> getProjection(double lat, double lon, double fromLat, double fromLon, double toLat, double toLon);
	 static double getProjectionLat(double lat, double lon, double fromLat, double fromLon, double toLat, double toLon);
	
	 static double getProjectionLon(double lat, double lon, double fromLat, double fromLon, double toLat, double toLon);

	 static double toRadians(double angdeg);
	
	/**
	 * Gets distance in meters
	 */
	 static double getDistance(double lat1, double lon1, double lat2, double lon2);
	
	
		
	 static double checkLongitude(double longitude);
	
	 static double checkLatitude(double latitude);
	
	 static int get31TileNumberX(double longitude);
	 static int get31TileNumberY( double latitude);
	
	 static double get31LongitudeX(int tileX);
	
	 static double get31LatitudeY(int tileY);
	
	
	
	 static double getTileNumberX(float zoom, double longitude);
	
	 static double getTileNumberY(float zoom,  double latitude);
	
	 static double getTileEllipsoidNumberY(float zoom, double latitude);
	
	 static double getLatitudeFromEllipsoidTileY(float zoom, float tileNumberY);
	
	
	 static double getLongitudeFromTile(float zoom, double x);
	
	 //static double getPowZoom(float zoom);
	

	
	 static float calcDiffPixelX(float rotateSin, float rotateCos, float dTileX, float dTileY, float tileSize);
	
	 static float calcDiffPixelY(float rotateSin, float rotateCos, float dTileX, float dTileY, float tileSize);
	
	 static double getLatitudeFromTile(float zoom, double y);
	
	
	 static int getPixelShiftX(int zoom, double long1, double long2, int tileSize);
	
	
	 static int getPixelShiftY(int zoom, double lat1, double lat2, int tileSize);
	
	
	
	 static void sortListOfMapObject(std::vector<MapObject> list,  double lat,  double lon);
	
	
	// Examples
//	System.out.println(buildShortOsmUrl(51.51829d, 0.07347d, 16)); // http://osm.org/go/0EEQsyfu
//	System.out.println(buildShortOsmUrl(52.30103d, 4.862927d, 18)); // http://osm.org/go/0E4_JiVhs
//	System.out.println(buildShortOsmUrl(40.59d, -115.213d, 9)); // http://osm.org/go/TelHTB--
	 static std::string buildShortOsmUrl(double latitude, double longitude, int zoom);

	 static std::string createShortLocString(double latitude, double longitude, int zoom);
	
	 static std::pair<double, double> decodeShortLocString(std::string s) ;
	
	/**	
	 * interleaves the bits of two 32-bit numbers. the result is known as a Morton code.	   
	 */
	 static long interleaveBits(long x, long y);

	/**
	 * Calculate rotation diff D, that R (rotate) + D = T (targetRotate)
	 * D is between -180, 180 
	 * @param rotate
	 * @param targetRotate
	 * @return 
	 */
	 static float unifyRotationDiff(float rotate, float targetRotate) ;
	
	/**
	 * Calculate rotation diff D, that R (rotate) + D = T (targetRotate)
	 * D is between -180, 180 
	 * @param rotate
	 * @return
	 */
	 static float unifyRotationTo360(float rotate) ;

	/**
	 * @param diff align difference between 2 angles ]-PI, PI] 
	 * @return 
	 */
	 static double alignAngleDifference(double diff) ;
	
	/**
	 * diff align difference between 2 angles ]-180, 180]
	 * @return 
	 */
	 static double degreesDiff(double a1, double a2) ;	

	
	 static double convert31YToMeters(float y1, float y2) ;
	
	 static double convert31XToMeters(float x1, float x2) ;
   
	
	 static std::pair<double, double> getProjectionPoint31(int px, int py, int st31x, int st31y,int end31x, int end31y) ;
	
	
	 static double squareRootDist31(int x1, int y1, int x2, int y2) ;
	
	 static double measuredDist31(int x1, int y1, int x2, int y2) ;
	
	 static double squareDist31TileMetric(int x1, int y1, int x2, int y2) ;
	
	 static double calculateProjection31TileMetric(int xA, int yA, int xB, int yB, int xC, int yC) ;


};



class OsmMapUtils {
public:
	static double getDistance(EntityNode e1, EntityNode e2);

	 static double getDistance(EntityNode e1, double latitude, double longitude);

	 static double getDistance(EntityNode e1, std::pair<double, double>  point);

	 static std::pair<double, double>  getCenter(EntityBase* e);

	 static std::pair<double, double>  getWeightCenter(std::vector<std::pair<double, double> > points);

	 static std::pair<double, double>  getWeightCenterForNodes(std::vector<std::shared_ptr<EntityNode>> EntityNodes);

	 static std::pair<double, double>  getMathWeightCenterForEntityNodes(std::vector<std::shared_ptr<EntityNode>> EntityNodes);

	 static void sortListOfEntities(std::list<std::shared_ptr<EntityBase>> list,  double lat,  double lon);

	 static void addIdsToList(std::list<std::shared_ptr<EntityBase>> source, std::list<long long> ids);

     static bool ccw(EntityNode A, EntityNode B, EntityNode C);

    // Return true if line segments AB and CD intersect
     static bool intersect2Segments(EntityNode A, EntityNode B, EntityNode C, EntityNode D);

	

	

	 static bool isClockwiseWay(std::shared_ptr<EntityWay> w);

	 static bool isClockwiseWay(std::vector<std::shared_ptr<EntityWay>> ways);

	// try to intersect from left to right
	 static double ray_intersect_lon(std::shared_ptr<EntityNode> EntityNode1, std::shared_ptr<EntityNode> EntityNode2, double latitude, double longitude);

    /**
     * Get the area in pixels
     * @param EntityNodes
     * @return
     */
     static double polygonAreaPixels(std::list<std::shared_ptr<EntityNode>> EntityNodes, int zoom);
	/**
	 * Get the area (in m²) of a closed way, represented as a list of EntityNodes
	 * 
	 * @param EntityNodes
	 *            the list of EntityNodes
	 * @return the area of it
	 */
	 static double getArea(std::vector<std::shared_ptr<EntityNode>> nodesVec);
	 static std::vector<bool> simplifyDouglasPeucker(std::vector<std::shared_ptr<EntityNode>>& n, int zoom, int epsilon, std::vector<std::shared_ptr<EntityNode>>& result, bool avoidNooses);

 	static bool checkForSmallAreas(std::vector<std::shared_ptr<EntityNode>> nodes, int zoom, int minz, int maxz);
	static std::list<std::shared_ptr<EntityNode>> simplifyCycleWay(std::vector<std::shared_ptr<EntityNode>>& ns, int zoom, int zoomWaySmothness);

private:
	
	static void simplifyDouglasPeucker(std::vector<std::shared_ptr<EntityNode>>& n, int zoom, int epsilon, std::vector<bool>& kept,
                                               int start, int end, bool avoidNooses);
	static double orthogonalDistance(int zoom,  std::shared_ptr<EntityNode> EntityNodeLineStart,  std::shared_ptr<EntityNode> EntityNodeLineEnd,  std::shared_ptr<EntityNode> RelEntityNode);
};


class MapZooms {
public:	
	class MapZoomPair {
	public:
		static int MAX_ALLOWED_ZOOM;
		MapZoomPair(int pminZoom, int pmaxZoom) {
			maxZoom = pmaxZoom;
			minZoom = pminZoom;
		}

		int getMinZoom() {
			return minZoom;
		}
		
		 int getMaxZoom() {
			return maxZoom;
		}
		
		std::string toString() {
			return std::string("MapZoomPair : ") + boost::lexical_cast<std::string>(minZoom) + " - " + boost::lexical_cast<std::string>(maxZoom);
		}

		bool operator<(const MapZoomPair& op2) const
		{
			return maxZoom < op2.maxZoom;
		}
	private:
		int minZoom;
		int maxZoom;
		
	};
	
private:
	std::list<MapZoomPair> levels;
	static MapZooms* DEFAULT;	
public:
	std::list<MapZoomPair> getLevels() {
		return levels;
	}
	
	void setLevels(std::list<MapZoomPair> parlevels) {
		levels = parlevels;
		levels.sort();
		levels.reverse();
	}
	/**
	 * @param zooms - could be 5-8;7-10;11-14;15-
	 */
	static MapZooms* parseZooms(std::string zooms);
	
	int size(){
		return levels.size();
	}
	
	MapZoomPair getLevel(int level){
		std::list<MapZoomPair>::iterator itLevel = levels.begin();
		std::advance(itLevel, level);
		return *itLevel;
	}
	

	static std::string MAP_ZOOMS_DEFAULT;
	static MapZooms* getDefault();
};
