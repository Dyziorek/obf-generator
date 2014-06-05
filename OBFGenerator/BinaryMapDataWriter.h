#pragma once
#include <google\protobuf\io\coded_stream.h>
#include <google\protobuf\io\zero_copy_stream_impl_lite.h>
#include <google\protobuf\io\zero_copy_stream_impl.h>
#include <boost\container\slist.hpp>


#include <boost/filesystem/path.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>





class IDataArray {
public:
	typedef boost::shared_ptr<IDataArray> Ptr;

	virtual uint64_t size() const = 0;

	/**
	 * Reads up to /size/ bytes from file and returns amount read.
	 *
	 * @arg offset - to read from
	 * @arg buf - a buffer allocated with at least /size/ capacity
	 * @arg size - size of buf
	 * @returns pointer to data, may or may not be pointer to /buf/
	 */
	virtual size_t read(uint64_t offset, size_t size, byte* buf) const = 0;

	/**
	 * Writes /size/ bytes to file beginning at /offset/.
	 */
	virtual size_t write(uint64_t offset, const void* src, size_t size) = 0;

	/**
	 * Writes a given string-buf to file beginning at /offset/.
	 */
	virtual size_t write(uint64_t offset, const std::string& buf);

	/**
	 * Describe the DataArray I.E. the name of the file
	 */
	virtual std::string describe() = 0;
};

std::string dataArrayToString(const IDataArray& dataarray);

class RandomAccessFile : boost::noncopyable, public IDataArray {
	HANDLE _fd;
	boost::filesystem::path _path;
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

	/**
	 * The number of bytes in the open file
	 */
	virtual uint64_t size() const;

	/**
	 * The number of blocks of /blockSize/ required to hold all file content
	 */
	uint32_t blocks(size_t blockSize) const;

	/// Implement IDataArray
	virtual size_t read(uint64_t offset, size_t size, byte* buf) const;
	virtual size_t write(uint64_t offset, const void* src, size_t size);
	virtual std::string describe();

	/**
	 * Return the path used to open the file
	 */
	const boost::filesystem::path& path() const;
};

class DataArraySlice : boost::noncopyable, public IDataArray {
	IDataArray::Ptr _parent;
	uint64_t _offset, _size;
public:
	DataArraySlice(const IDataArray::Ptr& parent, uint64_t offset, uint64_t size);
	DataArraySlice(const IDataArray::Ptr& parent, uint64_t offset);
	virtual uint64_t size() const;
	virtual size_t read ( uint64_t offset, size_t size, byte* buf ) const;
	virtual size_t write ( uint64_t offset, const void* src, size_t size );
    virtual std::string describe();
};





class BinaryMapDataWriter
{
public:
	BinaryMapDataWriter(void);
	~BinaryMapDataWriter(void);

	//boost::container::slist<FileRe

	google::protobuf::io::OstreamOutputStream stringOut;
	std::stringstream dataStream;
};

