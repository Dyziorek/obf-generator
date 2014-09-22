#pragma once
class MapDataGenerator
{
public:
	MapDataGenerator(void);
	~MapDataGenerator(void);
	bool GenerateTileImage31(int64_t tileID, SkBitmap& tileImage, std::vector<std::shared_ptr<MapObjectData>>& allMapObjets);
	bool GenerateTileImage(int64_t tileID, int zoom, SkBitmap& tileImage, std::vector<std::shared_ptr<MapObjectData>>& allMapObjets);

private:
	

};

