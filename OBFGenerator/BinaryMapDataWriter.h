#pragma once



namespace gio = google::protobuf::io;
namespace obf = OsmAnd::OBF;
namespace wfl = google::protobuf::internal;

typedef unsigned char uint8;

class RandomAccessFile : boost::noncopyable, public gio::ZeroCopyOutputStream {
private:
	HANDLE _fd;
	boost::filesystem::path _path;
	__int64 filePointer;
	uint64_t _size;
public:
	enum Mode {
	READ = 1,
	WRITE = 2,
	READWRITE = READ|WRITE,
	};

	RandomAccessFile();
	RandomAccessFile(const boost::filesystem::path& path, RandomAccessFile::Mode mode = READ, uint64_t size = 0);
	~RandomAccessFile();

	/**
	 * Open the given file. May throw std::ios_base::failure.
	 *
	 * non-zero size means open and create file of this size
	 */
	void open(const boost::filesystem::path& path, RandomAccessFile::Mode mode = READ, uint64_t size = 0);

	/**
	 * Close the underlying file
	 */
	void close();

	/**
	 * See if file is currently open
	 */
	bool is_open() const;

	__int64 seek(__int64 pointer);

	__int64 getFilePointer() { 
		return filePointer;
	}
	/**
	 * The number of bytes in the open file
	 */

	
	/**
	 * The number of blocks of /blockSize/ required to hold all file content
	 */
	uint32_t blocks(size_t blockSize) const;

	/// Implement IDataArray
	virtual bool Next(void** data, int* size);
	virtual void BackUp(int count);
	virtual __int64 ByteCount() const;
	//virtual size_t read(uint64_t offset, size_t size, byte* buf) ;
	//virtual size_t write(uint64_t offset, const void* src, size_t size);
	virtual std::string describe();

	size_t writeInt(int val);

	void SetCodedOutStream(gio::CodedOutputStream* streamWork)
	{  codeWork = streamWork; }

	/**
	 * Return the path used to open the file
	 */
	const boost::filesystem::path& path() const;

	gio::CopyingOutputStreamAdaptor implData;
	gio::CodedOutputStream* codeWork;
	uint8* _currentBuffer;

private:
	class  CopyingFileOutputStream : public gio::CopyingOutputStream {
   public:
	   CopyingFileOutputStream();
    ~CopyingFileOutputStream();

    bool Close();
    void SetCloseOnDelete(bool value) { close_on_delete_ = value; }
    int GetErrno() { return errno_; }
	void AssignHandle(HANDLE _fileH) {file_ = _fileH;}
    // implements CopyingOutputStream --------------------------------
    bool Write(const void* buffer, int size);
	void SetParent(RandomAccessFile* extObj) {parentObj = extObj;}
   private:
    // The file descriptor.
    HANDLE file_;
    bool close_on_delete_;
    bool is_closed_;
	RandomAccessFile* parentObj;
    // The errno of the I/O error, if one has occurred.  Otherwise, zero.
    int errno_;

    GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(CopyingFileOutputStream);
  };

	CopyingFileOutputStream implCopy;
};


class BinaryFileReference {
	
private:
	__int64 pointerToWrite;
	__int64 pointerToCalculateShiftFrom;
	__int64 pointerToCalculateShiftTo;
	
public:
	BinaryFileReference(__int64 pointerToWrite, __int64 pointerToCalculateShiftFrom) {
		this->pointerToWrite = pointerToWrite;
		this->pointerToCalculateShiftFrom = pointerToCalculateShiftFrom;
	}
	
	__int64 getStartPointer() {
		return pointerToCalculateShiftFrom;
	}
	
	__int64 getWritePointer()
	{
		return pointerToWrite;
	}
	int writeReference(RandomAccessFile& raf, __int64 pointerToCalculateShifTo)  {
		this->pointerToCalculateShiftTo = pointerToCalculateShifTo;
		__int64 currentPosition = raf.getFilePointer();
		int val = -1;
		if (currentPosition < pointerToWrite)
		{
			// cannot seek back to file, still in the buffer before write
			uint8* beginBuffer = raf._currentBuffer;
			if (raf.ByteCount() > pointerToWrite)
			{
				// it should be that if not this is error
				val = (int) (pointerToCalculateShiftTo - pointerToCalculateShiftFrom);
				memcpy((void*)(beginBuffer + (pointerToWrite-currentPosition)), &val, sizeof(val));
			}
			else
			{
				return -1;
			}
		}
		else
		{
			// it is in the written part
			raf.seek(pointerToWrite);
			val = (int) (pointerToCalculateShiftTo - pointerToCalculateShiftFrom);
			raf.writeInt(val);
			raf.seek(currentPosition);
		}
		return val;
	}

	
	static BinaryFileReference createSizeReference(__int64 pointerToWrite){
		return BinaryFileReference(pointerToWrite, pointerToWrite + 4);
	}
	
	static BinaryFileReference* createShiftReference(__int64 pointerToWrite, __int64 pointerShiftFrom){
		return new BinaryFileReference(pointerToWrite, pointerShiftFrom);
	}
	

};


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
	RandomAccessFile* raf;

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
	BinaryMapDataWriter(RandomAccessFile* outData);
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
			states.pop_front();
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
	int skipSomeNodes(const void* coordinates, int len, int i, int x, int y, boolean multi);
	bool writeStartMapIndex(std::string name);
	void writeMapEncodingRules(boost::ptr_map<std::string, MapRulType>& types);
	void startWriteMapLevelIndex(int minZoom, int maxZoom, int leftX, int rightX, int topY, int bottomY);
	std::unique_ptr<BinaryFileReference> startMapTreeElement(int leftX, int rightX, int topY, int bottomY, bool containsObjects, int landCharacteristic);
	void endWriteMapTreeElement();
	void preserveInt32Size();
	int writeInt32Size();
	void writeRawVarint32(std::vector<uint8>& mapDataBuf,int toVarint32);

	obf::MapDataBlock* createWriteMapDataBlock(__int64 baseID);
	obf::MapData writeMapData(__int64 diffId, int pleft, int ptop, sqlite3_stmt* selectData, std::vector<int> typeUse,
			std::vector<int> addtypeUse, std::map<MapRulType, std::string>& names, boost::unordered_map<std::string, int>& stringTable, obf::MapDataBlock* dataBlock,
			bool allowCoordinateSimplification);
	void writeMapDataBlock(obf::MapDataBlock* builder, boost::unordered_map<std::string, int>& stringTable, BinaryFileReference& ref);
	void endWriteMapLevelIndex();
	void endWriteMapIndex();
	
	
	

	google::protobuf::io::CodedOutputStream dataOut;
	std::stringstream dataStream;
	std::vector<uint8> mapDataBuf;

	private:
	boost::container::slist<int> states;
	boost::container::slist<Bounds> stackBounds;
	boost::container::slist<BinaryFileReference> references;
};

