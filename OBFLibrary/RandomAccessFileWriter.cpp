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
#include "RandomAccessFileWriter.h"
#include <boost/assert.hpp>
#include <boost/filesystem.hpp>
#include <fcntl.h>
#include <ios>
#include <sstream>
#include <sys/stat.h>
#include "ArchiveIO.h"

using namespace std;

namespace bsys = boost::system;
namespace fs = boost::filesystem;




RandomAccessFileWriter::RandomAccessFileWriter() :
	_path(""), _size(0), implData(nullptr), filePointer(0)
{
	_fd = INVALID_HANDLE_VALUE;
	codeWork = nullptr;
}

RandomAccessFileWriter::RandomAccessFileWriter(const boost::filesystem::path& path, RandomAccessFileWriter::Mode mode, uint64_t size) :
	_path(""), _size(0), implData(&implCopy), filePointer(0)
{
	codeWork = nullptr;
	_fd = INVALID_HANDLE_VALUE;
	open(path, mode, size);
	implCopy.AssignHandle(_fd);
	implCopy.SetParent(this);
}

RandomAccessFileWriter::~RandomAccessFileWriter()
{
	if (is_open()) {
		implData.Flush();
		close();
	}
}

RandomAccessFileWriter::CopyingFileOutputStream::CopyingFileOutputStream()
  : close_on_delete_(false),
    is_closed_(false),
	errno_(0)
{
}

RandomAccessFileWriter::CopyingFileOutputStream::~CopyingFileOutputStream() {
  if (close_on_delete_) {
    if (!Close()) {
		char errBuff[100];
      GOOGLE_LOG(ERROR) << "close() failed: " << strerror_s<100>(errBuff, errno_);
    }
  }
}

bool RandomAccessFileWriter::CopyingFileOutputStream::Close() {
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

bool RandomAccessFileWriter::CopyingFileOutputStream::Write(
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


void RandomAccessFileWriter::open(const boost::filesystem::path& path, RandomAccessFileWriter::Mode mode, uint64_t size)
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
		fp = SetFilePointer(_fd, (LONG)size, NULL, FILE_BEGIN);
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

void RandomAccessFileWriter::close()
{
	if (is_open()) {
		::CloseHandle(_fd);
		_fd = INVALID_HANDLE_VALUE;
		_path = "";
	}
}

bool RandomAccessFileWriter::is_open() const
{
	return _fd != INVALID_HANDLE_VALUE;
}

__int64 RandomAccessFileWriter::seek(__int64 newPos)
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

__int64 RandomAccessFileWriter::ByteCount() const
{
	return implData.ByteCount();
}

uint32_t RandomAccessFileWriter::blocks(size_t blockSize) const
{
	// Round up the number of blocks
	return (ByteCount() + blockSize - 1) / blockSize;
}

void RandomAccessFileWriter::BackUp ( int count) {
	
	implData.BackUp(count);
	//SetFilePointer(_fd, -count, NULL, FILE_CURRENT);
	//filePointer -= count;
}

bool RandomAccessFileWriter::Next(void** src, int* size)
{
	bool result = implData.Next(src, size);
	_currentBuffer = (uint8*)*src;
	return result;
}

size_t RandomAccessFileWriter::writeInt(int val)
{
	DWORD written;
	#ifndef BOOST_BIG_ENDIAN
		reverse_bytes(sizeof(val), (char*)&val);
	#endif
	WriteFile(_fd, &val, sizeof(val), &written, NULL);
	filePointer += written;
	return written;
}

string RandomAccessFileWriter::describe() {
	return _path.string();
}

const boost::filesystem::path& RandomAccessFileWriter::path() const
{
	return _path;
}
