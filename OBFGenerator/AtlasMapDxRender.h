#pragma once


class AtlasMapDxRender
{
public:
	AtlasMapDxRender(void);
	~AtlasMapDxRender(void);
	AtlasMapDxRender(AtlasMapDxRender&& other);
	AtlasMapDxRender& operator=(AtlasMapDxRender&& other);
	HRESULT Initialize(HWND baseHWND, int numLayers = 1);
	HRESULT ResizeWindow(HWND windowResized);
	void saveSkBitmapToResource(int textureID, const SkBitmap& skBitmap, int xoffset = 0, int yoffset = 0);
	//void packTexture(int textureID, const std::unordered_map<int32_t, std::shared_ptr<const SkBitmap>>& bitmaps);
	void updateTexture(std::vector<std::shared_ptr<MapRasterizer::RasterSymbolGroup>>& symbolData, std::shared_ptr<MapRasterizerContext> currCtx);
	void renderScene();
	 

private:
	class _Impl;

	std::unique_ptr<_Impl> pImpl;

	
};

