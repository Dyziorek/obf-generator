#pragma once


namespace gio = google::protobuf::io;


typedef unsigned char uint8;

class BinaryIndexDataReader
{
public:
	BinaryIndexDataReader(boost::filesystem::path& pathInfo);
	virtual ~BinaryIndexDataReader(void);

	BinaryMapDataReader& GetReader() {return reader;}
	void getMapObjects(boxI& areaCheck, int zoom, std::list<std::shared_ptr<MapObjectData>>& outList);
private:
	std::shared_ptr<gio::CodedInputStream> strmData;
	std::shared_ptr<RandomAccessFileReader> fileReader;
	void ReadMapData(google::protobuf::io::CodedInputStream* cis);
	void ReadRouteData(google::protobuf::io::CodedInputStream* cis);
	void ReadAddresIndex(google::protobuf::io::CodedInputStream* cis);
	BinaryMapDataReader reader;
	BinaryAddressDataReader addresser;
	BinaryRouteDataReader router;
};

