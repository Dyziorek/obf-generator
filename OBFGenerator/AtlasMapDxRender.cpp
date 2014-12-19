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
#include "TexturePacker.h"
#include "SkBitmap.h"
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

using namespace DirectX;

#include "AtlasMapDxRender.h"

class AtlasMapDxRender::_Impl 
{
private:

	enum textureTypeID
	{ 
		mapTexture = 0, 
		fontTexture = 1
	};

public:
	_Impl(void);
	~_Impl(void);

	HRESULT Initialize(HWND baseHWND, int numLayers);
	void saveSkBitmapToResource(int textureID, const SkBitmap& skBitmap, int xoffset, int yoffset);
	void packTexture(int textureID, std::unordered_map<std::string,  std::shared_ptr<const TextureBlock>>& textureBlocks, const std::unordered_map<int32_t, const std::shared_ptr<const SkBitmap>>& bitmaps);
	void drawDataToTexture( const std::unordered_map<int32_t, const std::shared_ptr<const SkBitmap>>& bitmaps, std::vector<std::shared_ptr<TextureBlock>>& hints, int8_t numTextures);
	void updateTexture(std::vector<std::shared_ptr<MapRasterizer::RasterSymbolGroup>>& symbolData);
	void renderScene();
private:
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
	/*
	std::unique_ptr<BasicEffect>                            g_BatchEffect;
	std::unique_ptr<EffectFactory>                          g_FXFactory;
	std::unique_ptr<GeometricPrimitive>                     g_Shape;
	std::unique_ptr<Model>                                  g_Model;
	std::unique_ptr<PrimitiveBatch<VertexPositionColor>>    g_Batch;
	*/
	std::unique_ptr<SpriteFont>                             g_Font;
	std::unique_ptr<SpriteBatch>                            g_Sprites;
	std::unordered_map<std::string,  std::shared_ptr<const TextureBlock>> textureMapNames;
	XMMATRIX                            g_World;
	XMMATRIX                            g_View;
	XMMATRIX                            g_Projection;

	XMVECTORF32 ClearColor;
	void InitSymbolTexture();
	void AddTextureLayer(int newLayers);
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

	for (auto numLayer = 0; numLayer < numLayers + 2; numLayer++)
	{
		Microsoft::WRL::ComPtr<ID3D11Resource> resourceElem;
		 Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> textureRV;
		hr = g_pd3dDevice->CreateTexture2D(&desc, nullptr, (ID3D11Texture2D**)resourceElem.GetAddressOf());
		hr = g_pd3dDevice->CreateShaderResourceView(resourceElem.Get(), &SRVDesc, &textureRV);
		textureResourceLayers.push_back(std::make_pair(std::move(resourceElem), std::move(textureRV)));
	}


	

    // Create DirectXTK objects
    g_States.reset( new CommonStates( g_pd3dDevice.Get() ) );
    g_Sprites.reset( new SpriteBatch( g_pImmediateContext.Get() ) );
	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView.Get(), ClearColor);

	InitSymbolTexture();
    return S_OK;
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
	}
}

void AtlasMapDxRender::_Impl::InitSymbolTexture()
{
	std::string initResNames[] = {"accomodation", "administrative", "aerialway_station","finance","fuel","historic_castle","landuse_grass","place_town", "railway_station"
		, "shop_bicycle", "shop_car_repair", "shop_pet", "tourism_hotel", "tourism_motel", "tourism_viewpoint", "tourism"};

	MapRasterizerProvider loaderResources;
	std::unordered_map<int32_t, const std::shared_ptr<const SkBitmap>> texturePackData;
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
			blocker->textureHandle = texturePackData.size();
			texturePackData.insert(std::make_pair(texturePackData.size(), std::move(resData)));
			textureMapNames.insert(std::make_pair(std::move(resName), std::move(blocker)));
		}
	}
	

	packTexture(1, textureMapNames, texturePackData);

}

void  AtlasMapDxRender::_Impl::drawDataToTexture( const std::unordered_map<int32_t, const std::shared_ptr<const SkBitmap>>& bitmaps, std::vector<std::shared_ptr<TextureBlock>>& hints, int8_t numTextures)
{
	HRESULT hr = S_OK;
	if (textureResourceLayers.size() - 2 < numTextures)
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
	}

	for (int texID = 0; texID < textureIDMap.size(); texID++)
	{
		auto layerMapData = textureResourceLayers[texID+2];
		auto resourceData = layerMapData.first;
		
		D3D11_MAPPED_SUBRESOURCE subData;

		hr = g_pImmediateContext->Map(resourceData.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &subData);
		char* mappedData = (char*)subData.pData;
		for(std::shared_ptr<TextureBlock> blockInfo : hints)
		{
			assert(bitmaps.size() > blockInfo->textureHandle);

			if (bitmaps.find((int32_t)blockInfo->textureHandle) != bitmaps.end())
			{
				auto bitmapData = bitmaps.find((int32_t)blockInfo->textureHandle)->second;
				

				char* m_pmap = (char*)bitmapData->getPixels(); 
				int bpl = bitmapData->bytesPerPixel();
				int stride = bitmapData->rowBytes(); 
				int yoffset = blockInfo->Subrect.left;
				int xoffset = blockInfo->Subrect.top;
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
						memcpy(mappedData+(xoffset*bpl), m_pmap+(xoffset*bpl), stride);
						mappedData += subData.RowPitch;
						m_pmap += stride;
					}
				}
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

	RECT src;
	src.top = 0;
	src.left = 0;
	src.bottom = 1024;
	src.right = 1024;

	g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView.Get(), ClearColor);

	// map layer texture
	auto layerData = textureResourceLayers[2];
	for (auto layerData : textureResourceLayers)
	{
		g_Sprites->Begin();
		g_Sprites->Draw(layerData.second.Get(), rc, &rc);
		g_Sprites->End();
	}
	g_pSwapChain->Present(0,0);
}


void AtlasMapDxRender::_Impl::packTexture(int textureID, std::unordered_map<std::string,  std::shared_ptr<const TextureBlock>>& blockData, const std::unordered_map<int32_t, const std::shared_ptr<const SkBitmap>>& bitmaps)
{
	TexturePacker packerData;

	

	int numLayers = 0;

	
	std::vector<std::shared_ptr<TextureBlock>> results;
	std::vector<const std::shared_ptr<const TextureBlock>> inputs;
	for(auto block : blockData)
	{
		inputs.push_back(block.second);
	}

	if (packerData.packTexture(inputs, 128, results))
	{
		for (int textureHandleID = 0; textureHandleID < results.size(); textureHandleID++)
		{
			if (numLayers < results[textureHandleID]->textureID+1)
			{
				numLayers = results[textureHandleID]->textureID+1;
			}
		}

		drawDataToTexture(bitmaps, results, numLayers);
	}
}

void AtlasMapDxRender::_Impl::updateTexture(std::vector<std::shared_ptr<MapRasterizer::RasterSymbolGroup>>& symbolData)
{
	MapRasterizerProvider loaderResources;
	
	std::unordered_map<int32_t, const std::shared_ptr<const SkBitmap>> bitmapIdHash;

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
						textureMapNames.insert(std::make_pair(std::move(textSymbol->shieldResourceName), std::move(blocker)));
						bitmapIdHash.insert(std::make_pair(blocker->textureHandle, textShieldBitmap));
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
				textureMapNames.insert(std::make_pair(std::move(iconSymbol->resourceName), std::move(blocker)));
				bitmapIdHash.insert(std::make_pair(blocker->textureHandle, bitmap));
			}
		}
	}

	packTexture(0, textureMapNames, bitmapIdHash);
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
void AtlasMapDxRender::saveSkBitmapToResource(int textureID, const SkBitmap& skBitmap, int xoffset, int yoffset)
{
	pImpl->saveSkBitmapToResource(textureID, skBitmap, xoffset, yoffset);
}
void AtlasMapDxRender::renderScene()
{
	pImpl->renderScene();
}

void AtlasMapDxRender::packTexture(int textureID, const std::unordered_map<int32_t, std::shared_ptr<const SkBitmap>>& bitmaps)
{
	//pImpl->packTexture(textureID, bitmaps);
}

void AtlasMapDxRender::updateTexture(std::vector<std::shared_ptr<MapRasterizer::RasterSymbolGroup>>& symbolData)
{
	pImpl->updateTexture(symbolData);
}