#pragma once
#include <google\protobuf\io\coded_stream.h>
#include <google\protobuf\io\zero_copy_stream_impl_lite.h>
#include <google\protobuf\io\zero_copy_stream_impl.h>
#include <boost\container\slist.hpp>

class BinaryMapDataWriter
{
public:
	BinaryMapDataWriter(void);
	~BinaryMapDataWriter(void);

	//boost::container::slist<FileRe

	google::protobuf::io::OstreamOutputStream stringOut;
	std::stringstream dataStream;
};

