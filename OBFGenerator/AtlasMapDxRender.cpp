#include "stdafx.h"

#include "SkBitmap.h"


#include <d3d11.h>

#include <directxmath.h>

#include "CommonStates.h"
#include "WICTextureLoader.h"
#include "Effects.h"
#include "GeometricPrimitive.h"
#include "Model.h"
#include "PrimitiveBatch.h"
#include "ScreenGrab.h"
#include "SpriteBatch.h"
#include "SpriteFont.h"
#include "VertexTypes.h"

using namespace DirectX;

#include "AtlasMapDxRender.h"

class AtlasMapDxRender::_Impl 
{
public:
	_Impl(void);
	~_Impl(void);

	HRESULT Initialize(HWND baseHWND);
	void saveSkBitmapToResource(const SkBitmap& skBitmap, int xoffset, int yoffset);
	void renderScene();
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
	*/
	std::unique_ptr<SpriteFont>                             g_Font;
	std::unique_ptr<SpriteBatch>                            g_Sprites;
	

	XMMATRIX                            g_World;
	XMMATRIX                            g_View;
	XMMATRIX                            g_Projection;



};


boost::thread_group generators;

AtlasMapDxRender::_Impl::_Impl(void)
{
	g_hInst = nullptr;
	g_hWnd = nullptr;
	g_driverType = D3D_DRIVER_TYPE_NULL;
	g_featureLevel = D3D_FEATURE_LEVEL_11_0;
	g_pd3dDevice = nullptr;
	g_pImmediateContext = nullptr;
	g_pSwapChain = nullptr;
	g_pRenderTargetView = nullptr;
	g_pDepthStencil = nullptr;
	g_pDepthStencilView = nullptr;

	g_pTextureRVSky = nullptr;
	g_pTextureRVMap = nullptr;
	g_pBatchInputLayout = nullptr;

}


AtlasMapDxRender::_Impl::~_Impl(void)
{
}


HRESULT AtlasMapDxRender::_Impl::Initialize(HWND baseHWND)
{
    HRESULT hr = S_OK;
	g_hWnd = baseHWND;
    RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = width;
    sd.BufferDesc.Height = height;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = g_hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;

    for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        g_driverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDeviceAndSwapChain( nullptr, g_driverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
                                            D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &g_featureLevel, &g_pImmediateContext );
        if( SUCCEEDED( hr ) )
            break;
    }
    if( FAILED( hr ) )
        return hr;

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetView );

    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

    // Create depth stencil texture
    D3D11_TEXTURE2D_DESC descDepth;
    ZeroMemory( &descDepth, sizeof(descDepth) );
    descDepth.Width = width;
    descDepth.Height = height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D11_USAGE_DEFAULT;
    descDepth.BindFlags = D3D11_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;
    hr = g_pd3dDevice->CreateTexture2D( &descDepth, nullptr, &g_pDepthStencil );
    if( FAILED( hr ) )
        return hr;

    // Create the depth stencil view
    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV;
    ZeroMemory( &descDSV, sizeof(descDSV) );
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil, &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, &g_pRenderTargetView, g_pDepthStencilView );

    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );
	
	
	D3D11_TEXTURE2D_DESC desc;
    desc.Width = 1024;
    desc.Height = 1024;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	UINT supported = 0;
	hr = g_pd3dDevice->CheckFormatSupport(DXGI_FORMAT_R8G8B8A8_UNORM, &supported);

	hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, (ID3D11Texture2D**)&g_pResourceMap);

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    memset( &SRVDesc, 0, sizeof( SRVDesc ) );
	SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = desc.MipLevels;

	hr = g_pd3dDevice->CreateShaderResourceView(g_pResourceMap, &SRVDesc, &g_pTextureRVMap);

    // Create DirectXTK objects
    g_States.reset( new CommonStates( g_pd3dDevice ) );
    g_Sprites.reset( new SpriteBatch( g_pImmediateContext ) );
	float ClearColor[4] = { 0.176f, 0.196f, 0.667f, 0.0f };
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);
    return S_OK;
}


void AtlasMapDxRender::_Impl::saveSkBitmapToResource(const SkBitmap& skBitmap, int xoffset, int yoffset)
{
	HRESULT hr = S_OK;
    typedef unsigned char UINT8; 
    typedef signed char SINT8; 
    typedef unsigned short UINT16; 
    typedef signed short SINT16; 
    typedef unsigned int UINT32; 
    typedef signed int SINT32; 
   
    //#define BitmapColorGetA(color) (((color) >> 24) & 0xFF) 
    //#define BitmapColorGetR(color) (((color) >> 16) & 0xFF) 
    //#define BitmapColorGetG(color) (((color) >> 8) & 0xFF) 
    //#define BitmapColorGetB(color) (((color) >> 0) & 0xFF) 
 
    int bmpWidth = skBitmap.width(); 
    int bmpHeight = skBitmap.height();
    int stride = skBitmap.rowBytes(); 
    char* m_pmap = (char*)skBitmap.getPixels(); 
    //virtual PixelFormat& GetPixelFormat() =0; //assume pf is ARGB; 
    //FILE* fp = fopen(path, "wb"); 
    //if(!fp){ 
    //    printf("saveSkBitmapToBMPFile: fopen %s Error!\n", path); 
    //} 
	
    SINT32 bpl=stride*4; 
	

   // for(SINT32 y=bmpHeight-1; y>=0; y--) 
   // { 
   //     SINT32 base=y*stride; 
   //     for(SINT32 x=0; x<(SINT32)bmpWidth; x++) 
   //     { 
   //         UINT32 i=base+x*4;
   //         UINT32 pixelData = *(UINT32*)(m_pmap+i);
   //         //UINT8 b1=BitmapColorGetB(pixelData);
   //         //UINT8 g1=BitmapColorGetG(pixelData);
   //         //UINT8 r1=BitmapColorGetR(pixelData);
   //         //UINT8 a1=BitmapColorGetA(pixelData);
   //         //r1=r1*a1/255; 
   //         //g1=g1*a1/255; 
   //         //b1=b1*a1/255; 
   //         //UINT32 temp=(a1<<24)|(r1<<16)|(g1<<8)|b1;//a bmp pixel in little endian is B,G,R,A
			//*(UINT32*)(m_pmap+i) = temp;
   //     } 
   // } 
	D3D11_BOX destRegion;
	destRegion.left = xoffset;
	destRegion.right = yoffset;
	destRegion.top = xoffset+bmpWidth;
	destRegion.bottom = yoffset+bmpHeight;
	destRegion.front = 0;
	destRegion.back = 1;

	D3D11_MAPPED_SUBRESOURCE subData;

	hr = g_pImmediateContext->Map(g_pResourceMap, 0, D3D11_MAP_WRITE_DISCARD, 0, &subData);
	char* mappedData = (char*)subData.pData;
	for (int i = 0; i < bmpHeight; i++)
	{
		memcpy(mappedData, m_pmap, stride);
		mappedData += subData.RowPitch;
		m_pmap += stride;
	}

	g_pImmediateContext->Unmap(g_pResourceMap, 0);
	//g_pImmediateContext->UpdateSubresource(g_pResourceMap, 0, &destRegion, m_pmap, stride, bpl*bmpHeight);

}


void AtlasMapDxRender::_Impl::renderScene()
{
	RECT rc;
    GetClientRect( g_hWnd, &rc );

	RECT src;
	src.top = 0;
	src.left = 0;
	src.bottom = 1024;
	src.right = 1024;

	g_Sprites->Begin();
	g_Sprites->Draw(g_pTextureRVMap, rc, &src);
	g_Sprites->End();

	g_pSwapChain->Present(0,0);
}

AtlasMapDxRender::AtlasMapDxRender(void) : pImpl(new _Impl())
{

}

AtlasMapDxRender::~AtlasMapDxRender(void)
{

}

AtlasMapDxRender::AtlasMapDxRender(AtlasMapDxRender&& other) : pImpl(std::move(other.pImpl))
{

}

AtlasMapDxRender& AtlasMapDxRender::operator=(AtlasMapDxRender&& other)
{
	pImpl = std::move(other.pImpl);
	return *this;
}

HRESULT AtlasMapDxRender::Initialize(HWND baseHWND)
{
	return pImpl->Initialize(baseHWND);
}
void AtlasMapDxRender::saveSkBitmapToResource(const SkBitmap& skBitmap, int xoffset, int yoffset)
{
	pImpl->saveSkBitmapToResource(skBitmap, xoffset, yoffset);
}
void AtlasMapDxRender::renderScene()
{
	pImpl->renderScene();
}