#pragma once


namespace gio = google::protobuf::io;
namespace obf = OsmAnd::OBF;
namespace wfl = google::protobuf::internal;

typedef unsigned char uint8;

class BinaryIndexDataReader
{
public:
	BinaryIndexDataReader(RandomAccessFileReader* outData);
	virtual ~BinaryIndexDataReader(void);



private:
	RandomAccessFileReader* rad;
	gio::CodedInputStream strmData;
	void skipUnknownField( gio::CodedInputStream* cis, int tag );
	google::protobuf::uint32 readBigEndianInt(gio::CodedInputStream* cis );
};

