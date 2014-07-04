#pragma once

namespace gio = google::protobuf::io;
namespace wfl = google::protobuf::internal;

typedef unsigned char uint8;

class RandomAccessFileReader : boost::noncopyable, public gio::ZeroCopyInputStream
{
private:
	HANDLE _fd;
	HANDLE _hmapfd;

	boost::filesystem::path _path;
	unsigned __int64 filePointer;
	uint64_t _size;

	bool unmap();
	uint8* map(unsigned __int64 position,unsigned __int64* newSize);
public:
	enum Mode {
	READ = 1,
	WRITE = 2,
	READWRITE = READ|WRITE,
	};

public:
	RandomAccessFileReader(void);
	RandomAccessFileReader(const boost::filesystem::path& path, RandomAccessFileReader::Mode mode = READ, uint64_t size = 0);
	~RandomAccessFileReader(void);

	/**
	 * Open the given file. May throw std::ios_base::failure.
	 *
	 * non-zero size means open and create file of this size
	 */
	void open(const boost::filesystem::path& path, RandomAccessFileReader::Mode mode = READ, uint64_t size = 0);
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

	// gio::ZeroCopyInputStream virtual overides those

	bool Next(const void** data, int* size);

    void BackUp(int count);

	bool Skip(int count);

  // Returns the total number of bytes read since this object was created.
	__int64 ByteCount() const;

private:
	static unsigned __int64 localMemoryBufferLimit;
	int pageSize;
	gio::CodedInputStream* codeWork;
	uint8* _currentBuffer;
	uint8* _currentMapBuffer;
	GOOGLE_DISALLOW_EVIL_CONSTRUCTORS(RandomAccessFileReader);
};


