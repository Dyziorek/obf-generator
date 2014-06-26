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

	static int readBigEndianInt( google::protobuf::io::CodedInputStream* cis);
	
	static void readStringTable( gio::CodedInputStream* cis, std::list<std::string>& stringTableOut );
	static bool readString( gio::CodedInputStream* cis, std::string& output );
	static void skipUnknownField( google::protobuf::io::CodedInputStream* cis, int tag );

	const BinaryMapDataReader& GetReader() const {return reader;}
private:
	RandomAccessFileReader* rad;
	gio::CodedInputStream strmData;
	
	void ReadMapData(google::protobuf::io::CodedInputStream* cis);
	BinaryMapDataReader reader;
};

