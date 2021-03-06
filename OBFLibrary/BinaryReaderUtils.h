#pragma once

namespace gio = google::protobuf::io;
namespace wfl = google::protobuf::internal;

typedef unsigned char uint8;

class BinaryReaderUtils
{
public:
	BinaryReaderUtils(void);
	~BinaryReaderUtils(void);

	static google::protobuf::uint32 readBigEndianInt( google::protobuf::io::CodedInputStream* cis);
	
	static void readStringTable( gio::CodedInputStream* cis, std::vector<std::string>& stringTableOut );
	static bool readString( gio::CodedInputStream* cis, std::string& output );
	static bool readSInt32( gio::CodedInputStream* cis, int32_t& output );
	static int64_t readSInt64( gio::CodedInputStream* cis );
	static void skipUnknownField( google::protobuf::io::CodedInputStream* cis, int tag );

};

