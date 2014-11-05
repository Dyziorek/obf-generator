#pragma once

#include "ArchiveIO.h"

namespace gio = google::protobuf::io;
namespace wfl = google::protobuf::internal;

typedef unsigned char uint8;

class RandomAccessFileWriter : boost::noncopyable, public gio::ZeroCopyOutputStream {
private:
	HANDLE _fd;
	boost::filesystem::path _path;
	unsigned __int64 filePointer;
	uint64_t _size;
public:
	enum Mode {
	READ = 1,
	WRITE = 2,
	READWRITE = READ|WRITE,
	};

	RandomAccessFileWriter();
	RandomAccessFileWriter(const boost::filesystem::path& path, RandomAccessFileWriter::Mode mode = READ, uint64_t size = 0);
	~RandomAccessFileWriter();

	/**
	 * Open the given file. May throw std::ios_base::failure.
	 *
	 * non-zero size means open and create file of this size
	 */
	void open(const boost::filesystem::path& path, RandomAccessFileWriter::Mode mode = READ, uint64_t size = 0);

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
	void SetParent(RandomAccessFileWriter* extObj) {parentObj = extObj;}
   private:
    // The file descriptor.
    HANDLE file_;
    bool close_on_delete_;
    bool is_closed_;
	RandomAccessFileWriter* parentObj;
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
	int writeReference(RandomAccessFileWriter& raf, __int64 pointerToCalculateShifTo)  {
		this->pointerToCalculateShiftTo = pointerToCalculateShifTo;
		__int64 currentPosition = raf.getFilePointer();
		int val = -1;
		int retValue = -1;
		if (currentPosition < pointerToWrite)
		{
			// cannot seek back to file, still in the buffer before write
			uint8* beginBuffer = raf._currentBuffer;
			if (raf.ByteCount() > pointerToWrite)
			{
				// it should be that if not this is error
				val = (int) (pointerToCalculateShiftTo - pointerToCalculateShiftFrom);
				retValue = val;
				#ifndef BOOST_BIG_ENDIAN
					reverse_bytes(sizeof(val),(char*)&val);
				#endif

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
			retValue = val;
			raf.writeInt(val);
			raf.seek(currentPosition);
		}
		return retValue;
	}

	
	static BinaryFileReference createSizeReference(__int64 pointerToWrite){
		return BinaryFileReference(pointerToWrite, pointerToWrite + 4);
	}
	
	static BinaryFileReference* createShiftReference(__int64 pointerToWrite, __int64 pointerShiftFrom){
		return new BinaryFileReference(pointerToWrite, pointerShiftFrom);
	}
	
public:
	int l, r, t, b;

};
