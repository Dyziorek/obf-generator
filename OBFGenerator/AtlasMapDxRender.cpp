#include "stdafx.h"

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
#include "DirectXHelpers.h"
#include "SkBitmap.h"
#include "TexturePacker.h"
#include "SkCanvas.h"
#include "SkPaint.h"

#include "SkDashPathEffect.h"
#include "SkBitmapProcShader.h"
#include "SkStream.h"
#include "SkImageDecoder.h"

#include "MapObjectData.h"
#include "MapStyleData.h"
#include "MapStyleChoiceValue.h"
#include "DefaultMapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleInfo.h"
#include "MapStyleEval.h"
#include <google\protobuf\io\coded_stream.h>
#include <boost/filesystem.hpp>
#include "RandomAccessFileReader.h"
#include "MapObjectData.h"
#include "BinaryMapDataReader.h"
#include "BinaryAddressDataReader.h"
#include "BinaryRouteDataReader.h"
#include "BinaryIndexDataReader.h"
#include <boost/detail/endian.hpp>
#include "BinaryReaderUtils.h"
#include "tinyxml2.h"
#include "MapStyleData.h"
#include "MapStyleChoiceValue.h"
#include "DefaultMapStyleValue.h"
#include "MapStyleRule.h"
#include "MapStyleInfo.h"
#include "MapStyleEval.h"
#include "MapRasterizer.h"
#include "MapRasterizerContext.h"
#include "MapRasterizerProvider.h"

#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/mem_fun.hpp>

#include "DirectXPackedVector.h"

using namespace DirectX;

#include "AtlasMapDxRender.h"

namespace mi = boost::multi_index;

class AtlasMapDxRender::_Impl 
{
private:

	enum textureTypeID
	{ 
		mapTexture = 0, 
		iconsTexture = 1
	};

public:
	_Impl(void);
	~_Impl(void);

	
	typedef boost::multi_index_container<std::shared_ptr<TextureBlock>, mi::indexed_by< mi::ordered_unique<mi::mem_fun<TextureBlock, std::string, &TextureBlock::textureName> >,
		mi::ordered_unique<mi::mem_fun<TextureBlock, uint32_t , &TextureBlock::texID>>>> stringIdMap;
	typedef stringIdMap::nth_index<1>::type lookupId;
	


	HRESULT Initialize(HWND baseHWND, int numLayers);
	HRESULT ResizeWindow(HWND windowResized);
	void saveSkBitmapToResource(int textureID, const SkBitmap& skBitmap, int xoffset, int yoffset);
	void packTexture(int textureID, stringIdMap& textureBlocks);
	void drawDataToTexture(std::vector<std::shared_ptr<TextureBlock>>& hints, int8_t numTextures);
	void updateTexture(std::vector<std::shared_ptr<MapRasterizer::RasterSymbolGroup>>& symbolData, std::shared_ptr<MapRasterizerContext> currCtx);
	void renderScene();
private:
	typedef bgm::ring<pointF> ringF;
	typedef bgm::box<pointF> boxF;
	HINSTANCE                           g_hInst;
	HWND                                g_hWnd;
	D3D_DRIVER_TYPE                     g_driverType;
	D3D_FEATURE_LEVEL                   g_featureLevel;
	Microsoft::WRL::ComPtr<ID3D11Device>                       g_pd3dDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>                g_pImmediateContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain>                     g_pSwapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>             g_pRenderTargetView;
	Microsoft::WRL::ComPtr<ID3D11Texture2D>                    g_pDepthStencil;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>             g_pDepthStencilView;

//	Microsoft::WRL::ComPtr<ID3D11Resource>	g_pResourceMap;
	Microsoft::WRL::ComPtr<ID3D11Resource>	g_pResourceSky;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  g_pTextureRVSky;
//	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>  g_pTextureRVMap;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>    g_pBatchInputLayout;

	std::unique_ptr<CommonStates>                           g_States;

	std::vector<std::pair<Microsoft::WRL::ComPtr<ID3D11Resource>, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>>> textureResourceLayers;
	std::vector<std::shared_ptr<MapRasterizer::RasterSymbolGroup>> symbolDataToScene;
	std::shared_ptr<MapRasterizerContext> _workCtx;
	/*
	std::unique_ptr<BasicEffect>                            g_BatchEffect;
	std::unique_ptr<EffectFactory>                          g_FXFactory;
	std::unique_ptr<GeometricPrimitive>                     g_Shape;
	std::unique_ptr<Model>                                  g_Model;
	std::unique_ptr<PrimitiveBatch<VertexPositionColor>>    g_Batch;
	*/
	std::unordered_map<std::string,std::unique_ptr<SpriteFont>>       g_Fonts;
	std::unique_ptr<SpriteBatch>                            g_Sprites;
	stringIdMap textureMapNames;
	//std::unordered_map<int32_t, const std::shared_ptr<const SkBitmap>> bitmapIdHash;
	
	//std::unordered_map<std::string,  std::shared_ptr<const TextureBlock>> textureMapNames;
	XMMATRIX                            g_World;
	XMMATRIX                            g_View;
	XMMATRIX                            g_Projection;
	MapRasterizerProvider               loaderResources;
	XMVECTORF32 ClearColor;
	void InitSymbolTexture();
	void InitFontsTexture();
	void AddTextureLayer(int newLayers);
	void vertexFromPointArea(const AreaI& area,const pointI& position, const pointD& scale, XMVECTOR& vertices);
	void vertexToPoint(XMVECTOR& vertice, pointF& result);
	bool notIntersect(const std::vector<std::pair<ringF, boxF>>& rings,const ringF& ringCheck);
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

	g_pBatchInputLayout = nullptr;
	
	//ClearColor = XMVECTORF32();
	ClearColor.f[0] =  1.0f;
	ClearColor.f[1] =  1.0f;
	ClearColor.f[2] =  1.0f;
	ClearColor.f[3] =  0.0f;

	
}


AtlasMapDxRender::_Impl::~_Impl(void)
{
}


HRESULT AtlasMapDxRender::_Impl::Initialize(HWND baseHWND, int numLayers)
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
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil.Get(), &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, g_pRenderTargetView.GetAddressOf(), g_pDepthStencilView.Get() );

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

	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    memset( &SRVDesc, 0, sizeof( SRVDesc ) );
	SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = desc.MipLevels;

	hr = g_pd3dDevice->CheckFormatSupport(DXGI_FORMAT_R8G8B8A8_UNORM, &supported);

	for (auto numLayer = 0; numLayer < numLayers + 1; numLayer++)
	{
		Microsoft::WRL::ComPtr<ID3D11Resource> resourceElem;
		 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureRV;
		hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, (ID3D11Texture2D**)resourceElem.GetAddressOf());
		hr = g_pd3dDevice->CreateShaderResourceView(resourceElem.Get(), &SRVDesc, &textureRV);
		textureResourceLayers.push_back(std::make_pair(std::move(resourceElem), std::move(textureRV)));
		if (numLayer == textureTypeID::mapTexture)
		{
			SetDebugObjectName(textureResourceLayers[0].first.Get(), "MapTextureData");
			SetDebugObjectName(textureResourceLayers[0].second.Get(), "MapTextureData");
		}
		if (numLayer == textureTypeID::iconsTexture)
		{
			SetDebugObjectName(textureResourceLayers[1].first.Get(), "IconTextureData");
			SetDebugObjectName(textureResourceLayers[1].second.Get(), "IconTextureData");
		}
	}


	

    // Create DirectXTK objects
    g_States.reset( new CommonStates( g_pd3dDevice.Get() ) );
    g_Sprites.reset( new SpriteBatch( g_pImmediateContext.Get() ) );
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView.Get(), ClearColor);

	InitSymbolTexture();
	InitFontsTexture();
    return S_OK;
}

HRESULT AtlasMapDxRender::_Impl::ResizeWindow(HWND baseHWND)
{
	HRESULT hr;
	g_hWnd = baseHWND;
    
	RECT rc;
    GetClientRect( g_hWnd, &rc );
    UINT width = rc.right - rc.left;
    UINT height = rc.bottom - rc.top;

	g_pImmediateContext->OMSetRenderTargets(0,0,0);
	g_pRenderTargetView.Reset();

	g_pSwapChain->ResizeBuffers(0,width,height,DXGI_FORMAT_UNKNOWN, 0);

	ID3D11Texture2D* pBackBuffer = nullptr;
    hr = g_pSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), ( LPVOID* )&pBackBuffer );
    if( FAILED( hr ) )
        return hr;

    hr = g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_pRenderTargetView );

    pBackBuffer->Release();
    if( FAILED( hr ) )
        return hr;

	if (g_pDepthStencil)
	{
		g_pDepthStencil.Reset();
	}

	if (g_pDepthStencilView)
	{
		g_pDepthStencilView.Reset();
	}

	D3D11_RENDER_TARGET_VIEW_DESC descData;

	g_pRenderTargetView->GetDesc(&descData);

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
    hr = g_pd3dDevice->CreateDepthStencilView( g_pDepthStencil.Get(), &descDSV, &g_pDepthStencilView );
    if( FAILED( hr ) )
        return hr;

    g_pImmediateContext->OMSetRenderTargets( 1, g_pRenderTargetView.GetAddressOf(), g_pDepthStencilView.Get() );



    // Setup the viewport
    D3D11_VIEWPORT vp;
    vp.Width = (FLOAT)width;
    vp.Height = (FLOAT)height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    g_pImmediateContext->RSSetViewports( 1, &vp );
	

}

void  AtlasMapDxRender::_Impl::AddTextureLayer(int newLayers)
{
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
	
	HRESULT hr = S_OK;
	D3D11_SHADER_RESOURCE_VIEW_DESC SRVDesc;
    memset( &SRVDesc, 0, sizeof( SRVDesc ) );
	SRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	SRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	SRVDesc.Texture2D.MipLevels = desc.MipLevels;

	for (auto numLayer = 0; numLayer < newLayers; numLayer++)
	{
		Microsoft::WRL::ComPtr<ID3D11Resource> resourceElem;
		 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureRV;
		hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, (ID3D11Texture2D**)resourceElem.GetAddressOf());
		hr = g_pd3dDevice->CreateShaderResourceView(resourceElem.Get(), &SRVDesc, &textureRV);
		textureResourceLayers.push_back(std::make_pair(std::move(resourceElem), std::move(textureRV)));
		SetDebugObjectName(textureResourceLayers[textureResourceLayers.size()-1].first.Get(), "iconTextureNameNext");
		SetDebugObjectName(textureResourceLayers[textureResourceLayers.size()-1].second.Get(), "iconTextureNameNext");
	}
}

void AtlasMapDxRender::_Impl::InitSymbolTexture()
{
	std::string initResNames[] = {"accomodation", "administrative", "aerialway_station","finance","fuel","historic_castle","landuse_grass","place_town", "railway_station"
		, "shop_bicycle", "shop_car_repair", "shop_pet", "tourism_hotel", "tourism_motel", "tourism_viewpoint", "tourism"};

		
	for (auto resName : initResNames)
	{
		std::shared_ptr<const SkBitmap> resData;
		bool bOK = loaderResources.obtainMapIcon(resName, resData);
		if (!bOK)
		{
			bOK = loaderResources.obtainTextShield(resName, resData);
		}
		if (bOK && static_cast<bool>(resData))
		{
			std::shared_ptr<TextureBlock> blocker(new TextureBlock());
			blocker->Subrect.left = 0;
			blocker->Subrect.right = resData->width();
			blocker->Subrect.bottom = resData->height();
			blocker->Subrect.top = 0;
			blocker->textureHandle = textureMapNames.size();
			blocker->texName = std::move(resName);
			blocker->bitmapLink = std::move(resData);
			textureMapNames.insert(blocker);
		}
	}
	

	packTexture(1, textureMapNames);
	
}

void AtlasMapDxRender::_Impl::vertexFromPointArea(const AreaI& area,const pointI& position, const pointD& scale, XMVECTOR& vertices)
{
	
	auto xVal = static_cast<float>(position.get<0>());
	auto xArea = static_cast<float>(area.min_corner().get<0>());
	auto yArea = static_cast<float>(area.min_corner().get<1>());
	auto yVal = static_cast<float>(position.get<1>());
	vertices.m128_f32[0] = ((xVal - xArea) / scale.get<0>());
	vertices.m128_f32[1] = ((yVal - yArea)/ scale.get<1>());
}

void AtlasMapDxRender::_Impl::vertexToPoint(XMVECTOR& vertice, pointF& result)
{
	result.set<0>(vertice.m128_f32[0]);
	result.set<1>(vertice.m128_f32[1]);
}

bool AtlasMapDxRender::_Impl::notIntersect(const std::vector<std::pair<ringF, boxF>>& rings,const ringF& ringCheck)
{
	boxF checkBox;
	bg::envelope(ringCheck, checkBox);
	for (const auto& ringToCheck : rings)
	{
		if (bg::intersects(ringToCheck.second, checkBox))
		{
			if (bg::intersects(ringToCheck.first, ringCheck))
				return false;
		}
	}

	return true;
}

void AtlasMapDxRender::_Impl::InitFontsTexture()
{
	std::vector<uint8_t> resultData = loaderResources.obtainResourceByName("map/fonts/arialn.spf");
	if (resultData.size() > 0)
	{
		auto DataPair = std::make_pair("arial", std::unique_ptr<SpriteFont>(std::move(new SpriteFont(g_pd3dDevice.Get(), resultData.data(), resultData.size()))));
		DataPair.second->SetDefaultCharacter(L' ');
		g_Fonts.insert(std::move(DataPair));
	}
	resultData = loaderResources.obtainResourceByName("map/fonts/calibri.spf");
	if (resultData.size() > 0)
	{
		auto DataPair = std::make_pair("calibri", std::unique_ptr<SpriteFont>(std::move(new SpriteFont(g_pd3dDevice.Get(), resultData.data(), resultData.size()))));
		DataPair.second->SetDefaultCharacter(L' ');
		g_Fonts.insert(std::move(DataPair));
	}	
	resultData = loaderResources.obtainResourceByName("map/fonts/gothic.spf");
	if (resultData.size() > 0)
	{
		auto DataPair = std::make_pair("gothic", std::unique_ptr<SpriteFont>(std::move(new SpriteFont(g_pd3dDevice.Get(), resultData.data(), resultData.size()))));
		DataPair.second->SetDefaultCharacter(L' ');
		g_Fonts.insert(std::move(DataPair));
	}
}

void  AtlasMapDxRender::_Impl::drawDataToTexture(  std::vector<std::shared_ptr<TextureBlock>>& hints, int8_t numTextures)
{
	HRESULT hr = S_OK;
	if (textureResourceLayers.size() - 1 < numTextures)
	{
		AddTextureLayer(numTextures - (textureResourceLayers.size() - 2));
	}

	std::vector<std::vector<std::shared_ptr<TextureBlock>>> textureIDMap;

	for (auto textureElem : hints)
	{
		if (textureElem->textureID +1 > textureIDMap.size())
		{
			textureIDMap.push_back(std::vector<std::shared_ptr<TextureBlock>>());
		}
		textureIDMap[textureElem->textureID].push_back(textureElem);
		if (textureMapNames.get<1>().find(textureElem->textureHandle) != textureMapNames.get<1>().end())
		{
			auto TextureObjPtr = textureMapNames.get<1>().find(textureElem->textureHandle)->get();
			TextureObjPtr->flipped = textureElem->flipped;
			TextureObjPtr->Subrect = textureElem->Subrect;
			TextureObjPtr->XOffset = textureElem->XOffset;
			TextureObjPtr->YOffset = textureElem->YOffset;
		}
	}

	for (int texID = 0; texID < textureIDMap.size(); texID++)
	{
		auto layerMapData = textureResourceLayers[texID+1];
		auto resourceData = layerMapData.first;
		
		D3D11_MAPPED_SUBRESOURCE subData;

		hr = g_pImmediateContext->Map(resourceData.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subData);
		
		for(std::shared_ptr<TextureBlock> blockInfo : textureIDMap[texID])
		{
			//assert(bitmaps.find(blockInfo->textureHandle) == bitmaps.end());
			char* mappedData = (char*)subData.pData;
			if (textureMapNames.get<1>().find((int32_t)blockInfo->textureHandle) != textureMapNames.get<1>().end())
			{
				auto bitmapData = textureMapNames.get<1>().find((int32_t)blockInfo->textureHandle)->get()->bitmapLink;
				assert(static_cast<bool>(bitmapData));

				char* m_pmap = (char*)bitmapData->getPixels(); 
				int bpl = bitmapData->bytesPerPixel();
				int stride = bitmapData->rowBytes(); 
				int yoffset = blockInfo->YOffset;
				int xoffset = blockInfo->XOffset;
				if (yoffset > 0)
				{
					mappedData += subData.RowPitch*yoffset;
				}
				if (blockInfo->flipped)
				{
					for (int x = 0; x < bitmapData->width(); x++)
					{
						for (int y = 0; y < bitmapData->height(); y++)
						{
							memcpy(mappedData+((xoffset*bpl)+y),m_pmap+(y*stride), bpl);
						}
						mappedData += subData.RowPitch;
						m_pmap+=bpl;
					}
				}
				else
				{
					for (int i = 0; i < bitmapData->height(); i++)
					{
						memcpy(mappedData+(xoffset*bpl), m_pmap, bitmapData->width()*bpl);
						mappedData += subData.RowPitch;
						m_pmap += stride;
					}
				}
			}
			else
			{
				assert(false);
			}
		}
		g_pImmediateContext->Unmap(resourceData.Get(), 0);
	}

}

void AtlasMapDxRender::_Impl::saveSkBitmapToResource(int textureID, const SkBitmap& skBitmap, int xoffset, int yoffset)
{
	HRESULT hr = S_OK;
    typedef unsigned char UINT8; 
    typedef signed char SINT8; 
    typedef unsigned short UINT16; 
    typedef signed short SINT16; 
    typedef unsigned int UINT32; 
    typedef signed int SINT32; 
   
    int bmpWidth = skBitmap.width(); 
    int bmpHeight = skBitmap.height();
    int stride = skBitmap.rowBytes(); 
    char* m_pmap = (char*)skBitmap.getPixels(); 
	int bpl = skBitmap.bytesPerPixel();
 

	D3D11_MAPPED_SUBRESOURCE subData;

	hr = g_pImmediateContext->Map(textureResourceLayers[textureID].first.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subData);
	char* mappedData = (char*)subData.pData;
	if (yoffset > 0)
	{
		m_pmap += stride*yoffset;
		mappedData += subData.RowPitch*yoffset;
	}

	for (int i = yoffset; i < bmpHeight; i++)
	{
		memcpy(mappedData+(xoffset*bpl), m_pmap+(xoffset*bpl), stride);
		mappedData += subData.RowPitch;
		m_pmap += stride;
	}

	g_pImmediateContext->Unmap(textureResourceLayers[textureID].first.Get(), 0);
	//g_pImmediateContext->UpdateSubresource(g_pResourceMap, 0, &destRegion, m_pmap, stride, bpl*bmpHeight);

}


void AtlasMapDxRender::_Impl::renderScene()
{

	RECT rc;
    GetClientRect( g_hWnd, &rc );

	long height = rc.bottom - rc.top;
	long width = rc.right - rc.left;



	std::vector<std::pair<ringF, boxF>> rings;

	
	

	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView.Get(), ClearColor);
	g_Sprites->Begin();
	// map layer texture
	auto layerData = textureResourceLayers[mapTexture];
	{
		
		g_Sprites->Draw(layerData.second.Get(), rc, &rc);
		
	}

	g_Sprites->End();

	auto iconData = textureResourceLayers[iconsTexture];

	if (!_workCtx)
	{
		
		// text layer
		g_pSwapChain->Present(0,0);
		return;
	}

	g_Sprites->Begin();
	const std::shared_ptr<const MapRasterizerContext> context = _workCtx;
	AreaI workArea = context->_area31;
	// icon layer textures used
	for (auto symbolGroup : symbolDataToScene)
	{
		auto symbolVal = symbolGroup;
		for(auto symbol : symbolVal->_symbols)
        {
			if(const auto textSymbol = std::dynamic_pointer_cast<const MapRasterizer::RasterSymbolonPath>(symbol))
			{
				XMVECTOR vecPos;
				vertexFromPointArea(workArea, textSymbol->location, context->_pixelScaleXY, vecPos);
				if(!textSymbol->shieldResourceName.empty() && textureMapNames.find(textSymbol->shieldResourceName) != textureMapNames.end())
				{
					auto textureValue = textureMapNames.find(textSymbol->shieldResourceName)->get();
					RECT destRect;
					destRect.left = textureValue->XOffset;
					destRect.right = textureValue->XOffset + textureValue->Subrect.right;
					destRect.top = textureValue->YOffset;
					destRect.bottom = textureValue->YOffset + textureValue->Subrect.bottom;

					ringF spriteBBox;
					pointF ptOrigin;
					vertexToPoint(vecPos, ptOrigin);
					spriteBBox.push_back(ptOrigin);
					vertexToPoint(vecPos+XMVectorSetInt(0, destRect.bottom - destRect.top, 0,0), ptOrigin);
					spriteBBox.push_back(ptOrigin);
					vertexToPoint(vecPos+XMVectorSetInt(destRect.right - destRect.left, destRect.bottom - destRect.top, 0,0), ptOrigin);
					spriteBBox.push_back(ptOrigin);
					vertexToPoint(vecPos+XMVectorSetInt(0, destRect.bottom - destRect.top, 0,0), ptOrigin);
					spriteBBox.push_back(ptOrigin);
					boxF env;
					bg::envelope(spriteBBox, env);
					if (rings.size() == 0 || notIntersect(rings, spriteBBox))
					{
						rings.push_back(std::make_pair(spriteBBox, env));
						g_Sprites->Draw(iconData.second.Get(), vecPos, &destRect);
					}

				}
			}
			else if(const auto iconSymbol = std::dynamic_pointer_cast<const MapRasterizer::RasterSymbolPin>(symbol))
			{
				if (iconSymbol->resourceName != "")
				{
					auto textureValue = textureMapNames.find(iconSymbol->resourceName)->get();
					XMVECTOR vecPos;
					vertexFromPointArea(workArea, iconSymbol->location, context->_pixelScaleXY, vecPos);
					RECT destRect;
					destRect.left = textureValue->XOffset;
					destRect.right = textureValue->XOffset + textureValue->Subrect.right;
					destRect.top = textureValue->YOffset;
					destRect.bottom = textureValue->YOffset + textureValue->Subrect.bottom;
					
					ringF spriteBBox;
					pointF ptOrigin;
					vertexToPoint(vecPos, ptOrigin);
					spriteBBox.push_back(ptOrigin);
					vertexToPoint(vecPos+XMVectorSetInt(0, destRect.bottom - destRect.top, 0,0), ptOrigin);
					spriteBBox.push_back(ptOrigin);
					vertexToPoint(vecPos+XMVectorSetInt(destRect.right - destRect.left, destRect.bottom - destRect.top, 0,0), ptOrigin);
					spriteBBox.push_back(ptOrigin);
					vertexToPoint(vecPos+XMVectorSetInt(0, destRect.bottom - destRect.top, 0,0), ptOrigin);
					spriteBBox.push_back(ptOrigin);
					boxF env;
					bg::envelope(spriteBBox, env);
					if (rings.size() == 0 || notIntersect(rings, spriteBBox))
					{
						rings.push_back(std::make_pair(spriteBBox, env));
						g_Sprites->Draw(iconData.second.Get(), vecPos, &destRect);
					}

				}
			}
		}

		/*g_Sprites->Draw(iconData.second.Get(), rc, &rc);*/
	}

	// text layer
	auto& fontRender = g_Fonts["arial"];
	
	for (auto symbolGroup : symbolDataToScene)
	{
		auto symbolVal = symbolGroup;
		for(auto symbol : symbolVal->_symbols)
        {
			if(const auto textSymbol = std::dynamic_pointer_cast<const MapRasterizer::RasterSymbolonPath>(symbol))
			{
				XMVECTOR vecPos;
				vertexFromPointArea(workArea, textSymbol->location, context->_pixelScaleXY, vecPos);
				XMVECTOR stringSize = fontRender->MeasureString(textSymbol->value.c_str());
				
				

				//XMVECTOR vecSize = fontRender->MeasureString(textSymbol->value.c_str());
				DirectX::PackedVector::XMCOLOR xcolor(textSymbol->color);
				XMVECTOR color = DirectX::PackedVector::XMLoadColor(&xcolor);

				if (textSymbol->drawOnPath)
				{
					std::vector<XMVECTOR> vecTexts;
#ifdef _DEBUG
					float xCheck = 0.0f;
					float yCheck = 0.0f;
#endif
					wchar_t charStr[2] = { 0, 0};
					for (auto charVal : textSymbol->value)
					{
						charStr[0] = charVal;
						vecTexts.push_back(fontRender->MeasureString(charStr));
#ifdef _DEBUG
						xCheck+=vecTexts.back().m128_f32[0];
						yCheck+=vecTexts.back().m128_f32[1];
#endif
						
					}

					stringSize *= 0.4;
					auto graphElem = symbol->graph;
					if (graphElem->_type == MapRasterizer::GraphElementType::Polyline)
					{
						const auto pointVec = graphElem->_mapData->points;
						float px = 0;
						float py = 0;
						XMVECTOR pxVec, pxVec2;
						for (int i = 1; i < pointVec.size(); i++) {
							vertexFromPointArea(workArea, pointVec[i - 1], context->_pixelScaleXY, pxVec);
							vertexFromPointArea(workArea, pointVec[i], context->_pixelScaleXY, pxVec2);
							px +=  pxVec2.m128_f32[0] - pxVec.m128_f32[0];
							py +=  pxVec2.m128_f32[1] - pxVec.m128_f32[1];
						}
						float rotation = 0.0;
						float pathLen = 0.0;
						float plen = 0.0;
						XMVECTOR pxLen = XMLoadFloat2(&XMFLOAT2(px, py));
						if (px != 0 || py != 0) {
							rotation = (float) (-std::atan2(px, py) + boost::math::constants::pi<float>() / 2.0f);
							//XMVECTOR pxV = XMVectorSet(px, py, 0,0);
							//XMVECTOR xRot = XMVector2AngleBetweenVectors(pxV, XMVectorSet(1, 0,0,0));
							//xRot.m128_f32[0] +=XM_PI/2;
							pathLen = XMVector2Length(pxLen).m128_f32[0];
							if (rotation < 0) rotation += XM_PI * 2.0f;
							if (rotation > XM_PI / 2.0f && rotation < 3.0f * XM_PI / 2.0f) {
								rotation += XM_PI;
							}
						}
						if (pathLen >= stringSize.m128_f32[0])
						{

							ringF spriteBBox;
							pointF ptOrigin;
							XMVECTOR rotationMatrix1;
							XMVECTOR rotationMatrix2;
							if (rotation != 0)
							{
								float sin, cos;

								XMScalarSinCos(&sin, &cos, rotation);

								XMVECTOR sinV = XMLoadFloat(&sin);
								XMVECTOR cosV = XMLoadFloat(&cos);

								rotationMatrix1 = XMVectorMergeXY(cosV, sinV);
								rotationMatrix2 = XMVectorMergeXY(-sinV, cosV);
							}
							else
							{
								rotationMatrix1 = g_XMIdentityR0;
								rotationMatrix2 = g_XMIdentityR1;
							}
							XMVECTOR destination = vecPos;
							XMVECTOR position1 = XMVectorMultiply(XMVectorSplatX(vecPos), rotationMatrix1);
							XMVECTOR position2 = XMVectorMultiplyAdd(XMVectorSplatY(vecPos), rotationMatrix2, position1);
							vertexToPoint(position2, ptOrigin);
							spriteBBox.push_back(ptOrigin);
							 position1 = XMVectorMultiply(XMVectorSplatX(vecPos+XMVectorSet(0, stringSize.m128_f32[1], 0, 0)), rotationMatrix1);
							 position2 = XMVectorMultiplyAdd(XMVectorSplatY(vecPos+XMVectorSet(0, stringSize.m128_f32[1], 0, 0)), rotationMatrix2, position1);
							vertexToPoint(position2, ptOrigin);
							spriteBBox.push_back(ptOrigin);
							position1 = XMVectorMultiply(XMVectorSplatX(vecPos+XMVectorSet(stringSize.m128_f32[0], stringSize.m128_f32[1], 0, 0)), rotationMatrix1);
							position2 = XMVectorMultiplyAdd(XMVectorSplatY(vecPos+XMVectorSet(stringSize.m128_f32[0], stringSize.m128_f32[1], 0, 0)), rotationMatrix2, position1);
							vertexToPoint(position2, ptOrigin);
							spriteBBox.push_back(ptOrigin);
							position1 = XMVectorMultiply(XMVectorSplatX(vecPos+XMVectorSetInt(stringSize.m128_f32[0], 0, 0, 0)), rotationMatrix1);
							position2 = XMVectorMultiplyAdd(XMVectorSplatY(vecPos+XMVectorSetInt(stringSize.m128_f32[0], 0, 0, 0)), rotationMatrix2, position1);
							vertexToPoint(position2, ptOrigin);
							spriteBBox.push_back(ptOrigin);
							boxF env;
							bg::envelope(spriteBBox, env);
							if (rings.size() == 0 || notIntersect(rings, spriteBBox))
							{
								rings.push_back(std::make_pair(spriteBBox, env));
								fontRender->DrawString(g_Sprites.get(), textSymbol->value.c_str(), vecPos, color, rotation, g_XMZero, XMVectorScale(g_XMOne, 0.4f));
							}
						}
					}
				}
				else
				{

					ringF spriteBBox;
					pointF ptOrigin;
					vertexToPoint(vecPos, ptOrigin);
					spriteBBox.push_back(ptOrigin);
					vertexToPoint(vecPos+XMVectorSet(0, stringSize.m128_f32[1], 0, 0), ptOrigin);
					spriteBBox.push_back(ptOrigin);
					vertexToPoint(vecPos+XMVectorSet(stringSize.m128_f32[0], stringSize.m128_f32[1], 0, 0), ptOrigin);
					spriteBBox.push_back(ptOrigin);
					vertexToPoint(vecPos+XMVectorSetInt(stringSize.m128_f32[0], 0, 0, 0), ptOrigin);
					spriteBBox.push_back(ptOrigin);
					boxF env;
					bg::envelope(spriteBBox, env);
					if (rings.size() == 0 || notIntersect(rings, spriteBBox))
					{
						rings.push_back(std::make_pair(spriteBBox, env));
						fontRender->DrawString(g_Sprites.get(), textSymbol->value.c_str(), vecPos, color, 0.0f, g_XMZero, XMVectorScale(g_XMOne, textSymbol->size));
					}
				}
			}
		}

		/*g_Sprites->Draw(iconData.second.Get(), rc, &rc);*/
	}
	g_Sprites->End();
	g_pSwapChain->Present(0,0);
}


void AtlasMapDxRender::_Impl::packTexture(int textureID, stringIdMap& blockData)
{
	TexturePacker packerData;

	

	int numLayers = 0;

	
	std::vector<std::shared_ptr<TextureBlock>> results;
	std::vector<const std::shared_ptr<const TextureBlock>> inputs;
	for(auto block : blockData)
	{
		inputs.push_back(block);
	}

	if (packerData.packTexture(inputs, 1024, results))
	{
		for (int textureHandleID = 0; textureHandleID < results.size(); textureHandleID++)
		{
			if (numLayers < results[textureHandleID]->textureID+1)
			{
				numLayers = results[textureHandleID]->textureID+1;
			}
		}

		drawDataToTexture(results, numLayers);
	}
}

void AtlasMapDxRender::_Impl::updateTexture(std::vector<std::shared_ptr<MapRasterizer::RasterSymbolGroup>>& symbolData, std::shared_ptr<MapRasterizerContext> currCtx)
{
	
	bool insertedNewSymbol = false;

	for(auto itSymbolsEntry =symbolData.cbegin(); itSymbolsEntry != symbolData.cend(); ++itSymbolsEntry)
    {
		auto symbolVal = *(itSymbolsEntry);
		for(auto itPrimitiveSymbol = symbolVal->_symbols.cbegin(); itPrimitiveSymbol != symbolVal->_symbols.cend(); ++itPrimitiveSymbol)
        {
			const auto& symbol = *itPrimitiveSymbol;
			if(const auto textSymbol = std::dynamic_pointer_cast<const MapRasterizer::RasterSymbolonPath>(symbol))
            {
				std::shared_ptr<const SkBitmap> textShieldBitmap;
				if(!textSymbol->shieldResourceName.empty() && textureMapNames.find(textSymbol->shieldResourceName) == textureMapNames.end())
				{
                    if( loaderResources.obtainTextShield(textSymbol->shieldResourceName, textShieldBitmap))
					{
						std::shared_ptr<TextureBlock> blocker(new TextureBlock());
						blocker->Subrect.left = 0;
						blocker->Subrect.right = textShieldBitmap->width();
						blocker->Subrect.bottom = textShieldBitmap->height();
						blocker->Subrect.top = 0;
						blocker->textureHandle = textureMapNames.size();
						blocker->texName = textSymbol->shieldResourceName;
						blocker->bitmapLink = std::move(textShieldBitmap);
						textureMapNames.insert(blocker);
						insertedNewSymbol = true;
					}				
				}
			}
			else if(const auto iconSymbol = std::dynamic_pointer_cast<const MapRasterizer::RasterSymbolPin>(symbol))
            {
				if(textureMapNames.find(iconSymbol->resourceName) != textureMapNames.end())
					continue;
				std::shared_ptr<const SkBitmap> bitmap;
                if(!loaderResources.obtainMapIcon(iconSymbol->resourceName, bitmap) || !bitmap)
                    continue;
				std::shared_ptr<TextureBlock> blocker(new TextureBlock());
				blocker->Subrect.left = 0;
				blocker->Subrect.right = bitmap->width();
				blocker->Subrect.bottom = bitmap->height();
				blocker->Subrect.top = 0;
				blocker->textureHandle = textureMapNames.size();
				blocker->texName = iconSymbol->resourceName;
				blocker->bitmapLink = std::move(bitmap);
				textureMapNames.insert(blocker);
				insertedNewSymbol = true;
			}
		}

		symbolDataToScene.push_back(std::move(symbolVal));
	}

	if (insertedNewSymbol)
	{
		packTexture(0, textureMapNames);
	}
	_workCtx.swap(currCtx);
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

HRESULT AtlasMapDxRender::Initialize(HWND baseHWND, int numLayers)
{
	return pImpl->Initialize(baseHWND, numLayers);
}

HRESULT AtlasMapDxRender::ResizeWindow(HWND windowResized)
{
	return pImpl->ResizeWindow(windowResized);
}

void AtlasMapDxRender::saveSkBitmapToResource(int textureID, const SkBitmap& skBitmap, int xoffset, int yoffset)
{
	pImpl->saveSkBitmapToResource(textureID, skBitmap, xoffset, yoffset);
}
void AtlasMapDxRender::renderScene()
{
	pImpl->renderScene();
}

//void AtlasMapDxRender::packTexture(int textureID, const std::unordered_map<int32_t, std::shared_ptr<const SkBitmap>>& bitmaps)
//{
//	//pImpl->packTexture(textureID, bitmaps);
//}

void AtlasMapDxRender::updateTexture(std::vector<std::shared_ptr<MapRasterizer::RasterSymbolGroup>>& symbolData, std::shared_ptr<MapRasterizerContext> currCtx)
{
	pImpl->updateTexture(symbolData, currCtx);
}