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
	google::protobuf::io::CodedInputStream strmData;
	void skipUnknownField( google::protobuf::io::CodedInputStream* cis, int tag );
};

