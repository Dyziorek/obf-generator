#include "stdafx.h"
#include "SkCanvas.h"
#include <google\protobuf\io\coded_stream.h>
#include <boost/filesystem.hpp>
#include "MapObjectData.h"
#include "MapStyleData.h"
#include "MapStyleChoiceValue.h"
#include "DefaultMapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleInfo.h"
#include "MapStyleEval.h"

#include "RandomAccessFileReader.h"
#include "MapObjectData.h"
#include "BinaryMapDataReader.h"
#include "BinaryAddressDataReader.h"
#include "BinaryRouteDataReader.h"
#include "BinaryIndexDataReader.h"
#include <boost/detail/endian.hpp>
#include "BinaryReaderUtils.h"
#include "MapRasterizer.h"
#include "MapRasterizerContext.h"
#include "MapRasterizerProvider.h"

#include "Tools.h"

#include "Sensorsapi.h"

namespace bf = boost::filesystem;

MapRasterizerProvider::MapRasterizerProvider(void)
{
	workingStyle.reset(new MapStyleInfo());
	workingStyle->loadRenderStyles(nullptr);

	_orderEval = std::shared_ptr<MapStyleEval>(new MapStyleEval(workingStyle, 1.0f));
	applyStyle(_orderEval);

	_pointEval = std::shared_ptr<MapStyleEval>(new MapStyleEval(workingStyle, 1.0f));
	_lineEval = std::shared_ptr<MapStyleEval>(new MapStyleEval(workingStyle, 1.0f));
	_polyEval = std::shared_ptr<MapStyleEval>(new MapStyleEval(workingStyle, 1.0f));
	_textEval = std::shared_ptr<MapStyleEval>(new MapStyleEval(workingStyle, 1.0f));

}


MapRasterizerProvider::~MapRasterizerProvider(void)
{
}

void MapRasterizerProvider::obtainMaps(const char* path)
{
	bf::path& fileInfo = bf::path(std::string(path));

	if ( bf::is_regular_file(fileInfo))
	{
		mapProviders.push_back(std::shared_ptr<BinaryIndexDataReader>(new BinaryIndexDataReader(fileInfo)));
	}
	else if (bf::is_directory(fileInfo))
	{
		bf::directory_iterator dirEntry = bf::directory_iterator(fileInfo);
		std::vector<bf::directory_entry> entries;
		std::for_each(dirEntry, bf::directory_iterator(), [&entries](bf::directory_entry input)
		{
			auto checker = input;
			if (checker.path().extension().generic_string() == std::string("pbf"))
			{
				entries.push_back(input);
			}
		});
		for (auto mapPath : entries)
		{
			auto& pathData = (bf::path&)mapPath.path();
			mapProviders.push_back(std::shared_ptr<BinaryIndexDataReader>(new BinaryIndexDataReader(pathData)));
		}
		
	}

}

std::list<std::shared_ptr<MapObjectData>> MapRasterizerProvider::obtainMapData(boxI& areaI, int zoom)
{
	std::list<std::shared_ptr<MapObjectData>> outListData;

	

	for (auto& mapData : mapProviders)
	{
		mapData->getMapObjects(areaI, zoom, outListData);

	}

	return outListData;
}


boxI MapRasterizerProvider::getWholeBox()
{
	boxI baseBox;
	bg::assign_inverse(baseBox);

	for (auto& mapDataItem : mapProviders)
	{
		for (auto& sectData : mapDataItem->GetReader().getSections())
		{
			bg::expand(baseBox, std::get<0>(sectData));
		}
	}

	return baseBox;
}

void MapRasterizerProvider::applyStyle(std::shared_ptr<MapStyleEval>& evalData)
{
	// no initial settings later on.
}

bool MapRasterizerProvider::obtainMapPrimitives(std::list<std::shared_ptr<MapObjectData>>& mapData, int zoom,  std::shared_ptr<MapRasterizerContext>& _context)
{
	auto& builtinDef = workingStyle->getDefaultValueDefinitions();

	_orderEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);
	_orderEval->setIntValue(builtinDef->id_INPUT_MINZOOM, zoom);

	_pointEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);
	_pointEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);

	_lineEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);
	_lineEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);

	_polyEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);
	_polyEval->setIntValue(builtinDef->id_INPUT_MAXZOOM, zoom);

	
	for (std::shared_ptr<MapObjectData> objectMapData: mapData)
	{
		std::shared_ptr<GraphicElementGroup> graphGroup(new GraphicElementGroup());
		int typeRuleIDIndex = 0;
		for(int typeRuleId : objectMapData->type)
		{
			auto& mapDecoder = objectMapData->section->rules->getRuleInfo(typeRuleId);
			
			_orderEval->setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
			_orderEval->setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);
			_orderEval->setIntValue(builtinDef->id_INPUT_LAYER, objectMapData->getSimpleLayerValue());
			_orderEval->setBoolValue(builtinDef->id_INPUT_AREA, objectMapData->isArea);
			_orderEval->setBoolValue(builtinDef->id_INPUT_CYCLE, objectMapData->isClosedFigure());
			_orderEval->setBoolValue(builtinDef->id_INPUT_POINT, objectMapData->points.size() == 1);

			MapStyleResult orderResults;
			bool bOK = _orderEval->evaluate(objectMapData, rulesetType::order, &orderResults);
			if (!bOK)
				continue;

			int graphicType = -1;
			bOK = orderResults.getIntVal(builtinDef->id_OUTPUT_OBJECT_TYPE, graphicType);
			if (!bOK)
				continue;
			int zOrder = -1;
			bOK = orderResults.getIntVal(builtinDef->id_OUTPUT_ORDER, zOrder);
			if (!bOK)
				continue;

			std::shared_ptr<GraphicElement> graphicElement(new GraphicElement(graphGroup, objectMapData, static_cast<GraphElementType>(graphicType), typeRuleIDIndex));
			graphicElement->zOrder = zOrder;

			if(graphicElement->_type == GraphElementType::Polygons)
			{
				if (objectMapData->points.size() <=2)
				{
					continue;
				}

				if (!objectMapData->isClosedFigure())
				{
					// not a correct polygon cannot be filled figure
					continue;
				}
				if (!objectMapData->isClosedFigure(true))
				{
					// not a correct polygon cannot be filled figure withour proper inner data
					continue;
				}

				_polyEval->setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
				_polyEval->setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);

				std::shared_ptr<MapStyleResult> polyResults(new MapStyleResult());
				bOK = _polyEval->evaluate(objectMapData, rulesetType::polygon, polyResults.get());
				if (!bOK)
					continue;
				
				graphicElement->styleResult = polyResults;

				auto areaData = Tools::polygonArea(objectMapData->points);
				if (areaData < /*MapRasterizer::PolygonAreaCutoffLowerThreshold*/ 75)
				{
					// polygon too small to show as polygon in low zoom so convert to point in low zooms
					graphicElement->zOrder += 1.0 / areaData;
					std::shared_ptr<GraphicElement> pointElement(new GraphicElement(graphGroup, objectMapData, GraphElementType::Points, typeRuleIDIndex));
					pointElement->zOrder = graphicElement->zOrder;
					graphGroup->_polygons.push_back(graphicElement);

					_pointEval->setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
					_pointEval->setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);
					std::shared_ptr<MapStyleResult> pointResults(new MapStyleResult());
					bOK = _pointEval->evaluate(objectMapData, rulesetType::point, pointResults.get());
					if (bOK)
					{
						// indicates it has icon
						pointElement->styleResult = pointResults;
					}
					if (typeRuleIDIndex == 0 || objectMapData->nameTypeString.empty())
					{
						graphGroup->_points.push_back(pointElement);
					}
				}

			}
			else if (graphicElement->_type == GraphElementType::Polylines)
			{
				if (objectMapData->points.size() < 2)
				{
					continue;
				}
				_lineEval->setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
				_lineEval->setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);
				_lineEval->setIntValue(builtinDef->id_INPUT_LAYER, objectMapData->getSimpleLayerValue());
				std::shared_ptr<MapStyleResult> lineResults(new MapStyleResult());
				bOK = _lineEval->evaluate(objectMapData, rulesetType::point, lineResults.get());
				if (!bOK)
					continue;

				graphicElement->styleResult = lineResults;

				graphGroup->_polyLines.push_back(graphicElement);
			}
			else if(graphicElement->_type == GraphElementType::Points)
			{
				if (objectMapData->points.size() < 1)
				{
					// no point defined
					continue;
				}
				_pointEval->setStringValue(builtinDef->id_INPUT_TAG, mapDecoder.tag);
				_pointEval->setStringValue(builtinDef->id_INPUT_VALUE, mapDecoder.value);
				std::shared_ptr<MapStyleResult> pointResults(new MapStyleResult());
				bOK = _pointEval->evaluate(objectMapData, rulesetType::point, pointResults.get());
				if (bOK)
				{
					graphicElement->styleResult = pointResults;
				}
				if (typeRuleIDIndex != 0 || (objectMapData->nameTypeString.size() == 0 && !bOK))
				{
					continue;
				}
				graphGroup->_points.push_back(graphicElement);
			}
			else
			{
				// !!
				assert(false);
				continue;
			}
			typeRuleIDIndex++;
		}
		graphGroup->_mapObject = objectMapData;
		_context->_graphicElements.push_back(graphGroup);
	}

	return _context->_graphicElements.size() > 0;
}