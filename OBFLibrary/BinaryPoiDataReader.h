#pragma once

namespace gp = google::protobuf;
namespace gio = google::protobuf::io;

struct AmenityPoi
{
    uint64_t id;
    std::string name;
    std::string latinName;
    pointI point31;
    std::string openingHours;
    std::string site;
    std::string phone;
    std::string description;
    uint32_t categoryId;
    uint32_t subcategoryId;
};

struct AmenityCategory
{
	std::string name;
	std::vector<std::string> subNames;
};

struct AmenityPoint
{
	uint32_t zoom;
	uint32_t x;
	uint32_t y;
	uint32_t hash;
	uint32_t offset;
};

class BinaryPoiDataReader
{
public:
	BinaryPoiDataReader(void);
	~BinaryPoiDataReader(void);

	void ReadPoiDataInfo(gio::CodedInputStream* gis, RandomAccessFileReader* outData);
	void ReadBoundsBox(gio::CodedInputStream* cis);
	void ReadCategories(gio::CodedInputStream* cis);
	void ReadAmenities(gio::CodedInputStream* cis, std::vector<std::shared_ptr<AmenityPoi>>& amCom, std::shared_ptr<AreaI>& bbox, std::set<uint64_t>& neededCategories,uint32_t zoom, uint32_t zoomDepth);
	void ReadAmenity(gio::CodedInputStream* gis, std::vector<std::shared_ptr<AmenityPoi>>& amCom, std::shared_ptr<AreaI>& bbox, std::set<uint64_t>& neededCategories,uint32_t zoom, uint32_t zoomDepth);
	bool ReadTileBox(gio::CodedInputStream* cis, std::shared_ptr<AreaI>& bbox, std::set<uint64_t>& neededCategories,uint32_t zoom, uint32_t zoomDepth, std::vector<std::shared_ptr<AmenityPoint>>& poiPoints, AmenityPoint* parentPoi, std::set<uint64_t>* skippedPoi);
	bool checkCategories(gio::CodedInputStream* cis, std::set<uint64_t>& neededCategories);
	void ReadAmenitiesData(gio::CodedInputStream* cis, std::vector<std::shared_ptr<AmenityPoi>>& amCom, std::shared_ptr<AreaI>& bbox, std::set<uint64_t>& neededCategories, uint32_t zoom, uint32_t zoomDepth);
	void ReadAmenity(gio::CodedInputStream* cis,uint32_t zoom, std::shared_ptr<AmenityPoi>& amenity, std::set<uint64_t>& neededCategories,std::shared_ptr<AreaI>& bbox, pointI& pt);
	AreaD poiBBox;
	std::string poiName;
	std::vector<AmenityCategory> categories;
private:
	enum {
            SubcategoryIdShift = 7,
            CategoryIdMask = (1u << SubcategoryIdShift) - 1,
        };
	int _offset;
};

