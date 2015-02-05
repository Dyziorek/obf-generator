#include "stdafx.h"
#include "MapRasterizerProvider.h"
#include "MapRasterizerContext.h"
#include "D2DRenderer.h"

D2DRenderer::D2DRenderer(void)
{
	_AFX_D2D_STATE* pD2DState = AfxGetD2DState();
	if (pD2DState == NULL)
	{
		return;
	}

}


D2DRenderer::~D2DRenderer(void)
{
}



bool D2DRenderer::DrawMap(CRenderTarget& canvas, MapRasterizerContext& contextData, MapRasterizerProvider& source)
{
	_AFX_D2D_STATE* pD2DState = AfxGetD2DState();
	if (pD2DState == NULL)
	{
		return false;
	}
}