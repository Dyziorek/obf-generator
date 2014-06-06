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
	_fd(-1), _path(""), _size(0)
{}

RandomAccessFile::RandomAccessFile(const boost::filesystem::path& path, RandomAccessFile::Mode mode, uint64_t size) :
	_fd(-1), _path(""), _size(0)
{
	open(path, mode, size);
}

RandomAccessFile::~RandomAccessFile()
{
	close();
}

void RandomAccessFile::open(const boost::filesystem::path& path, RandomAccessFile::Mode mode, uint64_t size)
{
	int m;
	switch (mode) {
		case READ: m = O_RDONLY; break;
		case WRITE: m = O_WRONLY|O_CREAT; break;
		case READWRITE: m = O_RDWR|O_CREAT; break;
		default: throw std::ios_base::failure("Unknown open-mode");
	}
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

	_fd =  ::CreateFile(path.c_str(), m, S_IREAD|S_IWRITE);
	if (_fd == INVALID_HANDLE_VALUE) {
		throw bsys::system_error(bsys::errc::make_error_code(static_cast<bsys::errc::errc_t>(errno)), "Failed opening "+path.string());
	} else if ((_size == 0) && (SetFilePointer(_fd, size, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) || !SetEndOfFile(_fd)) {
		::CloseHandle(_fd);
		ostringstream buf;
		buf << "Failed truncating " << path.string() << " to " << size;
		throw bsys::system_error(bsys::errc::make_error_code(static_cast<bsys::errc::errc_t>(errno)), buf.str());
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
	newSet.QuadPart = newPos;
	__int64 pointerSet;
	DWORD result = SetFilePointerEx(_fd, newSet, &pointerSet, FILE_BEGIN);
	if (result)
	{
		return pointerSet;
	}
	
	return 0;
}

__int64 RandomAccessFile::ByteCount() const
{
	return _size;
}

uint32_t RandomAccessFile::blocks(size_t blockSize) const
{
	// Round up the number of blocks
	return (ByteCount() + blockSize - 1) / blockSize;
}

void RandomAccessFile::BackUp ( int count) {
	
	SetFilePointer(_fd, -count, NULL, FILE_CURRENT);
	filePointer -= count;
}

bool RandomAccessFile::Next(void** src, int* size)
{
	DWORD written;
	filePointer += *size;
	WriteFile(_fd, src, *size, &written, NULL);
	if ((size_t)written != *size)
		throw std::ios_base::failure("Failed to write");
	filePointer += written;
	*size = written;
	return true;
}

size_t RandomAccessFile::writeInt(int val)
{
	DWORD written;
	WriteFile(_fd, &val, sizeof(val), &written, NULL);
	filePointer += written;
}

string RandomAccessFile::describe() {
	return _path.string();
}

const boost::filesystem::path& RandomAccessFile::path() const
{
	return _path;
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

bool BinaryMapDataWriter::writeStartMapIndex(std::string name)
{
	if (pushState(MAP_INDEX_INIT, OSMAND_STRUCTURE_INIT))
	{
		wfl::WireFormatLite::WriteTag(obf::OsmAndStructure::kMapIndexFieldNumber, wfl::WireFormatLite::WireType::WIRETYPE_FIXED32_LENGTH_DELIMITED,&dataOut);
		//dataOut.WriteTag(obf::OsmAndStructure::kMapIndexFieldNumber);
		preserveInt32Size();
		wfl::WireFormatLite::WriteString(obf::OsmAndMapIndex::kNameFieldNumber, name, &dataOut);
	}
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