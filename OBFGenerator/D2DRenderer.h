#pragma once
class D2DRenderer
{
public:
	D2DRenderer(void);
	~D2DRenderer(void);

	bool CreateRenderTarget();

	bool DrawMap(CRenderTarget& canvas, MapRasterizerContext& contextData, MapRasterizerProvider& source);
};

