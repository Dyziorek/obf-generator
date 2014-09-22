#pragma once
class AtlasMapDxRender
{
public:
	AtlasMapDxRender(void);
	~AtlasMapDxRender(void);
	HRESULT Initialize(HWND baseHWND);
	void saveSkBitmapToResource(const SkBitmap& skBitmap, ID3D11Resource* textureBuffer, int xoffset, int yoffset);

private:
	HINSTANCE                           g_hInst;
	HWND                                g_hWnd;
	D3D_DRIVER_TYPE                     g_driverType;
	D3D_FEATURE_LEVEL                   g_featureLevel;
	ID3D11Device*                       g_pd3dDevice;
	ID3D11DeviceContext*                g_pImmediateContext;
	IDXGISwapChain*                     g_pSwapChain;
	ID3D11RenderTargetView*             g_pRenderTargetView;
	ID3D11Texture2D*                    g_pDepthStencil;
	ID3D11DepthStencilView*             g_pDepthStencilView;

	ID3D11Resource*						g_pResourceMap;
	ID3D11Resource*						g_pResourceSky;
	ID3D11ShaderResourceView*           g_pTextureRVSky;
	ID3D11ShaderResourceView*           g_pTextureRVMap;
	ID3D11InputLayout*                  g_pBatchInputLayout;

	std::unique_ptr<CommonStates>                           g_States;
	/*
	std::unique_ptr<BasicEffect>                            g_BatchEffect;
	std::unique_ptr<EffectFactory>                          g_FXFactory;
	std::unique_ptr<GeometricPrimitive>                     g_Shape;
	std::unique_ptr<Model>                                  g_Model;
	std::unique_ptr<PrimitiveBatch<VertexPositionColor>>    g_Batch;
	std::unique_ptr<SpriteFont>                             g_Font;
	*/
	std::unique_ptr<SpriteBatch>                            g_Sprites;
	

	XMMATRIX                            g_World;
	XMMATRIX                            g_View;
	XMMATRIX                            g_Projection;

};

