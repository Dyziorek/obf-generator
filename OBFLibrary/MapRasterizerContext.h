#pragma once

#include <future>

class MapRasterizerContext
{
public:
	MapRasterizerContext(void);
	~MapRasterizerContext(void);

	std::vector<std::shared_ptr<GraphicElementGroup>> _graphicElements;
	std::vector<std::shared_ptr<RasterSymbolGroup>> symbols;


};

