#include "stdafx.h"
#include "BinaryMapDataWriter.h"
#include "..\..\..\..\core\protos\OBF.pb.h"




#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <fcntl.h>
#include <ios>
#include <sstream>
#include <sys/stat.h>

using namespace std;

namespace bsys = boost::system;
namespace fs = boost::filesystem;

size_t IDataArray::write ( uint64_t offset, const string& buf ) {
	return write(offset, buf.data(), buf.length());
}

string dataArrayToString ( const IDataArray& dataarray ) {
	std::vector<byte> buf(dataarray.size());
	dataarray.read(0, dataarray.size(), buf.data());
	return string(reinterpret_cast<char*>(buf.data()), buf.size());
}

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

uint64_t RandomAccessFile::size() const
{
	return _size;
}

uint32_t RandomAccessFile::blocks(size_t blockSize) const
{
	// Round up the number of blocks
	return (size() + blockSize - 1) / blockSize;
}

size_t RandomAccessFile::read ( uint64_t offset, size_t size, byte* buf ) const {
	BOOST_ASSERT(buf);
	BOOST_ASSERT(offset+size <= _size);
	SetFilePointer(_fd, offset, NULL, FILE_BEGIN);
	DWORD sizeRead;
	ReadFile(_fd, buf, size, &sizeRead, NULL);
	return sizeRead;
}

size_t RandomAccessFile::write(uint64_t offset, const void* src, size_t size)
{
	DWORD written;
	SetFilePointer(_fd, offset, NULL, FILE_BEGIN);
	WriteFile(_fd, src, size, &written, NULL);
	if ((size_t)written != size)
		throw std::ios_base::failure("Failed to write");
	return written;
}

string RandomAccessFile::describe() {
	return _path.string();
}

const boost::filesystem::path& RandomAccessFile::path() const
{
	return _path;
}

DataArraySlice::DataArraySlice ( const IDataArray::Ptr& parent, uint64_t offset, uint64_t size ) :
	_parent(parent),
	_offset(offset),
	_size(size)
{
	BOOST_ASSERT(offset + size <= _parent->size());
}

DataArraySlice::DataArraySlice ( const IDataArray::Ptr& parent, uint64_t offset ) :
	_parent(parent),
	_offset(offset),
	_size(parent->size()-offset)
{
	BOOST_ASSERT(offset <= _parent->size());
}

uint64_t DataArraySlice::size() const {
	return _size;
}

size_t DataArraySlice::read ( uint64_t offset, size_t size, byte* buf ) const {
	BOOST_ASSERT(offset + size <= _size);
	return _parent->read(_offset + offset, size, buf);
}

size_t DataArraySlice::write ( uint64_t offset, const void* src, size_t size ) {
	BOOST_ASSERT(offset + size <= _size);
	return _parent->write(_offset + offset, src, size);
}

string DataArraySlice::describe() {
	ostringstream buf;
	buf << _parent->describe() << '[' << _offset << ':' << _size << ']';
	return buf.str();
}

BinaryMapDataWriter::BinaryMapDataWriter(void) : stringOut(&dataStream)
{
}


BinaryMapDataWriter::~BinaryMapDataWriter(void)
{
}
