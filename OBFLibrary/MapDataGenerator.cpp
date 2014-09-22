#include "stdafx.h"
#include "SkCanvas.h"
#include "SkColor.h"
#include "SkSurface.h"
#include "SkImage.h"
#include "SkData.h"
#include "SkStream.h"
#include "SkGraphics.h"
#include "SkBitmap.h"
#include "SkBitmapDevice.h"
#include "MapObjectData.h"
#include "MapDataGenerator.h"


MapDataGenerator::MapDataGenerator(void)
{
}


MapDataGenerator::~MapDataGenerator(void)
{
}


bool MapDataGenerator::GenerateTileImage31(int64_t tileID, SkBitmap& tileImage, std::vector<std::shared_ptr<MapObjectData>>& allMapObjets)
{
	SkBitmapDevice devData(tileImage);
	SkCanvas mapData(&devData);
	
	mapData.clear(SK_ColorBLACK);
	auto dZoom = MapUtils::getPowZoom(31);

	return false;
}

bool MapDataGenerator::GenerateTileImage(int64_t tileID, int zoom, SkBitmap& tileImage, std::vector<std::shared_ptr<MapObjectData>>& allMapObjets)
{

	return false;
}
