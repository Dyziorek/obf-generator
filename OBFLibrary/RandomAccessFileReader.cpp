#include "stdafx.h"
#include <google\protobuf\descriptor.h>
#include <google\protobuf\io\coded_stream.h>
#include <google\protobuf\io\zero_copy_stream_impl_lite.h>
#include <google\protobuf\io\zero_copy_stream_impl.h>
#include <google\protobuf\wire_format_lite.h>
#include <boost\container\slist.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>

#include "RandomAccessFileReader.h"
#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <fcntl.h>
#include <ios>
#include <sstream>
#include <sys/stat.h>

using namespace std;

namespace bsys = boost::system;
namespace fs = boost::filesystem;
namespace bio = boost::iostreams;

unsigned __int64 RandomAccessFileReader::localMemoryBufferLimit = 16 * 1024 * 1024; // 16 mbytes

RandomAccessFileReader::RandomAccessFileReader(void)
{
}


RandomAccessFileReader::~RandomAccessFileReader(void)
{
	if (is_open())
	{
		unmap();
		close();
	}
}

RandomAccessFileReader::RandomAccessFileReader(const boost::filesystem::path& path, RandomAccessFileReader::Mode mode/* = READ*/, uint64_t size /*= 0*/) :
	_path(path), _size(0), filePointer(0), _mode(mode)
{
	codeWork = nullptr;
	_currentBuffer = nullptr;
	_currentMapBuffer = nullptr;

	_fd = INVALID_HANDLE_VALUE;
	pageSize = 0;
	//open(_path, mode, size);

}

void RandomAccessFileReader::open(const boost::filesystem::path& path, RandomAccessFileReader::Mode mode, uint64_t size)
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
		_fd =  ::CreateFile(pathName.c_str(), m, 0 , NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL , NULL);
	}
	else
	{
		return;
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
	}
	
	_path = path;
	_size = size;

	 SYSTEM_INFO info;
    ::GetSystemInfo(&info);
    pageSize = static_cast<int>(info.dwAllocationGranularity);

}


bool RandomAccessFileReader::is_open() const
{
	return _fd != INVALID_HANDLE_VALUE;
}

void RandomAccessFileReader::close()
{
	if (is_open()) {
		::CloseHandle(_fd);
		_fd = INVALID_HANDLE_VALUE;
		_path = "";
	}
}

bool RandomAccessFileReader::unmap()
{
	if (_currentMapBuffer != nullptr)
	{
		BOOL bRet = UnmapViewOfFile(_currentMapBuffer);
		if (bRet)
		{
			_currentMapBuffer = nullptr;
			return CloseHandle(_hmapfd);
		}
	}
	return false;
}


uint8* RandomAccessFileReader::map(unsigned __int64 position, unsigned __int64* newSize)
{
	if (!is_open())
	{
		open(_path, _mode, 0);
	}

	auto mappedSize = *newSize;
    if(filePointer + mappedSize >= _size)
        mappedSize = _size - filePointer;
    
	LARGE_INTEGER positioner;
	positioner.HighPart = position >> 32;
	positioner.LowPart = position & (unsigned __int64)0xFFFFFFFF;
	//positioner.QuadPart = (position / pageSize) * pageSize;
	unsigned __int64 iViewDelta = position & (pageSize - 1);

	if (iViewDelta)
	{
		positioner.LowPart = positioner.LowPart & ~(pageSize - 1);
	}

	_hmapfd = CreateFileMapping(_fd, NULL, PAGE_READONLY, 0, 0, NULL);
	if (_hmapfd != nullptr)
	{
		void* data = nullptr;
		if (mappedSize < localMemoryBufferLimit)
		{
			data = MapViewOfFile(_hmapfd, FILE_MAP_READ, positioner.HighPart, positioner.LowPart, iViewDelta + mappedSize);
		}
		else
		{
			data = MapViewOfFile(_hmapfd, FILE_MAP_READ, positioner.HighPart, positioner.LowPart, mappedSize + iViewDelta);
		}
		if (data != nullptr)
		{
			uint8* initialOffset = static_cast<uint8*>(data);
			_currentMapBuffer = static_cast<uint8*>(data);
			*newSize = mappedSize;
			return initialOffset + iViewDelta;
		}
		else
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
				::CloseHandle(_fd);
				ostringstream buf;
				buf << "Failed with reason " << (const char*)lpMsgBuf << std::endl;
				LocalFree(lpMsgBuf);
				throw bsys::system_error(bsys::errc::make_error_code(static_cast<bsys::errc::errc_t>(errno)), buf.str());
		}
	}
	else
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
	}
	return NULL;
}

__int64 RandomAccessFileReader::ByteCount() const
{
	return filePointer;
}


void RandomAccessFileReader::BackUp ( int count) {
	
	if(count > filePointer)
        filePointer = 0;
    else
        filePointer -= count;
}

bool RandomAccessFileReader::Next(const void** src, int* size)
{
	if (_currentMapBuffer != nullptr)
	{
		unmap();
	}
    
	if (is_open())
	{
		// Check if current position is in valid range
		if(filePointer < 0 || filePointer >= _size)
		{
			*src = nullptr;
			*size = 0;
			return false;
		}
	}


    // Map new portion of data
    auto mappedSize = localMemoryBufferLimit;

	_currentBuffer = map(filePointer, &mappedSize);

    // Check if memory was mapped successfully
    if(!_currentBuffer)
    {
        *src = nullptr;
        *size = 0;
        return false;
    }
    else
    {
        filePointer += mappedSize;

        *src = _currentBuffer;
        *size = (int)mappedSize;
        return true;
	}

	return true;
}

bool RandomAccessFileReader::Skip(int count)
{
	if(filePointer + count >= _size)
	{
		filePointer = _size;
		return false;
	}
	else
	{
		filePointer += count;
		return true;
	}
}