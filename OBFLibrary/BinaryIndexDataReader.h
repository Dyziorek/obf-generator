#pragma once


namespace gio = google::protobuf::io;
namespace obf = OsmAnd::OBF;
namespace wfl = google::protobuf::internal;

typedef unsigned char uint8;

class BinaryIndexDataReader
{
public:
	BinaryIndexDataReader(boost::filesystem::path& pathInfo);
	virtual ~BinaryIndexDataReader(void);

	BinaryMapDataReader& GetReader() {return reader;}
private:
	RandomAccessFileReader* rad;
	std::shared_ptr<gio::CodedInputStream> strmData;
	std::shared_ptr<RandomAccessFileReader> fileReader;
	void ReadMapData(google::protobuf::io::CodedInputStream* cis);
	void ReadRouteData(google::protobuf::io::CodedInputStream* cis);
	void ReadAddresIndex(google::protobuf::io::CodedInputStream* cis);
	BinaryMapDataReader reader;
	BinaryAddressDataReader addresser;
	BinaryRouteDataReader router;
};

