#pragma once


namespace gio = google::protobuf::io;
namespace obf = OsmAnd::OBF;
namespace wfl = google::protobuf::internal;

typedef unsigned char uint8;





class BinaryMapDataWriter
{
private:
	struct Bounds{
		Bounds(int l, int r, int t, int b)
		{
			left= l;
			top = t;
			bottom = b;
			right = r;
		}

		int left;
		int right;
		int top;
		int bottom;
	};
private:
	RandomAccessFileWriter* raf;

	static int OSMAND_STRUCTURE_INIT;
	static int MAP_INDEX_INIT;
	static int MAP_ROOT_LEVEL_INIT;
	static int MAP_TREE;

	static int ADDRESS_INDEX_INIT;
	static int CITY_INDEX_INIT;

	static int TRANSPORT_INDEX_INIT;
	static int TRANSPORT_STOPS_TREE;
	static int TRANSPORT_ROUTES;

	static int POI_INDEX_INIT;
	static int POI_BOX;
	static int POI_DATA;
	
	static int ROUTE_INDEX_INIT;
	static int ROUTE_TREE;
	static int ROUTE_BORDER_BOX;

	static int SHIFT_COORDINATES;
    static int MASK_TO_READ;
	static int ROUTE_SHIFT_COORDINATES;
public:
	BinaryMapDataWriter(RandomAccessFileWriter* outData);
	~BinaryMapDataWriter(void);

	__int64 getFilePointer()
	{
		return dataOut.ByteCount();
	}

	bool pushState(int nextState, int prevState)
	{
		if (states.front() == prevState)
		{
			states.push_front(nextState);
#ifdef _DEBUG
		std::wstringstream strm;
		strm << L"States on stack:";
		for (int isState : states)
		{
			strm << " " << isState;
		}
		strm << std::endl << L"pushing state:" << nextState << std::endl;
		OutputDebugString(strm.str().c_str());
#endif	
		return true;
		}
#ifdef _DEBUG
		std::wstringstream strm;
		strm << L"Wrong States to push on stack:";
		for (int isState : states)
		{
			strm << " " << isState;
		}
		strm << std::endl << L"pushing state:" << nextState << L" expected last state " << prevState << std::endl;
		OutputDebugString(strm.str().c_str());
#endif
		return false;
	}

	bool pushState(int nextState)
	{
		states.push_front(nextState);
#ifdef _DEBUG
		std::wstringstream strm;
		strm << L"States on stack:";
		for (int isState : states)
		{
			strm << " " << isState;
		}
		strm << std::endl << L"pushing state:" << nextState << std::endl;
		OutputDebugString(strm.str().c_str());
#endif
		return true;
	}

	bool popState(int lastState)
	{
		if (states.front() == lastState)
		{
#ifdef _DEBUG
			std::wstringstream strm;
			strm << L"States on stack:";
			for (int isState : states)
			{
				strm << " " << isState;
			}
			strm << std::endl << L"popping state:" << lastState << std::endl;
			OutputDebugString(strm.str().c_str());
#endif
			states.pop_front();
			return true;
		}
#ifdef _DEBUG
		std::wstringstream strm;
		strm << L"Wrong States on stack:";
		for (int isState : states)
		{
			strm << " " << isState;
		}
		strm << std::endl << L"popping state:" << lastState << std::endl;
		OutputDebugString(strm.str().c_str());
#endif
		return false;
	}

	bool peekState(int lastState)
	{
		return states.front() == lastState;
	}

	void checkPeek(int* statesArg, int sizeInt)
	{
		for(int stateValIdx = 0; stateValIdx < sizeInt; stateValIdx++)
		{
			if (statesArg[stateValIdx] == states.front())
				return;
		}
		throw std::bad_exception("Not allowed states");
	}
	static double orthogonalDistance(int x, int y, int x1, int y1, int x2, int y2) {
		long A = (x - x1);
		long B = (y - y1);
		long C = (x2 - x1);
		long D = (y2 - y1);
		return abs(A * D - C * B) / sqrt(C * C + D * D);
	}
	int skipSomeNodes(const void* coordinates, int len, int i, int x, int y, bool multi);
	bool writeStartMapIndex(std::string name);
	void writeMapEncodingRules(boost::ptr_map<std::string, MapRulType>& types);
	void startWriteMapLevelIndex(int minZoom, int maxZoom, int leftX, int rightX, int topY, int bottomY);
	std::unique_ptr<BinaryFileReference> startMapTreeElement(int leftX, int rightX, int topY, int bottomY, bool containsObjects, int landCharacteristic);
	void endWriteMapTreeElement();
	void preserveInt32Size();
	int writeInt32Size();
	void writeRawVarint32(std::vector<uint8>& mapDataBuf,int toVarint32);

	// Map Data Functions

	obf::MapDataBlock* createWriteMapDataBlock(__int64 baseID);
	obf::MapData writeMapData(__int64 diffId, int pleft, int ptop, sqlite3_stmt* selectData, std::vector<int> typeUse,
			std::vector<int> addtypeUse, std::map<MapRulType, std::string>& names, std::unordered_map<std::string, int>& stringTable, obf::MapDataBlock* dataBlock,
			bool allowCoordinateSimplification);
	void writeMapDataBlock(obf::MapDataBlock* builder, std::unordered_map<std::string, int>& stringTable, BinaryFileReference& ref);
	void endWriteMapLevelIndex();
	void endWriteMapIndex();
	
	// Addressing Data Functions
	
	bool startWriteAddressIndex(std::string name);
	void endWriteAddressIndex();
	
	void startCityBlockIndex(int type);
	void endCityBlockIndex();
	void writeAddressNameIndex(std::map<std::string, std::list<std::shared_ptr<MapObject>>>& namesIndex);
	bool checkEnNameToWrite(MapObject& obj);

	std::unordered_map<std::string, std::shared_ptr<BinaryFileReference>> writeIndexedTable(int tag, std::list<std::string> indexedTable);
	BinaryFileReference* writeCityHeader(MapObject& city, int cityType);
	obf::StreetIndex createStreetAndBuildings(Street street, int cx, int cy, std::string postcodeFilter, 
			std::unordered_map<__int64,std::set<Street>>& mapNodeToStreet, std::unordered_map<Street, std::list<EntityNode>>& wayNodes);
	void writeCityIndex(CityObj cityOrPostcode, std::list<Street>& streets, std::unordered_map<Street, std::list<EntityNode>>& wayNodes, 
			BinaryFileReference* ref);
	
	// POI Data Functions
	__int64 startWritePoiIndex(std::string name, int left31, int right31, int bottom31, int top31);
	void endWritePoiBox();
	void endWritePoiData();
	void endWritePoiIndex();
	void writePoiCategoriesTable(POICategory& cs);
	 void writePoiCategories(POICategory& poiCats);
	 void writePoiSubtypesTable(POICategory& cs);
	std::unordered_map<POIBox,  std::list<std::shared_ptr<BinaryFileReference>>> writePoiNameIndex(std::unordered_map<std::string, std::unordered_set<POIBox>>& namesIndex, __int64 startPoiIndex);
	void writePoiDataAtom(long id, int x24shift, int y24shift, 
			std::string type, std::string subtype,  std::unordered_map<MapRulType*, std::string>& additionalNames, OBFRenderingTypes& rtypes, 
			POICategory&  globalCategories);
	void startWritePoiData(int zoom, int x, int y, std::vector<std::shared_ptr<BinaryFileReference>>& fpPoiBox);
	std::shared_ptr<BinaryFileReference> startWritePoiBox(int zoom, int tileX, int tileY, __int64 startPoiIndex, bool end);

	// Route Data fctns
	void startWriteRouteIndex(std::string name);
	void endWriteRouteIndex();
	void endRouteTreeElement();
	void writeRouteEncodingRules(std::list<MapRouteType> routeEncode);
	std::unique_ptr<BinaryFileReference> startRouteTreeElement(int leftX, int rightX, int topY, int bottomY, bool containsObjects, bool basemap);
	obf::RouteData writeRouteData(int diffId, int pleft, int ptop, std::vector<int> types, std::vector<std::tuple<int, int, int, std::vector<int>>>  points, 
		std::unordered_map<MapRouteType, std::string, hashMapRoute, equalMapRoute>& names, std::unordered_map<std::string, int>& stringTable, obf::OsmAndRoutingIndex_RouteDataBlock& dataBlock,
			bool allowCoordinateSimplification, bool writePointId);
	void writeRouteDataBlock(obf::OsmAndRoutingIndex_RouteDataBlock& builder, std::unordered_map<std::string, int>& stringTable , BinaryFileReference& ref);
	void close();


	google::protobuf::io::CodedOutputStream dataOut;
	std::stringstream dataStream;
	std::vector<uint8> mapDataBuf;
	std::vector<uint8> typesDataBuf;
	std::vector<uint8> typesAddDataBuf;

	private:
	boost::container::slist<int> states;
	boost::container::slist<Bounds> stackBounds;
	boost::container::slist<BinaryFileReference> references;
};

