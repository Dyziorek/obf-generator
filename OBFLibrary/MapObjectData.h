#pragma once
class MapObjectData
{
public:
	MapObjectData(void);
	~MapObjectData(void);
};


struct MapDecodingRule
{
    uint32_t type;

    std::string tag;
    std::string value;
};