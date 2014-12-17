#pragma once

class AtlasMapDxRender
{
public:
	AtlasMapDxRender(void);
	~AtlasMapDxRender(void);
	AtlasMapDxRender(AtlasMapDxRender&& other);
	AtlasMapDxRender& operator=(AtlasMapDxRender&& other);
	HRESULT Initialize(HWND baseHWND, int numLayers = 1);
	void saveSkBitmapToResource(int textureID, const SkBitmap& skBitmap, int xoffset = 0, int yoffset = 0);
	void renderScene();

private:
	class _Impl;

	std::unique_ptr<_Impl> pImpl;


	
};

