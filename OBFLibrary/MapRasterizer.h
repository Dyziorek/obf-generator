#pragma once
class MapRasterizer
{
public:
	MapRasterizer(MapRasterizerProvider& context);
	~MapRasterizer(void);

	void DrawMap(SkCanvas& canvas);

private:

	MapRasterizerProvider& _context;
};

