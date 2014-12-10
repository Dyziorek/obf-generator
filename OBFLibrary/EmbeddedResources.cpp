#include "stdafx.h"
#include "EmbeddedResources.h"
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/stream_buffer.hpp>
#include <boost/iostreams/filtering_stream.hpp>
#include <boost/iostreams/device/array.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>
#include <boost/iostreams/device/back_inserter.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/iostreams/filter/zlib.hpp>

namespace io = boost::iostreams;

struct EmbeddedResource
{
    std::string name;
    size_t size;
    const uint8_t* data;
};

#include "EmbeddedResources_bundle.cpp"

EmbeddedResources::EmbeddedResources(void)
{
}


EmbeddedResources::~EmbeddedResources(void)
{
}


std::vector<char> EmbeddedResources::getDataFromResource(std::string name)
{
	auto strBuff = getRawFromResource(name);
	if (strBuff.size() > 0)
	{
		std::vector<char> decompressed;
		io::filtering_istream is;
		is.push(io::zlib_decompressor());
		is.push(io::array_source(strBuff.data(), strBuff.size()));
		io::copy(is,io::back_inserter(decompressed));
		return decompressed;	
	}
	return std::vector<char>();
}

std::vector<char> EmbeddedResources::getRawFromResource(std::string name)
{
	for (int resIdx = 0; resIdx != __bundled_resources_count; resIdx++)
	{
		if ( __bundled_resources[resIdx].name == name)
		{
			std::vector<char> valueSet;
			valueSet.resize( __bundled_resources[resIdx].size - 4);
			memcpy_s(valueSet.data(),  __bundled_resources[resIdx].size - 4,  __bundled_resources[resIdx].data + 4,  __bundled_resources[resIdx].size - 4);
			return valueSet;
		}
	}
	return std::vector<char>();
}