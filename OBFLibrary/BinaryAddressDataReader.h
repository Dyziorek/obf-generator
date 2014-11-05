#pragma once


namespace gp = google::protobuf;
namespace gio = google::protobuf::io;


struct MapAddresBlock;


class BinaryAddressDataReader
{
public:
	enum ObfAddressBlockType
    {
        CitiesOrTowns = 1,
        Villages = 3,
        Postcodes = 2,
    };

public:
	BinaryAddressDataReader(void);
	~BinaryAddressDataReader(void);

	void ReadMapAddresses(gio::CodedInputStream* cis);
	void readCityInfo(gio::CodedInputStream* cis, std::shared_ptr<MapAddresBlock>& block);
private:
	std::string name;
	std::string enName;
	
	std::list<std::shared_ptr<MapAddresBlock>> blocksInfo;
};

struct MapAddresBlock
{
	BinaryAddressDataReader::ObfAddressBlockType _type;
	uint32_t _offset;
	uint32_t _length;
};
