#include "stdafx.h"
#include "BlockReader.h"
#include "portable_binary_iarchive.hpp"
#include "proto\fileformat.pb.h"
#include "proto\osmformat.pb.h"
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include "boost/iostreams/device/array.hpp"
#include "boost/iostreams/copy.hpp"
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include "zlib.h"


namespace io = boost::iostreams;
namespace ar = boost::archive;


BlockReader::BlockReader(void)
{
	
}


BlockReader::~BlockReader(void)
{
}

BlockHeader BlockReader::ReadBlockHead(ar::binary_iarchive& strmData)
{
	int blockSize = INT_MAX;
	BlockHeader header;
	try
	{
	ar::binary_iarchive& arc = strmData;

	arc >> blockSize;

	char * cptr = reinterpret_cast<char *>(&blockSize);
	reverse_bytes(sizeof(blockSize), cptr);

	if (blockSize > 65536)
	{
		return BlockHeader();
	}

	std::vector<BYTE>buffer(blockSize);

	arc.load_binary((char*)&buffer.front(), blockSize);
	

	header.ParseFromArray((void*)&buffer.front(), blockSize);

	}
	catch (ar::archive_exception arcEx)
	{
	}

	//typedef io::basic_array_source<char> Device;
	//io::stream<Device> strReader();
	//std::istream strInput(strReader.rdbuf());
	


	return header;
}

Blob BlockReader::ReadContents(BlockHeader& infoData, ar::binary_iarchive& strmData)
{
	ar::binary_iarchive& arc = strmData;
	std::vector<BYTE>buffer(infoData.datasize());

	arc.load_binary((char*)&buffer.front(), infoData.datasize());

	Blob blobData;
	blobData.ParseFromArray((void*)&buffer.front(), infoData.datasize());

	return blobData;
}

HeaderBlock BlockReader::GetHeaderContents(Blob& info)
{
	HeaderBlock block;
	if (info.has_raw())
	{
		std::vector<BYTE>buffer(info.raw().size());
		std::copy(info.raw().begin(), info.raw().end(), buffer.begin());
		block.ParseFromArray((void*)&buffer.front(), buffer.size());
	}
	else if (info.has_zlib_data())
	{
		std::vector<char> decompressed;
		io::filtering_istream is;
		is.push(io::zlib_decompressor());
		is.push(io::array_source(&info.zlib_data().front(), info.zlib_data().size()));
		io::copy(is,io::back_inserter(decompressed));
		
		block.ParseFromArray((void*)&decompressed.front(), decompressed.size());
		
	}
	return block;
}

PrimitiveBlock BlockReader::GetBlockContents(Blob& info)
{
	PrimitiveBlock block;
	if (info.has_raw())
	{
		std::vector<BYTE>buffer(info.raw().size());
		std::copy(info.raw().begin(), info.raw().end(), buffer.begin());
		block.ParseFromArray((void*)&buffer.front(), buffer.size());
	}
	else if (info.has_zlib_data())
	{
		std::vector<char> decompressed;
		io::filtering_istream is;
		is.push(io::zlib_decompressor());
		is.push(io::array_source(&info.zlib_data().front(), info.zlib_data().size()));
		io::copy(is,io::back_inserter(decompressed));
		
		block.ParseFromArray((void*)&decompressed.front(), decompressed.size());
		
	}
	return block;
}