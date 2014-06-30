#pragma once

template <class TObject> class TileManager
{
private:
	std::map<__int64, std::vector<TObject>> longObjectMap;
	int zoom;

private:
	void putObjects(int tx, int ty, std::vector<TObject>& r){
		if(longObjectMap.find(evTile(tx, ty)) != longObjectMap.end()){
			for (TObject &vec : longObjectMap[evTile(tx, ty)])
				r.push_back(vec);
		}
#ifdef _DEBUG
		else
		{
			//OutputDebugString(L"Can't insert into existing object slot");
		}
#endif
	}

	__int64 evTile(int tileX, int tileY){
		return ((tileX) << zoom) + tileY;
	}

	void removeObject(TObject object, __int64 tile) {
		if(longObjectMap.find(tile) != longObjectMap.end()){
			longObjectMap.erase(longObjectMap.find(tile));
		}
	}
  
	__int64 addObject(TObject object, __int64 tile) {
		if(longObjectMap.find(tile) == longObjectMap.end()){
			longObjectMap.insert(std::make_pair(tile, std::vector<TObject>()));
			longObjectMap.find(tile)->second.push_back(object);
		}
		else
		{
			longObjectMap.find(tile)->second.push_back(object);	
		}
		return tile;
	}

public:
	TileManager(void){zoom = 15;}
	TileManager(int newZoom){zoom = newZoom;}
	~TileManager(void){}

	int getZoom() {return zoom;}

	bool isEmpty() { return longObjectMap.size() == 0;}

	int getObjectCount() { int size = 0;
		std::for_each(longObjectMap.begin(), longObjectMap.end(), 
			[](std::map<__int64, std::vector<TObject>>::iterator itM) { size += itM.second.size();});
		return size;
	}

	
	
	 std::vector<TObject> getAllObjects(){
		std::vector<TObject> l;
		for(std::pair<__int64, std::vector<TObject>> it : longObjectMap)
		{
			for (TObject &vec : it.second)
			{
				l.push_back(vec);
			}
		}
		return l;
	}
	
	 std::vector<TObject> getObjects(double latitudeUp, double longitudeUp, double latitudeDown, double longitudeDown) {
		int tileXUp = (int) MapUtils.getTileNumberX(zoom, longitudeUp);
		int tileYUp = (int) MapUtils.getTileNumberY(zoom, latitudeUp);
		int tileXDown = (int) MapUtils.getTileNumberX(zoom, longitudeDown) + 1;
		int tileYDown = (int) MapUtils.getTileNumberY(zoom, latitudeDown) + 1;
		std::vector<TObject> result;
		for (int i = tileXUp; i <= tileXDown; i++) {
			for (int j = tileYUp; j <= tileYDown; j++) {
				putObjects(i, j, result);
			}
		}
		return result;
	}
	
	 std::vector<TObject> getObjects(int leftX31, int topY31, int rightX31, int bottomY31) {
		std::vector<TObject> result = new ArrayList<TObject>();
		return getObjects(leftX31, topY31, rightX31, bottomY31, result);
	}
	
	 std::vector<TObject> getObjects(int leftX31, int topY31, int rightX31, int bottomY31, std::vector<TObject> result ) {
		int tileXUp = leftX31 >> (31 - zoom);
		int tileYUp = topY31 >> (31 - zoom);
		int tileXDown = (rightX31 >> (31 - zoom)) + 1;
		int tileYDown = (bottomY31 >> (31 - zoom)) + 1;
		for (int i = tileXUp; i <= tileXDown; i++) {
			for (int j = tileYUp; j <= tileYDown; j++) {
				putObjects(i, j, result);
			}
		}
		return result;
	}
	
	/**
	 * @depth of the neighbor tile to visit
	 * returns not exactly sorted list, 
	 * however the first longObjectMap are from closer tile than last
	 */
	 std::vector<TObject> getClosestObjects(double latitude, double longitude, int defaultStep){
		if(isEmpty()){
			return std::vector<TObject>();
		}
		int dp = 0;
		std::vector<TObject> l;
		while (l.size() == 0) {
			l = getClosestObjects(latitude, longitude, dp, dp + defaultStep);
			dp += defaultStep;
		}
		return l;
	}
	
	 std::vector<TObject> getClosestObjects(double latitude, double longitude){
		return getClosestObjects(latitude, longitude, 3);
	}
		
	 std::vector<TObject> getClosestObjects(double latitude, double longitude, int startDepth, int depth){
		int tileX = (int) MapUtils::getTileNumberX((float)zoom, longitude);
		int tileY = (int) MapUtils::getTileNumberY((float)zoom, latitude);
		std::vector<TObject> result;
		
		if(startDepth <= 0){
			putObjects(tileX, tileY, result);
			startDepth = 1;
		}
		
		// that's very difficult way visiting node : 
		// similar to visit by spiral
		// however the simplest way could be to visit row by row & after sort tiles by distance (that's less efficient) 
		
		// go through circle
		for (int i = startDepth; i <= depth; i++) {

			// goes 
			for (int j = 0; j <= i; j++) {
				// left & right
				int dx = j == 0 ? 0 : -1;
				for (; dx < 1 || (j < i && dx == 1); dx += 2) {
					// north
					putObjects(tileX + dx * j, tileY + i, result);
					// east
					putObjects(tileX + i, tileY - dx * j, result);
					// south
					putObjects(tileX - dx * j, tileY - i, result);
					// west
					putObjects(tileX - i, tileY + dx * j, result);
				}
			}
		}
		return result;
	}
	
	
	
#pragma  warning (push)
#pragma  warning (disable: 4244)
	 __int64 evaluateTile(double latitude, double longitude){
		int tileX = (int) MapUtils::getTileNumberX(zoom, longitude);
		int tileY = (int) MapUtils::getTileNumberY(zoom, latitude);
		return evTile(tileX, tileY);
	}

#pragma  warning (pop)
	 long evaluateTileXY(int x31, int y31){
		return evTile(x31 >> (31 - zoom), y31 >> (31 - zoom));
	}
	
	 void unregisterObject(double latitude, double longitude, TObject object){
		long tile = evaluateTile(latitude, longitude);
		removeObject(object, tile);
	}
	
	
	 void unregisterObjectXY(int  x31, int y31, TObject object){
		long tile = evaluateTileXY(x31, y31);
		removeObject(object, tile);
	}

	
	
	 long registerObjectXY(int x31, int y31, TObject object){
		return addObject(object, evTile(x31 >> (31 - zoom), y31 >> (31 - zoom)));
	}
	
	 __int64 registerObject(double latitude, double longitude, TObject object){
		__int64 tile = evaluateTile(latitude, longitude);
		return addObject(object, tile);
	}


	
	 void clear(){
		longObjectMap.clear();
	}
};

