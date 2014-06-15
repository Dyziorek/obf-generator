#include "stdafx.h"
#include <google\protobuf\io\coded_stream.h>
#include <google\protobuf\io\zero_copy_stream_impl_lite.h>
#include <google\protobuf\io\zero_copy_stream_impl.h>
#include <google\protobuf\wire_format_lite.h>
#include <boost\container\slist.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include "..\..\..\..\core\protos\OBF.pb.h"
#include "BinaryMapDataWriter.h"
#include "ArchiveIO.h"


int BinaryMapDataWriter::OSMAND_STRUCTURE_INIT = 1;
int BinaryMapDataWriter:: MAP_INDEX_INIT = 2;
int BinaryMapDataWriter:: MAP_ROOT_LEVEL_INIT = 3;
int BinaryMapDataWriter:: MAP_TREE = 4;

int BinaryMapDataWriter:: ADDRESS_INDEX_INIT = 5;
int BinaryMapDataWriter:: CITY_INDEX_INIT = 6;

int BinaryMapDataWriter:: TRANSPORT_INDEX_INIT = 9;
int BinaryMapDataWriter:: TRANSPORT_STOPS_TREE = 10;
int BinaryMapDataWriter:: TRANSPORT_ROUTES = 11;

int BinaryMapDataWriter:: POI_INDEX_INIT = 12;
int BinaryMapDataWriter:: POI_BOX = 13;
int BinaryMapDataWriter:: POI_DATA = 14;
	
int BinaryMapDataWriter:: ROUTE_INDEX_INIT = 15;
int BinaryMapDataWriter:: ROUTE_TREE = 16;
int BinaryMapDataWriter:: ROUTE_BORDER_BOX = 17;

int BinaryMapDataWriter::SHIFT_COORDINATES = 5;
int BinaryMapDataWriter::MASK_TO_READ = ~((1 << SHIFT_COORDINATES) - 1);
int BinaryMapDataWriter::ROUTE_SHIFT_COORDINATES = 4;

#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <fcntl.h>
#include <ios>
#include <sstream>
#include <sys/stat.h>

using namespace std;

namespace bsys = boost::system;
namespace fs = boost::filesystem;




RandomAccessFile::RandomAccessFile() :
	_path(""), _size(0), implData(nullptr), filePointer(0)
{
	_fd = INVALID_HANDLE_VALUE;
	codeWork = nullptr;
}

RandomAccessFile::RandomAccessFile(const boost::filesystem::path& path, RandomAccessFile::Mode mode, uint64_t size) :
	_path(""), _size(0), implData(&implCopy), filePointer(0)
{
	codeWork = nullptr;
	_fd = INVALID_HANDLE_VALUE;
	open(path, mode, size);
	implCopy.AssignHandle(_fd);
	implCopy.SetParent(this);
}

RandomAccessFile::~RandomAccessFile()
{
	implData.Flush();
	close();
}

RandomAccessFile::CopyingFileOutputStream::CopyingFileOutputStream()
  : close_on_delete_(false),
    is_closed_(false),
	errno_(0)
{
}

RandomAccessFile::CopyingFileOutputStream::~CopyingFileOutputStream() {
  if (close_on_delete_) {
    if (!Close()) {
		char errBuff[100];
      GOOGLE_LOG(ERROR) << "close() failed: " << strerror_s<100>(errBuff, errno_);
    }
  }
}

bool RandomAccessFile::CopyingFileOutputStream::Close() {
  GOOGLE_CHECK(!is_closed_);

  is_closed_ = true;
  if (CloseHandle(file_) != 0) {
    // The docs on close() do not specify whether a file descriptor is still
    // open after close() fails with EIO.  However, the glibc source code
    // seems to indicate that it is not.
    errno_ = GetLastError();
    return false;
  }

  return true;
}

bool RandomAccessFile::CopyingFileOutputStream::Write(
    const void* buffer, int size) {
  GOOGLE_CHECK(!is_closed_);
  int total_written = 0;

  

  const uint8* buffer_base = reinterpret_cast<const uint8*>(buffer);

  while (total_written < size) {
    DWORD bytes;
	BOOL success = FALSE;
    do {
		success = WriteFile(file_, buffer_base + total_written, size - total_written, &bytes, NULL);
    } while (!success);

    if (bytes <= 0) {
      // Write error.

      // FIXME(kenton):  According to the man page, if write() returns zero,
      //   there was no error; write() simply did not write anything.  It's
      //   unclear under what circumstances this might happen, but presumably
      //   errno won't be set in this case.  I am confused as to how such an
      //   event should be handled.  For now I'm treating it as an error, since
      //   retrying seems like it could lead to an infinite loop.  I suspect
      //   this never actually happens anyway.

      if (bytes < 0) {
        errno_ = GetLastError();
      }
      return false;
    }
    total_written += bytes;
  }
  parentObj->filePointer += total_written;
  return true;
}


void RandomAccessFile::open(const boost::filesystem::path& path, RandomAccessFile::Mode mode, uint64_t size)
{
	int m;
	switch (mode) {
		case READ: m = GENERIC_READ; break;
		case WRITE: m = GENERIC_WRITE; break;
		case READWRITE: m = GENERIC_READ|GENERIC_WRITE; break;
		default: throw std::ios_base::failure("Unknown open-mode");
	}
	bool exists = fs::exists(path);
	_size = fs::exists(path) ? fs::file_size(path) : 0;

	if (_size != size) {
		if (size == 0) {
			size = _size;
		} else if (_size != 0) {
			ostringstream buf;
			buf << path << " exists with mismatching size, (" << size << " : " << fs::file_size(path) << ")";
			throw bsys::system_error(bsys::errc::make_error_code(bsys::errc::file_exists), buf.str());
		}
	}

	std::wstring pathName = path.c_str();

	//CreateFileW(
 //   _In_ LPCWSTR lpFileName,
 //   _In_ DWORD dwDesiredAccess,
 //   _In_ DWORD dwShareMode,
 //   _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
 //   _In_ DWORD dwCreationDisposition,
 //   _In_ DWORD dwFlagsAndAttributes,
 //   _In_opt_ HANDLE hTemplateFile
 //   );
	if (exists)
	{
		_fd =  ::CreateFile(pathName.c_str(), m, 0 , NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL , NULL);
	}
	else
	{
		_fd =  ::CreateFile(pathName.c_str(), m, 0 , NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL , NULL);
	}
	if (_fd == INVALID_HANDLE_VALUE) {
		DWORD win32Err = ::GetLastError();
		LPVOID lpMsgBuf;
		FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        win32Err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
		LocalFree(lpMsgBuf);
		throw bsys::system_error(bsys::errc::make_error_code(static_cast<bsys::errc::errc_t>(errno)), "Failed opening "+path.string());
	} else if ((_size == 0)) {
		DWORD fp = 0;
		BOOL bRes = TRUE;
		fp = SetFilePointer(_fd, size, NULL, FILE_BEGIN);
		bRes = SetEndOfFile(_fd);
		if (fp == INVALID_SET_FILE_POINTER || !bRes)
		{
			DWORD win32Err = ::GetLastError();
		LPVOID lpMsgBuf;
		FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | 
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        win32Err,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) &lpMsgBuf,
        0, NULL );
		LocalFree(lpMsgBuf);
			::CloseHandle(_fd);
			ostringstream buf;
			buf << "Failed truncating " << path.string() << " to " << size;
			throw bsys::system_error(bsys::errc::make_error_code(static_cast<bsys::errc::errc_t>(errno)), buf.str());
		}
	}
	_path = path;
	_size = size;
}

void RandomAccessFile::close()
{
	if (is_open()) {
		::CloseHandle(_fd);
		_fd = INVALID_HANDLE_VALUE;
		_path = "";
	}
}

bool RandomAccessFile::is_open() const
{
	return _fd != INVALID_HANDLE_VALUE;
}

__int64 RandomAccessFile::seek(__int64 newPos)
{
	filePointer = newPos;
	LARGE_INTEGER newSet;
	LARGE_INTEGER newSetPointer;
	newSet.QuadPart = newPos;
	__int64 pointerSet;
	DWORD result = SetFilePointerEx(_fd, newSet, &newSetPointer, FILE_BEGIN);
	if (result)
	{
		newPos = newSetPointer.QuadPart;
		return newPos;
	}
	
	return 0;
}

__int64 RandomAccessFile::ByteCount() const
{
	return implData.ByteCount();
}

uint32_t RandomAccessFile::blocks(size_t blockSize) const
{
	// Round up the number of blocks
	return (ByteCount() + blockSize - 1) / blockSize;
}

void RandomAccessFile::BackUp ( int count) {
	
	implData.BackUp(count);
	//SetFilePointer(_fd, -count, NULL, FILE_CURRENT);
	//filePointer -= count;
}

bool RandomAccessFile::Next(void** src, int* size)
{
	bool result = implData.Next(src, size);
	_currentBuffer = (uint8*)*src;
	return result;
}

size_t RandomAccessFile::writeInt(int val)
{
	DWORD written;
	WriteFile(_fd, &val, sizeof(val), &written, NULL);
	filePointer += written;
	return written;
}

string RandomAccessFile::describe() {
	return _path.string();
}

const boost::filesystem::path& RandomAccessFile::path() const
{
	return _path;
}

BinaryMapDataWriter::BinaryMapDataWriter(RandomAccessFile* outData) : dataOut(outData)
{
		raf = outData;
		raf->SetCodedOutStream(&dataOut);
		wfl::WireFormatLite::WriteUInt32(obf::OsmAndStructure::kVersionFieldNumber, 2, &dataOut);
		time_t timeDate;
		time(&timeDate);
		wfl::WireFormatLite::WriteInt64(obf::OsmAndStructure::kDateCreatedFieldNumber, timeDate, &dataOut);
		pushState(OSMAND_STRUCTURE_INIT);
}

BinaryMapDataWriter::~BinaryMapDataWriter(void)
{
}

void BinaryMapDataWriter::preserveInt32Size()
{
	__int64 local = getFilePointer();
	BinaryFileReference binRef = BinaryFileReference::createSizeReference(local);
	references.push_front(binRef);
	wfl::WireFormatLite::WriteFixed32NoTag(0, &dataOut);
}

int BinaryMapDataWriter::writeInt32Size()
{
	BinaryFileReference ref = references.front();
	references.pop_front();
	// write directly to file with shift et al...
	int length = ref.writeReference(*raf, getFilePointer());
	return length;
}

void BinaryMapDataWriter::writeRawVarint32(std::vector<uint8>& mapDataBuf,int toVarint32)
{
	google::protobuf::uint32 result = wfl::WireFormatLite::ZigZagEncode32(toVarint32);
	while (result & 0x7F != 0)
	{
		mapDataBuf.push_back((result & 0x7F) | 0x80);
		result >>= 7;
	}
	mapDataBuf.push_back(result);
}

bool BinaryMapDataWriter::writeStartMapIndex(std::string name)
{
	if (pushState(MAP_INDEX_INIT, OSMAND_STRUCTURE_INIT))
	{
		wfl::WireFormatLite::WriteTag(obf::OsmAndStructure::kMapIndexFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
		//dataOut.WriteTag(obf::OsmAndStructure::kMapIndexFieldNumber);
		preserveInt32Size();
		wfl::WireFormatLite::WriteString(obf::OsmAndMapIndex::kNameFieldNumber, name, &dataOut);
	}
	return true;
}

void BinaryMapDataWriter::writeMapEncodingRules(boost::ptr_map<std::string, MapRulType>& types) 
{
	peekState(MAP_INDEX_INIT);

	std::vector<MapRulType*> out;
		int highestTargetId = types.size();
		// 1. prepare map rule type to write
		for (auto rulType : types) {
			if (rulType.second->targetTagValue != nullptr || rulType.second->getFreq() == 0 || !rulType.second->isMap()) {
				rulType.second->setTargetId(highestTargetId++);
			} else {
				out.push_back(rulType.second);
			}
		}

		// 2. sort by frequency and assign ids
		std::sort(out.begin(), out.end(), [](MapRulType* op1, MapRulType* op2)
		{
			return op1->freq < op2->freq;
		});

		for (int i = 0; i < out.size(); i++) {

			obf::OsmAndMapIndex_MapEncodingRule builder;
			
			MapRulType* rule = out[i];
			rule->setTargetId(i + 1);

			builder.set_tag(rule->getTag());
			if (rule->getValue() != "") {
				builder.set_value(rule->getValue());
			}
			builder.set_minzoom(rule->getMinzoom());
			if (rule->isAdditional()) {
				builder.set_type(1);
			} else if(rule->isText()) {
				builder.set_type(2);
			}
			
			wfl::WireFormatLite::WriteMessage(obf::OsmAndMapIndex::kRulesFieldNumber, builder, &dataOut);

			//dataStream.writeMessage(OsmandOdb.OsmAndMapIndex.RULES_FIELD_NUMBER, rulet);
		}
	}


void BinaryMapDataWriter::startWriteMapLevelIndex(int minZoom, int maxZoom, int leftX, int rightX, int topY, int bottomY)
{
		pushState(MAP_ROOT_LEVEL_INIT, MAP_INDEX_INIT);

		wfl::WireFormatLite::WriteTag(obf::OsmAndMapIndex::kLevelsFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
		preserveInt32Size();

		wfl::WireFormatLite::WriteInt32(obf::OsmAndMapIndex_MapRootLevel::kMaxZoomFieldNumber, maxZoom, &dataOut);
		
		wfl::WireFormatLite::WriteInt32(obf::OsmAndMapIndex_MapRootLevel::kMinZoomFieldNumber, minZoom, &dataOut);
		
		wfl::WireFormatLite::WriteInt32(obf::OsmAndMapIndex_MapRootLevel::kLeftFieldNumber, leftX, &dataOut);
		
		wfl::WireFormatLite::WriteInt32(obf::OsmAndMapIndex_MapRootLevel::kRightFieldNumber, rightX, &dataOut);
		
		wfl::WireFormatLite::WriteInt32(obf::OsmAndMapIndex_MapRootLevel::kTopFieldNumber, topY, &dataOut);
		
		wfl::WireFormatLite::WriteInt32(obf::OsmAndMapIndex_MapRootLevel::kBottomFieldNumber, bottomY, &dataOut);
		

		stackBounds.push_front(Bounds(leftX, rightX, topY, bottomY));
	}

std::unique_ptr<BinaryFileReference> BinaryMapDataWriter::startMapTreeElement(int leftX, int rightX, int topY, int bottomY, bool containsObjects, int landCharacteristic)
{
	int vacP[] = {MAP_ROOT_LEVEL_INIT, MAP_TREE};
	checkPeek(vacP, sizeof(vacP)/sizeof(int));
	if (states.front() == MAP_ROOT_LEVEL_INIT) {
		wfl::WireFormatLite::WriteTag(obf::OsmAndMapIndex_MapRootLevel::kBoxesFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
		} else {
			wfl::WireFormatLite::WriteTag(obf::OsmAndMapIndex_MapDataBox::kBoxesFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
		}
		pushState(MAP_TREE);
		preserveInt32Size();
		long fp = getFilePointer();

        // align with borders with grid
        leftX &= MASK_TO_READ;
        topY &= MASK_TO_READ;
		Bounds bounds = stackBounds.front();
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndMapIndex_MapDataBox::kLeftFieldNumber, leftX - bounds.left, &dataOut);
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndMapIndex_MapDataBox::kRightFieldNumber,  rightX - bounds.right, &dataOut);
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndMapIndex_MapDataBox::kTopFieldNumber, topY - bounds.top, &dataOut);
		wfl::WireFormatLite::WriteSInt32(obf::OsmAndMapIndex_MapDataBox::kBottomFieldNumber, bottomY - bounds.bottom, &dataOut);

		if(landCharacteristic != 0) {
			wfl::WireFormatLite::WriteBool(obf::OsmAndMapIndex_MapDataBox::kOceanFieldNumber, landCharacteristic < 0, &dataOut);
		}
		stackBounds.push_front(Bounds(leftX, rightX, topY, bottomY));
		std::unique_ptr<BinaryFileReference> ref;
		if (containsObjects) {
			wfl::WireFormatLite::WriteTag(obf::OsmAndMapIndex_MapDataBox::kShiftToMapDataFieldNumber,  wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED, &dataOut);
			ref = std::move(std::unique_ptr<BinaryFileReference>(BinaryFileReference::createShiftReference(getFilePointer(), fp)));
			wfl::WireFormatLite::WriteFixed32NoTag(0, &dataOut);
		}
		return ref;
	}

void BinaryMapDataWriter::endWriteMapTreeElement(){
		popState(MAP_TREE);
		stackBounds.pop_front();
		writeInt32Size();
	}


obf::MapDataBlock* BinaryMapDataWriter::createWriteMapDataBlock(__int64 baseID)
{
	obf::MapDataBlock block;
	obf::MapDataBlock* pBlock = block.New();
	pBlock->set_baseid(baseID);
	return pBlock;
}

int BinaryMapDataWriter::skipSomeNodes(const void* coordinates, int len, int i, int x, int y, boolean multi) {
		int delta;
		delta = 1;
		// keep first/latest point untouched
		// simplified douglas\peuker
		// just try to skip some points very close to this point
		while (i + delta < len - 1) {
			int nx = parseIntFromBytes(coordinates, (i + delta) * 8);
			int ny = parseIntFromBytes(coordinates, (i + delta) * 8 + 4);
			int nnx = parseIntFromBytes(coordinates, (i + delta + 1) * 8);
			int nny = parseIntFromBytes(coordinates, (i + delta + 1) * 8 + 4);
			if(nnx == 0 && nny == 0) {
				break;
			}
			double dist = BinaryMapDataWriter::orthogonalDistance(nx, ny, x, y, nnx, nny);
			if (dist > 31 ) {
				break;
			}
			delta++;
		}
		return delta;
	}

obf::MapData BinaryMapDataWriter::writeMapData(__int64 diffId, int pleft, int ptop, sqlite3_stmt* selectData, std::vector<int> typeUse,
			std::vector<int> addtypeUse, std::map<MapRulType, std::string>& names, boost::unordered_map<std::string, int>& stringTable, obf::MapDataBlock* dataBlock,
			bool allowCoordinateSimplification)
			{
		obf::MapData data;
		// calculate size
		mapDataBuf.clear();
		int pcalcx = (pleft >> SHIFT_COORDINATES);
		int pcalcy = (ptop >> SHIFT_COORDINATES);
		const void* plData = sqlite3_column_blob(selectData, 1);
		int bloblSize = sqlite3_column_bytes(selectData, 1);
		int len = bloblSize / 8;
		int delta = 1;
		for (int i = 0; i < len; i+= delta) {
			int x = parseIntFromBytes(plData, i * 8);
			int y = parseIntFromBytes(plData, i * 8 + 4);
			int tx = (x >> SHIFT_COORDINATES) - pcalcx;
			int ty = (y >> SHIFT_COORDINATES) - pcalcy;
			writeRawVarint32(mapDataBuf, tx);
			writeRawVarint32(mapDataBuf, ty);
			pcalcx = pcalcx + tx ;
			pcalcy = pcalcy + ty ;
			delta = 1;
			if (allowCoordinateSimplification) {
				delta = skipSomeNodes(plData, len, i, x, y, false);
			}
		}
		//COORDINATES_SIZE += CodedOutputStream.computeRawVarint32Size(mapDataBuf.size())
		//		+ CodedOutputStream.computeTagSize(MapData.COORDINATES_FIELD_NUMBER) + mapDataBuf.size();
		int area = sqlite3_column_int(selectData, 0);
		if (area) {
			data.set_areacoordinates((void*)&mapDataBuf.front(), mapDataBuf.size());
		} else {
			data.set_coordinates((void*)&mapDataBuf.front(), mapDataBuf.size());
		}
		 plData = sqlite3_column_blob(selectData, 2);
		 bloblSize = sqlite3_column_bytes(selectData, 2);
		if (bloblSize) {
			mapDataBuf.clear();
			pcalcx = (pleft >> SHIFT_COORDINATES);
			pcalcy = (ptop >> SHIFT_COORDINATES);
			len = bloblSize / 8;
			for (int i = 0; i < len; i+= delta) {
				int x = parseIntFromBytes(plData, i * 8);
				int y = parseIntFromBytes(plData, i * 8 + 4);
				if (x == 0 && y == 0) {
					if (mapDataBuf.size() > 0) {
						data.add_polygoninnercoordinates((void*)&mapDataBuf.front(), mapDataBuf.size());
						mapDataBuf.clear();
					}
					pcalcx = (pleft >> SHIFT_COORDINATES);
					pcalcy = (ptop >> SHIFT_COORDINATES);
				} else {
					int tx = (x >> SHIFT_COORDINATES) - pcalcx;
					int ty = (y >> SHIFT_COORDINATES) - pcalcy;

					writeRawVarint32(mapDataBuf, tx);
					writeRawVarint32(mapDataBuf, ty);

					pcalcx = pcalcx + tx ;
					pcalcy = pcalcy + ty ;
					delta = 1;
					if (allowCoordinateSimplification) {
						delta = skipSomeNodes(plData, len, i, x, y, true);
					}
				}
			}
		}
		
		mapDataBuf.clear();
		for (int i = 0; i < typeUse.size() ; i++) {
			writeRawVarint32(mapDataBuf, typeUse[i]);
		}
		data.set_types((void*)&mapDataBuf.front(), mapDataBuf.size());
		//TYPES_SIZE += CodedOutputStream.computeTagSize(OsmandOdb.MapData.TYPES_FIELD_NUMBER)
		//		+ CodedOutputStream.computeRawVarint32Size(mapDataBuf.size()) + mapDataBuf.size();
		if (addtypeUse.size() > 0) {
			mapDataBuf.clear();
			for (int i = 0; i < addtypeUse.size() ; i++) {
				writeRawVarint32(mapDataBuf, addtypeUse[i]);
			}
			data.set_additionaltypes((void*)&mapDataBuf.front(), mapDataBuf.size());
			//TYPES_SIZE += CodedOutputStream.computeTagSize(OsmandOdb.MapData.ADDITIONALTYPES_FIELD_NUMBER);
		}

		mapDataBuf.clear();
		if (names.size() > 0) {
			for (std::pair<MapRulType, std::string> s : names) {
				writeRawVarint32(mapDataBuf, s.first.getTargetId());
				int ls = 0;
				if (stringTable.find(s.second) == stringTable.end())
				{
					int ls = stringTable.size();
					stringTable.insert(std::make_pair(s.second, ls));
				}
				else
				{
					ls = stringTable[s.second];
				}
				writeRawVarint32(mapDataBuf, ls);
			}
		}
		//STRING_TABLE_SIZE += mapDataBuf.size();
		if (names.size() > 0)
		{
			data.set_stringnames((void*)&mapDataBuf.front(), mapDataBuf.size());
		}

		data.set_id(diffId);
		//ID_SIZE += CodedOutputStream.computeSInt64Size(OsmandOdb.MapData.ID_FIELD_NUMBER, diffId);
		return data;
	}

	void BinaryMapDataWriter::writeMapDataBlock(obf::MapDataBlock* builder, boost::unordered_map<std::string, int>& stringTable, BinaryFileReference& ref)
	 {
		 int vacP[] = {MAP_ROOT_LEVEL_INIT};
		checkPeek(vacP, sizeof(vacP)/sizeof(int));
		obf::StringTable bs;
		if (!stringTable.empty()) {
			for (std::pair<std::string,int> s : stringTable) {
				bs.add_s(s.first);
			}
		
		
		obf::StringTable* refSt = builder->mutable_stringtable();
		refSt->MergeFrom(bs);
		int size = refSt->ByteSize();

		//STRING_TABLE_SIZE += CodedOutputStream.computeTagSize(OsmandOdb.MapDataBlock.STRINGTABLE_FIELD_NUMBER)
		//		+ CodedOutputStream.computeRawVarint32Size(size) + size;
		}
		wfl::WireFormatLite::WriteTag(obf::OsmAndMapIndex_MapRootLevel::kBlocksFieldNumber,  wfl::WireFormatLite::WireTypeForFieldType(wfl::WireFormatLite::FieldType::TYPE_MESSAGE),&dataOut);
		
		//codedOutStream.flush();
		ref.writeReference(*raf, getFilePointer());
		//MapDataBlock block = builder.build();
		//MAP_DATA_SIZE += block.getSerializedSize();
		builder->SerializeToCodedStream(&dataOut);
		//codedOutStream.writeMessageNoTag(block);
	}

	void BinaryMapDataWriter::endWriteMapLevelIndex() {
		popState(MAP_ROOT_LEVEL_INIT);
		stackBounds.pop_front();
		int len = writeInt32Size();
		//log.info("MAP level SIZE : " + len);
	}

	void BinaryMapDataWriter::endWriteMapIndex()
	{
		popState(MAP_INDEX_INIT);
		writeInt32Size();
	}