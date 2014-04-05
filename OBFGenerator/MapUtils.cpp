#include "stdafx.h"
#include "MapUtils.h"


#define LOCM_PI       3.14159265358979323846

MapUtils::MapUtils(void)
{
}


MapUtils::~MapUtils(void)
{
}



	
	  double MapUtils::scalarMultiplication(double xA, double yA, double xB, double yB, double xC, double yC) {
		// Scalar multiplication between (AB, AC)
		return (xB - xA) * (xC - xA) + (yB- yA) * (yC -yA);
	}

	  double MapUtils::getOrthogonalDistance(double lat, double lon, double fromLat, double fromLon, double toLat, double toLon) {
		return getDistance(getProjectionLat(lat, lon, fromLat, fromLon, toLat, toLon), getProjectionLon(lat, lon, fromLat, fromLon, toLat, toLon) , lat, lon);
	}
	
	  double MapUtils::getProjectionLat(double lat, double lon, double fromLat, double fromLon, double toLat, double toLon) {
		// not very accurate computation on sphere but for distances < 1000m it is ok
		double mDist = (fromLat - toLat) * (fromLat - toLat) + (fromLon - toLon) * (fromLon - toLon);
		double projection = scalarMultiplication(fromLat, fromLon, toLat, toLon, lat, lon);
		double prlat;
		double prlon;
		if (projection < 0) {
			prlat = fromLat;
			prlon = fromLon;
		} else if (projection >= mDist) {
			prlat = toLat;
			prlon = toLon;
		} else {
			prlat = fromLat + (toLat - fromLat) * (projection / mDist);
			prlon = fromLon + (toLon - fromLon) * (projection / mDist);
		}
		return prlat;
	}
	
	  double MapUtils::getProjectionLon(double lat, double lon, double fromLat, double fromLon, double toLat, double toLon) {
		// not very accurate computation on sphere but for distances < 1000m it is ok
		double mDist = (fromLat - toLat) * (fromLat - toLat) + (fromLon - toLon) * (fromLon - toLon);
		double projection = scalarMultiplication(fromLat, fromLon, toLat, toLon, lat, lon);
		double prlat;
		double prlon;
		if (projection < 0) {
			prlat = fromLat;
			prlon = fromLon;
		} else if (projection >= mDist) {
			prlat = toLat;
			prlon = toLon;
		} else {
			prlat = fromLat + (toLat - fromLat) * (projection / mDist);
			prlon = fromLon + (toLon - fromLon) * (projection / mDist);
		}
		return prlon;
	}

	  std::pair<double, double>  MapUtils::getProjection(double lat, double lon, double fromLat, double fromLon, double toLat, double toLon)
	  {
		  return std::make_pair(getProjectionLat(lat, lon,fromLat, fromLon, toLat,toLon),getProjectionLon(lat, lon,fromLat, fromLon, toLat,toLon));
	  }

	  double MapUtils::toRadians(double angdeg) {
//		return toRadians(angdeg);
		return angdeg / 180.0 * 3.14159265358979323846;
	}
	
	/**
	 * Gets distance in meters
	 */
	  double MapUtils::getDistance(double lat1, double lon1, double lat2, double lon2){
		double R = 6372.8; // for haversine use R = 6372.8 km instead of 6371 km
		double dLat = toRadians(lat2-lat1);
		double dLon = toRadians(lon2-lon1); 
		double a = sin(dLat/2) * sin(dLat/2) +
		        cos(toRadians(lat1)) * cos(toRadians(lat2)) * 
		        sin(dLon/2) * sin(dLon/2); 
		//double c = 2 * atan2(sqrt(a), sqrt(1-a));
		//return R * c * 1000;
		// simplyfy haversine:
		return (2 * R * 1000 * asin(sqrt(a)));
	}
	
	
		
	  double MapUtils::checkLongitude(double longitude) {
		while (longitude < -180 || longitude > 180) {
			if (longitude < 0) {
				longitude += 360;
			} else {
				longitude -= 360;
			}
		}
		return longitude;
	}
	
	  double MapUtils::checkLatitude(double latitude) {
		while (latitude < -90 || latitude > 90) {
			if (latitude < 0) {
				latitude += 180;
			} else {
				latitude -= 180;
			}
		}
		if(latitude < -85.0511) {
			return -85.0511;
 		} else if(latitude > 85.0511){
 			return 85.0511;
 		}
		return latitude;
	}
	
	  int MapUtils::get31TileNumberX(double longitude){
		longitude = checkLongitude(longitude);
		long l = 1L << 31;
		return (int)((longitude + 180)/360 * l);
	}
	  int MapUtils::get31TileNumberY( double latitude){
		latitude = checkLatitude(latitude);
		double eval = log( tan(toRadians(latitude)) + 1/cos(toRadians(latitude)) );
		long l = 1L << 31;
		if(eval > LOCM_PI){
			eval = LOCM_PI;
		}
		return  (int) ((1 - eval / LOCM_PI) / 2 * l);
	}
	
	  double MapUtils::get31LongitudeX(int tileX){
		return getLongitudeFromTile(21, tileX /1024);
	}
	
	  double MapUtils::get31LatitudeY(int tileY){
		return getLatitudeFromTile(21, tileY /1024);
	}
	
	
	
	/**
	 * 
	 * Theses methods operate with degrees (evaluating tiles & vice versa) 
	 * degree longitude measurements (-180, 180) [27.56 Minsk]
	// degree latitude measurements (90, -90) [53.9]
	 */
	
	  double MapUtils::getTileNumberX(float zoom, double longitude){
		if(longitude == 180){
			return getPowZoom(zoom) - 1;
		}
		longitude = checkLongitude(longitude);
		return (longitude + 180)/360 * getPowZoom(zoom);
	}
	
	  double MapUtils::getTileNumberY(float zoom,  double latitude){
		latitude = checkLatitude(latitude);
		double eval = log( tan(toRadians(latitude)) + 1/cos(toRadians(latitude)) );
		if (!_finite(eval)) {
			latitude = latitude < 0 ? - 89.9 : 89.9;
			eval = log( tan(toRadians(latitude)) + 1/cos(toRadians(latitude)) );
		}
		return (1 - eval / LOCM_PI) / 2 * getPowZoom(zoom);
	}
	
	  double MapUtils::getTileEllipsoidNumberY(float zoom, double latitude){
		double E2 = (double) latitude * LOCM_PI / 180;
		long sradiusa = 6378137;
		long sradiusb = 6356752;
		double J2 = (double) sqrt(sradiusa * sradiusa - sradiusb * sradiusb)	/ sradiusa;
		double M2 = (double) log((1 + sin(E2))
				/ (1 - sin(E2)))/ 2- J2	* log((1 + J2 * sin(E2))/ (1 - J2 * sin(E2))) / 2;
		double B2 = getPowZoom(zoom);
		return B2 / 2 - M2 * B2 / 2 / LOCM_PI;
	}
	
	  double MapUtils::getLatitudeFromEllipsoidTileY(float zoom, float tileNumberY){
		double MerkElipsK = 0.0000001;
		long sradiusa = 6378137;
		long sradiusb = 6356752;
		double FExct = (double) sqrt(sradiusa * sradiusa
				- sradiusb * sradiusb)
				/ sradiusa;
		double TilesAtZoom = getPowZoom(zoom);
		double result = (tileNumberY - TilesAtZoom / 2)
				/ -(TilesAtZoom / (2 * LOCM_PI));
		result = (2 * atan(exp(result)) - LOCM_PI / 2) * 180
				/ LOCM_PI;
		double Zu = result / (180 / LOCM_PI);
		double yy = (tileNumberY - TilesAtZoom / 2);

		double Zum1 = Zu;
		Zu = asin(1 - ((1 + sin(Zum1)) * pow(1 - FExct * sin(Zum1), FExct))
				/ (exp((2 * yy) / -(TilesAtZoom / (2 * LOCM_PI))) * pow(1 + FExct * sin(Zum1), FExct)));
		while (abs(Zum1 - Zu) >= MerkElipsK) {
			Zum1 = Zu;
			Zu = asin(1 - ((1 + sin(Zum1)) * pow(1 - FExct * sin(Zum1), FExct))
					/ (exp((2 * yy) / -(TilesAtZoom / (2 * LOCM_PI))) * pow(1 + FExct * sin(Zum1), FExct)));
		}

		return Zu * 180 / LOCM_PI;
	}
	
	
	  double MapUtils::getLongitudeFromTile(float zoom, double x) {
		return x / getPowZoom(zoom) * 360.0 - 180.0;
	}
	
	  double MapUtils::getPowZoom(float zoom){
		if(zoom >= 0 && zoom - floor(zoom) < 0.001f){
			return 1 << ((int)zoom); 
		} else {
			return pow(2, zoom);
		}
	}
	

	
	  float MapUtils::calcDiffPixelX(float rotateSin, float rotateCos, float dTileX, float dTileY, float tileSize){
		return (rotateCos * dTileX - rotateSin * dTileY) * tileSize ;
	}
	
	  float MapUtils::calcDiffPixelY(float rotateSin, float rotateCos, float dTileX, float dTileY, float tileSize){
		return (rotateSin * dTileX + rotateCos * dTileY) * tileSize ;
	}
	
	  double MapUtils::getLatitudeFromTile(float zoom, double y){
		int sign = y < 0 ? -1 : 1;
		return atan(sign*sinh(LOCM_PI * (1 - 2 * y / getPowZoom(zoom)))) * 180 / LOCM_PI;
	}
	
	
	  int MapUtils::getPixelShiftX(int zoom, double long1, double long2, int tileSize){
		return (int) ((getTileNumberX(zoom, long1) - getTileNumberX(zoom, long2)) * tileSize);
	}
	
	
	  int MapUtils::getPixelShiftY(int zoom, double lat1, double lat2, int tileSize){
		return (int) ((getTileNumberY(zoom, lat1) - getTileNumberY(zoom, lat2)) * tileSize);
	}
	
	
	
	  void MapUtils::sortListOfMapObject(std::vector<MapObject> vecObj, double lat, double lon){
		 std::sort(vecObj.begin(), vecObj.end(), [&](MapObject o1, MapObject o2)
		  {
			  return MapUtils::getDistance(o1.getLatLon().first, o1.getLatLon().second, lat, lon) < MapUtils::getDistance(o2.getLatLon().first, o2.getLatLon().second,
						lat, lon);
		  });
	}
	
	
	// Examples
//	System.out.println(buildShortOsmUrl(51.51829d, 0.07347d, 16)); // http://osm.org/go/0EEQsyfu
//	System.out.println(buildShortOsmUrl(52.30103d, 4.862927d, 18)); // http://osm.org/go/0E4_JiVhs
//	System.out.println(buildShortOsmUrl(40.59d, -115.213d, 9)); // http://osm.org/go/TelHTB--
	  std::string MapUtils::buildShortOsmUrl(double latitude, double longitude, int zoom){
        return "http://net.osmand.com" + createShortLocString(latitude, longitude, zoom) + "?m";
	}

	  std::string MapUtils::createShortLocString(double latitude, double longitude, int zoom) {
		long lat = (long) (((latitude + 90)/180)*(1L << 32));
		long lon = (long) (((longitude + 180)/360)*(1L << 32));
		long code = interleaveBits(lon, lat);
		std::string str = "";
	    // add eight to the zoom level, which approximates an accuracy of one pixel in a tile.
		for (int i = 0; i < ceil((zoom + 8) / 3); i++) {
		    str += intToBase64[(int) ((code >> (58 - 6 * i)) & 0x3f)];
		}
		// append characters onto the end of the string to represent
		// partial zoom levels (characters themselves have a granularity of 3 zoom levels).
		for (int j = 0; j < (zoom + 8) % 3; j++) {
			str += '-';
		}
		return str;
	}
	
	  std::pair<double, double> MapUtils::decodeShortLocString(std::string s) {
		long x = 0;
		long y = 0;
	    int z = 0;
		int z_offset = 0;

		for (int i = 0; i < s.length(); i++) {
			if (s.at(i) == '-') {
				z_offset--;
				continue;
			}
			char c = s.at(i);
			for (int j = 0; j < sizeof(intToBase64); j++) {
				if (c == intToBase64[j]) {
					for (int k = 0; k < 3; k++) {
						x <<= 1;
						if ((j & 32) != 0) {
							x = x | 1;
						}
						j <<= 1;
						y <<= 1;
						if ((j & 32) != 0) {
							y = y | 1;
						}
						j <<= 1;
					}
					z += 3;
					break;
				}
			}
		}
		x <<= (32 - z);
		y <<= (32 - z);
//		int zoom = z - 8 - ((3 + z_offset) % 3);
		double dlat = (180 * (y) / ((double)(1L << 32))) - 90;
		double dlon = (360 * (x)/ ((double)(1L << 32))) - 180;
		return std::make_pair(dlat, dlon);
	}
	
	/**	
	 * interleaves the bits of two 32-bit numbers. the result is known as a Morton code.	   
	 */
	  long  MapUtils::interleaveBits(long x, long y){
		long c = 0;
		for(byte b = 31; b>=0; b--){
			c = (c << 1) | ((x >> b) & 1);
			c = (c << 1) | ((y >> b) & 1);
		}
		return c;
	}

	/**
	 * Calculate rotation diff D, that R (rotate) + D = T (targetRotate)
	 * D is between -180, 180 
	 * @param rotate
	 * @param targetRotate
	 * @return 
	 */
	  float  MapUtils::unifyRotationDiff(float rotate, float targetRotate) {
		float d = targetRotate - rotate;
		while(d >= 180){
			d -= 360;
		}
		while(d < -180){
			d += 360;
		}
		return d;
	}
	
	/**
	 * Calculate rotation diff D, that R (rotate) + D = T (targetRotate)
	 * D is between -180, 180 
	 * @param rotate
	 * @return
	 */
	  float  MapUtils::unifyRotationTo360(float rotate) {
		while(rotate < -180){
			rotate += 360;
		}
		while(rotate > +180){
			rotate -= 360;
		}
		return rotate;
	}

	/**
	 * @param diff align difference between 2 angles ]-PI, PI] 
	 * @return 
	 */
	  double MapUtils::alignAngleDifference(double diff) {
		while(diff > LOCM_PI) {
			diff -= 2 * LOCM_PI;
		}
		while(diff <=-LOCM_PI) {
			diff += 2 * LOCM_PI;
		}
		return diff;
		
	}
	
	/**
	 * diff align difference between 2 angles ]-180, 180]
	 * @return 
	 */
	  double MapUtils::degreesDiff(double a1, double a2) {
		double diff = a1 - a2;
		while(diff > 180) {
			diff -= 360;
		}
		while(diff <=-180) {
			diff += 360;
		}
		return diff;
		
	}	

	
	  double MapUtils::convert31YToMeters(float y1, float y2) {
		// translate into meters 
		return (y1 - y2) * 0.01863;
	}
	
	  double MapUtils::convert31XToMeters(float x1, float x2) {
		// translate into meters 
		return (x1 - x2) * 0.011;
	}
   
	
	  std::pair<double, double> MapUtils::getProjectionPoint31(int px, int py, int st31x, int st31y,int end31x, int end31y) {
		double projection = calculateProjection31TileMetric(st31x, st31y, end31x,
				end31y, px, py);
		double mDist = squareRootDist31(end31x, end31y, st31x,
				st31y);
		int pry = end31y;
		int prx = end31x;
		if (projection < 0) {
			prx = st31x;
			pry = st31y;
		} else if (projection >= mDist * mDist) {
			prx = end31x;
			pry = end31y;
		} else {
			prx = (int) (st31x + (end31x - st31x)
					* (projection / (mDist * mDist)));
			pry = (int) (st31y + (end31y - st31y)
					* (projection / (mDist * mDist)));
		}
		return std::make_pair(prx, pry);
	}
	
	
	  double MapUtils::squareRootDist31(int x1, int y1, int x2, int y2) {
		// translate into meters 
		double dy = MapUtils::convert31YToMeters(y1, y2);
		double dx = MapUtils::convert31XToMeters(x1, x2);
		return sqrt(dx * dx + dy * dy);
//		return measuredDist(x1, y1, x2, y2);
	}
	
	  double MapUtils::measuredDist31(int x1, int y1, int x2, int y2) {
		return getDistance(MapUtils::get31LatitudeY(y1), MapUtils::get31LongitudeX(x1), MapUtils::get31LatitudeY(y2), MapUtils::get31LongitudeX(x2));
	}
	
	  double MapUtils::squareDist31TileMetric(int x1, int y1, int x2, int y2) {
		// translate into meters 
		double dy = convert31YToMeters(y1, y2);
		double dx = convert31XToMeters(x1, x2);
		return dx * dx + dy * dy;
	}
	
	  double MapUtils::calculateProjection31TileMetric(int xA, int yA, int xB, int yB, int xC, int yC) {
		// Scalar multiplication between (AB, AC)
		double multiple = MapUtils::convert31XToMeters(xB, xA) * MapUtils::convert31XToMeters(xC, xA) +
				MapUtils::convert31YToMeters(yB, yA) * MapUtils::convert31YToMeters(yC, yA);
		return multiple;
	}

	
	  
	 double OsmMapUtils::getDistance(EntityNode e1, EntityNode e2) {
		return MapUtils::getDistance(e1.lat, e1.lon, e2.lat, e2.lon);
	}

	 double OsmMapUtils::getDistance(EntityNode e1, double latitude, double longitude) {
		return MapUtils::getDistance(e1.lat, e1.lon, latitude, longitude);
	}

	 double OsmMapUtils::getDistance(EntityNode e1, std::pair<double, double> point) {
		 return MapUtils::getDistance(e1.lat, e1.lon, point.first, point.second);
	}

	 std::pair<double, double> OsmMapUtils::getCenter(EntityBase* e) {

		 if (dynamic_cast<EntityNode*>(e) != NULL ) {
			 EntityNode* ne = dynamic_cast<EntityNode*>(e);
			return ne->getLatLon();
		} else if (dynamic_cast<EntityWay*>(e) != NULL) {
			EntityWay* we = dynamic_cast<EntityWay*>(e);
			return getWeightCenterForNodes(we->nodes);
		} else if (dynamic_cast<EntityRelation*>(e) != NULL) {
			EntityRelation* re = dynamic_cast<EntityRelation*>(e);
			std::vector<std::pair<double, double>> list;
			for (std::shared_ptr<EntityBase> fe : re->getRelations() ) {
				std::pair<double, double> c = std::make_pair(-1000,-1000);
				// skip relations to avoid circular dependencies
				if ((dynamic_cast<EntityRelation*>(fe.get()) == NULL)) {
					c = getCenter(fe.get());
				}
				if (c.first != -1000) {
					list.push_back(c);
				}
			}
			return getWeightCenter(list);
		}
		return std::make_pair(-1000,-1000);
	}

	 std::pair<double, double> OsmMapUtils::getWeightCenter(std::vector<std::pair<double, double>> nodes) {
		if (nodes.empty()) {
			return std::make_pair(-1000,-1000);
		}
		double longitude = 0;
		double latitude = 0;
		for (std::pair<double, double> n : nodes) {
			longitude += n.first;
			latitude += n.second;
		}
		return std::make_pair(latitude / nodes.size(), longitude / nodes.size());
	}

	 std::pair<double, double> OsmMapUtils::getWeightCenterForNodes(std::vector<std::shared_ptr<EntityNode>> nodes) {
		if (nodes.empty()) {
			return std::make_pair(-1000,-1000);
		}
		double longitude = 0;
		double latitude = 0;
		int count = 0;
		for (std::shared_ptr<EntityNode> n : nodes) {
				count++;
				longitude += n->lon;
				latitude += n->lat;
		}
		if (count == 0) {
			 return std::make_pair(-1000,-1000);;
		}
		return std::make_pair(latitude / count, longitude / count);
	}

	 std::pair<double, double> OsmMapUtils::getMathWeightCenterForEntityNodes(std::vector<std::shared_ptr<EntityNode>> nodes) {
		if (nodes.empty()) {
			return std::make_pair(-1000,-1000);
		}
		double longitude = 0;
		double latitude = 0;
		double sumDist = 0;
		std::shared_ptr<EntityNode> prev;
		for (std::shared_ptr<EntityNode> n : nodes) {
			
				if (prev->isEmpty()) {
					prev = n;
				} else {
					double dist = getDistance(*prev, *n);
					sumDist += dist;
					longitude += (prev->lon + n->lon) * dist / 2;
					latitude += (n->lat + n->lat) * dist / 2;
					prev = n;
				}
		}
		if (sumDist == 0) {
			return prev->getLatLon();
		}
		return std::make_pair(latitude / sumDist, longitude / sumDist);
	}

	 void OsmMapUtils::sortListOfEntities(std::list<std::shared_ptr<EntityBase>> list, double lat, double lon) {
		 list.sort([lat, lon] (std::shared_ptr<EntityBase> e1, std::shared_ptr<EntityBase> e2){
			 return MapUtils::getDistance(e1->getLatLon().first, e1->getLatLon().second, lat, lon) <  MapUtils::getDistance(e2->getLatLon().first, e2->getLatLon().second, lat, lon);
		 });
	}

	 void OsmMapUtils::addIdsToList(std::list<std::shared_ptr<EntityBase>> source, std::list<long long> ids) {
		for (std::shared_ptr<EntityBase> e : source) {
			ids.push_back(e->id);
		}
	}

     boolean OsmMapUtils::ccw(EntityNode A, EntityNode B, EntityNode C) {
        return (C.lat-A.lat) * (B.lon-A.lon) > (B.lat-A.lat) *
                (C.lon-A.lon);
    }

    // Return true if line segments AB and CD intersect
     boolean OsmMapUtils::intersect2Segments(EntityNode A, EntityNode B, EntityNode C, EntityNode D) {
        return ccw(A, C, D) != ccw(B, C, D) && ccw(A, B, C) != ccw(A, B, D);
    }

	 std::vector<boolean> OsmMapUtils::simplifyDouglasPeucker(std::vector<std::shared_ptr<EntityNode>> n, int zoom, int epsilon, std::vector<std::shared_ptr<EntityNode>> result, boolean avoidNooses) {
		if (zoom > 31) {
			zoom = 31;
		}
		std::vector<boolean> kept(n.size());
		int first = 0;
		int nsize = n.size();
		while (first < nsize) {
			if (n[first].get() != nullptr) {
				break;
			}
			first++;
		}
		int last = nsize - 1;
		while (last >= 0) {
			if (n[last].get() != nullptr) {
				break;
			}
			last--;
		}
		if (last - first < 1) {
			return kept;
		}
		// check for possible cycle
		boolean checkCycle = true;
		boolean cycle = false;
		while (checkCycle && last > first) {
			checkCycle = false;

			double x1 = MapUtils::getTileNumberX(zoom, n[first]->lon);
			double y1 = MapUtils::getTileNumberY(zoom, n[first]->lat);
			double x2 = MapUtils::getTileNumberX(zoom, n[last]->lon);
			double y2 = MapUtils::getTileNumberY(zoom, n[last]->lat);
			if (abs(x1 - x2) + abs(y1 - y2) < 0.001) {
				last--;
				cycle = true;
				checkCycle = true;
			}
		}
		if (last - first < 1) {
			return kept;
		}
		simplifyDouglasPeucker(n, zoom, epsilon, kept, first, last, avoidNooses);
		result.push_back(n[first]);
		for (int i = 0; i < kept.size(); i++) {
			if(kept[i]) {
				result.push_back(n[i]);
			}
		}
		if (cycle) {
			result.push_back(n[first]);
		}
		kept[first] = true;
		
		return kept;
	}

	 void OsmMapUtils::simplifyDouglasPeucker(std::vector<std::shared_ptr<EntityNode>> n, int zoom, int epsilon, std::vector<boolean> kept,
                                               int start, int end, boolean avoidNooses) {
		double dmax = -1;
		int index = -1;
		for (int i = start + 1; i <= end - 1; i++) {
			if (n[i].get() == nullptr) {
				continue;
			}
			double d = orthogonalDistance(zoom, n[start], n[end], n[i]);// calculate distance from line
			if (d > dmax) {
				dmax = d;
				index = i;
			}
		}
        boolean nooseFound = false;
        if(avoidNooses && index >= 0) {
            std::shared_ptr<EntityNode> st = n[start];
            std::shared_ptr<EntityNode> e = n[end];
            for(int i = 0; i < n.size() - 1; i++) {
                if(i == start - 1) {
                    i = end;
                    continue;
                }
                std::shared_ptr<EntityNode> np = n.at(i);
                std::shared_ptr<EntityNode> np2 = n.at(i + 1);
                if(np == NULL || np2 == NULL) {
                    continue;
                }
                if (OsmMapUtils::intersect2Segments(*st, *e, *np, *np2)) {
                    nooseFound = true;
                    break;
                }
            }
        }
		if (dmax >= epsilon || nooseFound ) {
			simplifyDouglasPeucker(n, zoom, epsilon, kept, start, index, avoidNooses);
			simplifyDouglasPeucker(n, zoom, epsilon, kept, index, end, avoidNooses);
		} else {
			kept[end] = true;
		}
	}

	 double OsmMapUtils::orthogonalDistance(int zoom, std::shared_ptr<EntityNode> EntityNodeLineStart, std::shared_ptr<EntityNode> EntityNodeLineEnd, std::shared_ptr<EntityNode> RetEntityNode) {
		std::pair<double, double> p = MapUtils::getProjection(RetEntityNode->lat, RetEntityNode->lon, EntityNodeLineStart->lat,
				EntityNodeLineStart->lon, EntityNodeLineEnd->lat, EntityNodeLineEnd->lon);

		double x1 = MapUtils::getTileNumberX(zoom, p.second);
		double y1 = MapUtils::getTileNumberY(zoom, p.first);
		double x2 = MapUtils::getTileNumberX(zoom, RetEntityNode->lon);
		double y2 = MapUtils::getTileNumberY(zoom, RetEntityNode->lat);
		double C = x2 - x1;
		double D = y2 - y1;
		return sqrt(C * C + D * D);
	}

	 BOOL OsmMapUtils::isClockwiseWay(std::shared_ptr<EntityWay> w) {
		 std::vector<std::shared_ptr<EntityWay>> ways;
		 ways.push_back(w);
		return isClockwiseWay(ways);
	}

	 BOOL OsmMapUtils::isClockwiseWay(std::vector<std::shared_ptr<EntityWay>> ways) {
		if (ways.empty()) {
			return true;
		}
		std::pair<double, double> pos = ways[0]->getLatLon();
		double lat = pos.first;
		double lon = 180;
		double firstLon = -360;
		boolean firstDirectionUp = false;
		double previousLon = -360;

		double clockwiseSum = 0;

		std::shared_ptr<EntityNode> prev = NULL;
		boolean firstWay = true;
		for (std::shared_ptr<EntityWay> w : ways) {
			std::vector<std::shared_ptr<EntityNode>> ns = w->nodes;
			int startInd = 0;
			int nssize = ns.size();
			if (firstWay && nssize > 0) {
				prev = ns[0];
				startInd = 1;
				firstWay = false;
			}
			for (int i = startInd; i < nssize; i++) {
				std::shared_ptr<EntityNode> next = ns[i];
				double rlon = ray_intersect_lon(*prev, *next, lat, lon);
				if (rlon != -360) {
					boolean skipSameSide = (prev->lat <= lat) == (next->lat <= lat);
					if (skipSameSide) {
						continue;
					}
					boolean directionUp = prev->lat <= lat;
					if (firstLon == -360) {
						firstDirectionUp = directionUp;
						firstLon = rlon;
					} else {
						boolean clockwise = (!directionUp) == (previousLon < rlon);
						if (clockwise) {
							clockwiseSum += abs(previousLon - rlon);
						} else {
							clockwiseSum -= abs(previousLon - rlon);
						}
					}
					previousLon = rlon;
				}
				prev = next;
			}
		}

		if (firstLon != -360) {
			boolean clockwise = (!firstDirectionUp) == (previousLon < firstLon);
			if (clockwise) {
				clockwiseSum += abs(previousLon - firstLon);
			} else {
				clockwiseSum -= abs(previousLon - firstLon);
			}
		}

		return clockwiseSum >= 0;
	}

	// try to intersect from left to right
	 double OsmMapUtils::ray_intersect_lon(EntityNode EntityNode1, EntityNode EntityNode2, double latitude, double longitude) {
		// a EntityNode below
		EntityNode a = EntityNode1.lat < EntityNode2.lat ? EntityNode1 : EntityNode2;
		// b EntityNode above
		EntityNode b = a == EntityNode2 ? EntityNode1 : EntityNode2;
		if (latitude == a.lat || latitude == b.lat) {
			latitude += 0.00000001;
		}
		if (latitude < a.lat || latitude > b.lat) {
			return -360;
		} else {
			if (longitude < min(a.lon, b.lon)) {
				return -360;
			} else {
				if (a.lon == b.lon && longitude == a.lon) {
					// the EntityNode on the boundary !!!
					return longitude;
				}
				// that tested on all cases (left/right)
				double lon = b.lon - (b.lat - latitude) * (b.lon - a.lon)
						/ (b.lat - a.lat);
				if (lon <= longitude) {
					return lon;
				} else {
					return -360;
				}
			}
		}
	}

    /**
     * Get the area in pixels
     * @param EntityNodes
     * @return
     */
     double OsmMapUtils::polygonAreaPixels(std::list<std::shared_ptr<EntityNode>> EntityNodes, int zoom) {
        double area = 0.;
        double mult = 1 / MapUtils::getPowZoom(max(31 - (zoom + 8), 0));
		std::vector<std::shared_ptr<EntityNode>> nodesVec(EntityNodes.begin(), EntityNodes.end());
        int j = nodesVec.size() - 1;
        for (int i = 0; i < nodesVec.size(); i++) {
            std::shared_ptr<EntityNode> x = nodesVec[i];
            std::shared_ptr<EntityNode> y = nodesVec[j];
            if(x != NULL && y != NULL) {
            area += (MapUtils::get31TileNumberX(y->lon) + (double)MapUtils::get31TileNumberX(x->lon))*
                    (MapUtils::get31TileNumberY(y->lat) - (double)MapUtils::get31TileNumberY(x->lat));
            }
            j = i;
        }
        return abs(area) * mult * mult * .5;
    }

	/**
	 * Get the area (in m²) of a closed way, represented as a list of EntityNodes
	 * 
	 * @param EntityNodes
	 *            the list of EntityNodes
	 * @return the area of it
	 */
	 double OsmMapUtils::getArea(std::vector<std::shared_ptr<EntityNode>> nodeVec) {
		// x = longitude
		// y = latitude
		// calculate the reference point (lower left corner of the bbox)
		// start with an arbitrary value, bigger than any lat or lon
		double refX = 500, refY = 500;
		for (std::shared_ptr<EntityNode> n : nodeVec) {
			if (n->lat < refY)
				refY = n->lat;
			if (n->lon < refX)
				refX = n->lon;
		}

		std::vector<double> xVal;
		std::vector<double> yVal;

		for (std::shared_ptr<EntityNode> n : nodeVec) {
			// distance from bottom line to x coordinate of EntityNode
			double xDist = MapUtils::getDistance(refY, refX, refY, n->lon);
			// distance from left line to y coordinate of EntityNode
			double yDist = MapUtils::getDistance(refY, refX, n->lat, refX);

			xVal.push_back(xDist);
			yVal.push_back(yDist);
		}

		double area = 0;

		for (int i = 1; i < xVal.size(); i++) {
			area += xVal.at(i - 1) * yVal.at(i) - xVal.at(i) * yVal.at(i - 1);
		}

		return abs(area) / 2;
	}


MapZooms* MapZooms::DEFAULT = nullptr;
int MapZooms::MapZoomPair::MAX_ALLOWED_ZOOM = 22;
std::string MapZooms::MAP_ZOOMS_DEFAULT = "11;12;13-14;15-";
