#include <assert.h>
#include <d3d11.h>
#include <d3dx11.h>
#include <FreeImage.h>
#include <limits>
#include <new>
#include <SDL/SDL.h>
#include <SDL/SDL_syswm.h>
#include <string>
#include <windows.h>
#include <xnamath.h>
#include "DX11ConstantBuffer.h"
#include "DX11DepthStencil.h"
#include "DX11DepthStencilState.h"
#include "DX11FVF.h"
#include "DX11InputLayout.h"
#include "DX11PixelShader.h"
#include "DX11RenderDevice.h"
#include "DX11RenderTarget.h"
#include "DX11SamplerState.h"
#include "DX11Scene.h"
#include "DX11StateManager.h"
#include "DX11Texture.h"
#include "DX11VertexShader.h"
#include "Heap.h"
#include "Intersection.h"
#include "ProcTexcoordMatrix2DAutoAlign.h"
#include "ShapeBuilder.h"
#include "SimdMat4x4.h"

#include "../deferred/Deferred.h"

static uint32_t s_allocCount = 0;
static uint32_t s_totalAllocCount = 0;

void * operator new(size_t size) throw()
{
	s_allocCount++;
	s_totalAllocCount++;
	if (s_totalAllocCount % 100 == 0)
		printf("total alloc count: %u\n", s_totalAllocCount);
	return malloc(size);
}

void * operator new[](size_t size) throw()
{
	s_allocCount++;
	return malloc(size);
}

void operator delete(void * p)
{
	free(p);
	s_allocCount--;
}

void operator delete[](void * p)
{
	free(p);
	s_allocCount--;
}

#define ENABLE_FULLSCREEN 0
#define ENABLE_TESSALATION 0
#define ENABLE_GS 0

static ID3D11ShaderResourceView * s_ppNullShaderResources[8] = { 0 };

static XMMATRIX ToXMMATRIX(const Mat4x4 & mat)
{
	return XMMatrixSet(
		mat(0, 0), mat(0, 1), mat(0, 2), mat(0, 3),
		mat(1, 0), mat(1, 1), mat(1, 2), mat(1, 3),
		mat(2, 0), mat(2, 1), mat(2, 2), mat(2, 3),
		mat(3, 0), mat(3, 1), mat(3, 2), mat(3, 3));
}

static XMMATRIX ToXMMATRIX(const SimdMat4x4 & _mat)
{
	SimdMat4x4 mat = _mat.CalcTranspose();
	return XMMatrixSet(
		mat(0, 0), mat(0, 1), mat(0, 2), mat(0, 3),
		mat(1, 0), mat(1, 1), mat(1, 2), mat(1, 3),
		mat(2, 0), mat(2, 1), mat(2, 2), mat(2, 3),
		mat(3, 0), mat(3, 1), mat(3, 2), mat(3, 3));
}

static SimdMat4x4 ToSimdMat4x4(const XMMATRIX & mat)
{
	return SimdMat4x4(
		mat(0, 0), mat(0, 1), mat(0, 2), mat(0, 3),
		mat(1, 0), mat(1, 1), mat(1, 2), mat(1, 3),
		mat(2, 0), mat(2, 1), mat(2, 2), mat(2, 3),
		mat(3, 0), mat(3, 1), mat(3, 2), mat(3, 3));
}

static void TestAllocation();
static void TestIntersection();
static void TestVirtualAlloc();

struct InstanceData
{
	XMMATRIX transform;
	XMFLOAT4 color;
	float blurRadius;
	XMFLOAT2 sunPos;
	float targetSx;
	float targetSy;
};

struct
{
	float x;
	float y;
	float vx;
	float vy;
	bool godray;
} sParticles[16];

static void SetRenderTargets(
	ID3D11DeviceContext * pDeviceCtx,
	ID3D11RenderTargetView * pRT1,
	ID3D11RenderTargetView * pRT2,
	ID3D11RenderTargetView * pRT3,
	ID3D11RenderTargetView * pRT4,
	uint32_t numRenderTargets,
	ID3D11DepthStencilView * pDepthStencil)
{
	ID3D11RenderTargetView * pRTs[4] = { pRT1, pRT2, pRT3, pRT4 };

	D3D11_VIEWPORT vp;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;

	bool found = false;
	UINT sx = 0;
	UINT sy = 0;

	for (uint32_t i = 0; i < numRenderTargets; ++i)
	{
		if (pRTs[i])
		{
			ID3D11Resource * pResource = 0;
			pRTs[i]->GetResource(&pResource);
			A(pResource != 0);
			ID3D11Texture2D * pTexture = 0;
			pResource->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&pTexture);
			A(pTexture != 0);
			D3D11_TEXTURE2D_DESC desc;
			pTexture->GetDesc(&desc);
			pTexture->Release();
			if (found)
			{
				assert(sx == desc.Width);
				assert(sy == desc.Height);
			}
			else
			{
				sx = desc.Width;
				sy = desc.Height;
				found = true;
			}
		}
	}

	assert(found);

	vp.Width = static_cast<float>(sx);
	vp.Height = static_cast<float>(sy);

	pDeviceCtx->RSSetViewports(1, &vp);

	pDeviceCtx->OMSetRenderTargets(numRenderTargets, pRTs, pDepthStencil);
}

static void ClearRenderTarget(ID3D11DeviceContext * pDeviceCtx, ID3D11RenderTargetView * pRT, float r, float g, float b, float a)
{
	const float rgba[4] = { r, g, b, a };
	pDeviceCtx->ClearRenderTargetView(pRT, rgba);
}

static void RenderFullscreenQuad(
	ID3D11Device * pDevice,
	ID3D11DeviceContext * pDeviceCtx,
	Scene * pScene,
	VertexShader & vs,
	XMFLOAT4 texcoords,
	uint32 fvf = FVF_XYZ | FVF_TEX1)
{
	IMemAllocator * pScratchHeap = pScene->GetScratchHeap();

	InputLayout * pInputLayout = CreateInputLayoutFromFVF(pDevice, pScratchHeap, vs, fvf);

	//

	struct Vertex
	{
		float x, y, z;
		float u, v;
	};

	const Vertex vertices[] =
	{
		{ -1.f, -1.f, .5f, texcoords.x, texcoords.y },
		{ +1.f, -1.f, .5f, texcoords.z, texcoords.y },
		{ -1.f, +1.f, .5f, texcoords.x, texcoords.w },
		{ +1.f, +1.f, .5f, texcoords.z, texcoords.w }
	};

	D3D11_BUFFER_DESC bufferVBDesc;
	ZeroMemory(&bufferVBDesc, sizeof(bufferVBDesc));
	bufferVBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferVBDesc.ByteWidth = sizeof(vertices);
	bufferVBDesc.CPUAccessFlags = 0;
	bufferVBDesc.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA bufferVBInit;
	ZeroMemory(&bufferVBInit, sizeof(bufferVBInit));
	bufferVBInit.pSysMem = vertices;

	ID3D11Buffer * pBufferVB = 0;
	V(pDevice->CreateBuffer(&bufferVBDesc, &bufferVBInit, &pBufferVB));

	//

	const UINT stride = sizeof(Vertex);
	const UINT offset = 0;
	pDeviceCtx->IASetVertexBuffers(0, 1, &pBufferVB, &stride, &offset);
	pDeviceCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	pDeviceCtx->IASetInputLayout(pInputLayout->GetInputLayout());

	pDeviceCtx->Draw(4, 0);

	//

	pBufferVB->Release();
	pBufferVB = 0;

	pScratchHeap->SafeDelete(pInputLayout);
}

static void RenderMesh(
	ID3D11Device * pDevice,
	ID3D11DeviceContext * pDeviceCtx,
	Scene * pScene,
	Mesh & mesh,
	VertexShader & vs)
{
	IMemAllocator * pScratchHeap = pScene->GetScratchHeap();

	ResVB * vb = mesh.GetVB();

	const uint32_t fvf = vb->GetFVF();

	InputLayout * pInputLayout = CreateInputLayoutFromFVF(pDevice, pScratchHeap, vs, fvf);
	ID3D11Buffer * pBufferVB = CreateBufferFromFVFWithData(pDevice, pScratchHeap, fvf, vb);
	const UINT stride = static_cast<UINT>(GetVetexSizeUsingFVF(fvf));

	//

	const UINT offset = 0;
	pDeviceCtx->IASetVertexBuffers(0, 1, &pBufferVB, &stride, &offset);
	pDeviceCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pDeviceCtx->IASetInputLayout(pInputLayout->GetInputLayout());

	pDeviceCtx->Draw(vb->GetVertexCnt(), 0);

	//

	SafeRelease(pBufferVB);
	pScratchHeap->SafeDelete<InputLayout>(pInputLayout);
}

static Texture2D * CreateRandomTexture(ID3D11Device * pDevice, DXGI_FORMAT format, uint32_t sx, uint32_t sy)
{
	uint32_t bytesPerPixel = 0;

	switch (format)
	{
	case DXGI_FORMAT_R8G8B8A8_UNORM:
		bytesPerPixel = 4;
		break;
	default:
		assert(false);
	}

	uint32_t pitch = bytesPerPixel * sx;
	uint32_t byteCount = bytesPerPixel * sx * sy;

	uint8_t * pBytes = new uint8_t[byteCount];

	for (uint32_t i = 0; i < byteCount; ++i)
		pBytes[i] = rand() & 255;

	Texture2D * pTexture = new Texture2D(pDevice, sx, sy, format, pBytes, pitch);

	delete[] pBytes;
	pBytes = 0;

	static const char debugName[] = "RandomTexture";
	V(pTexture->GetTexture()->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(debugName), debugName));

	return pTexture;
}

static Texture2D * CreateRadialTexture(ID3D11Device * pDevice, uint32_t s)
{
	BYTE * pBytes = new BYTE[s * s];

	float mid = (s - 1) / 2.0f;
	float radius = mid - 1.0f;

	for (uint32_t y = 0; y < s; ++y)
	{
		BYTE * pLine = pBytes + y * s;

		for (uint32_t x = 0; x < s; ++x)
		{
			float dx = x - mid;
			float dy = y - mid;
			float d = sqrt(dx * dx + dy * dy);
			float i = 1.0f - d / radius;

			if (i < 0.0f)
				i = 0.0f;

			pLine[x] = static_cast<BYTE>(i * 255.0f);
		}
	}

	Texture2D * pTexture = new Texture2D(pDevice, s, s, DXGI_FORMAT_R8_UNORM, pBytes, s);

	delete[] pBytes;
	pBytes = 0;

	static const char debugName[] = "RadialTexture";
	V(pTexture->GetTexture()->SetPrivateData(WKPDID_D3DDebugObjectName, strlen(debugName), debugName));

	return pTexture;
}

static SimdMat4x4 GetMirrorMatrix(const SimdMat4x4 & mat, SimdVecArg plane)
{
	SimdVec n2 = plane.Permute(VEC_ZERO, 0,1,2,7).Mul(SimdVec(2.f));
	SimdVec d = plane.ReplicateW();

	SimdVec vX = plane.Dot3(mat.m_rows[0]);
	SimdVec vY = plane.Dot3(mat.m_rows[1]);
	SimdVec vZ = plane.Dot3(mat.m_rows[2]);
	SimdVec vT = plane.Dot3(mat.m_rows[3]).Sub(d);

	SimdMat4x4 r;

	r.m_rows[0] = mat.m_rows[0].Sub(vX.Mul(n2));
	r.m_rows[1] = mat.m_rows[1].Sub(vY.Mul(n2));
	r.m_rows[2] = mat.m_rows[2].Sub(vZ.Mul(n2));
	r.m_rows[3] = mat.m_rows[3].Sub(vT.Mul(n2));

	return r;
}

static void TestDeferred();
static void TestGodRays();

int main(int argc, char * argv[])
{
	//TestDeferredStuff();

	//TestAllocation();
	//TestIntersection();
	//TestVirtualAlloc();
	TestDeferred();
	//TestGodRays();

	ExitProcess(0);

	return 0;
}

static void TestDeferred()
{
#if 0
	const uint32_t sx = 1920;
	const uint32_t sy = 1080;
#else
	const uint32_t sx = 512;
	const uint32_t sy = 512;
#endif
	const bool fullscreen = sx == 1920;

	SDL_Init(SDL_INIT_EVERYTHING);

	// create window

	SDL_Surface * pSurface = SDL_SetVideoMode(sx, sy, 32, fullscreen ? 0 : 0);

	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	SDL_GetWMInfo(&wminfo);
	HWND hWnd = wminfo.window;

	SDL_ShowCursor(1);

	{
	// create device & swap chain

	RenderDevice device;

	device.Initialize(sx, sy, hWnd, fullscreen);

	ID3D11Device * pDevice = device.GetDevice();
	ID3D11DeviceContext * pDeviceCtx = device.GetDeviceCtx();
	ID3D11RenderTargetView * pBackBufferView = device.GetBackBufferView();

	StateManager deviceMgr(pDevice);

	Scene scene;

	DepthStencil depthStencil(
		pDevice, sx, sy,
		DXGI_FORMAT_R16_TYPELESS,
		DXGI_FORMAT_D16_UNORM,
		DXGI_FORMAT_R16_UNORM);

	// setup shaders

	const char * geometryDepthDefines[] = { "DEPTH_ONLY", "1", 0 };

	VertexShader passthroughVS(pDevice, "test/shaders/deferred.hlsl", "vs_passthrough", 0);
	VertexShader geometryDepthVS(pDevice, "test/shaders/deferred.hlsl", "vs_geometry", geometryDepthDefines);
	PixelShader geometryDepthPS(pDevice, "test/shaders/deferred.hlsl", "ps_geometry", geometryDepthDefines);
	VertexShader geometryVS(pDevice, "test/shaders/deferred.hlsl", "vs_geometry", 0);
	PixelShader geometryPS(pDevice, "test/shaders/deferred.hlsl", "ps_geometry", 0);
	VertexShader lightVS(pDevice, "test/shaders/deferred.hlsl", "vs_light", 0);
	PixelShader lightPS(pDevice, "test/shaders/deferred.hlsl", "ps_light", 0);
	VertexShader compositeVS(pDevice, "test/shaders/deferred.hlsl", "vs_composite", 0);
	PixelShader compositePS(pDevice, "test/shaders/deferred.hlsl", "ps_composite", 0);

	// setup G-buffer

	RenderTarget normalRenderTarget(pDevice, sx, sy, DXGI_FORMAT_R32G32B32A32_FLOAT);
	RenderTarget albedoRenderTarget(pDevice, sx, sy, DXGI_FORMAT_R32G32B32A32_FLOAT);
	RenderTarget lightRenderTarget(pDevice, sx, sy, DXGI_FORMAT_R32G32B32A32_FLOAT);

	typedef struct
	{
		XMMATRIX wvp;
		XMMATRIX w;
	} GeometryState;

	ConstantBuffer geometryCB(pDevice, sizeof(GeometryState));

	typedef struct
	{
		XMMATRIX wvp;
		XMMATRIX invP;
	} LightState;

	ConstantBuffer lightCB(pDevice, sizeof(LightState));

	typedef struct
	{
		XMMATRIX wvp;
	} CompositeState;

	ConstantBuffer compositeCB(pDevice, sizeof(CompositeState));

	SamplerState ssLinearWrap(pDevice, D3D11_FILTER_MIN_MAG_MIP_LINEAR, false);
	SamplerState ssPointClamp(pDevice, D3D11_FILTER_MIN_MAG_MIP_POINT, true);

	Texture2D * pRandomTexture = CreateRandomTexture(pDevice, DXGI_FORMAT_R8G8B8A8_UNORM, 256, 256);
	Texture2D * pLightTexture = CreateRadialTexture(pDevice, 256);

	// initialize objects

	float t = 0.0f;
	float mousePos[2] = { 0.0f, 0.0f };

	bool stop = false;

	while (stop == false)
	{
		// process events

		SDL_Event e;

		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_KEYDOWN:
				if (e.key.keysym.sym == SDLK_ESCAPE)
					stop = true;
				break;
			case SDL_MOUSEMOTION:
				mousePos[0] = e.motion.x;
				mousePos[1] = e.motion.y;
				break;
			}
		}

		// update logic

		//const float dt = 1.0f / 60.0f;
		const float dt = 1.0f / 200.0f;

		t += dt;

		// render frame

		static int frame = 0;
		++frame;

		scene.Begin();

		SetRenderTargets(pDeviceCtx, normalRenderTarget.GetRenderTarget(), albedoRenderTarget.GetRenderTarget(), 0, 0, 2, depthStencil.GetDepthStencilView());
		ClearRenderTarget(pDeviceCtx, normalRenderTarget.GetRenderTarget(), 0.0f, 0.0f, 0.0f, 0.0f);
		ClearRenderTarget(pDeviceCtx, albedoRenderTarget.GetRenderTarget(), 0.0f, 0.0f, 0.0f, 0.0f);
		pDeviceCtx->ClearDepthStencilView(depthStencil.GetDepthStencilView(), D3D11_CLEAR_DEPTH, 1.0f, 0x00);

		deviceMgr.SetRS(RS_DEPTHTEST, 1);
		deviceMgr.SetRS(RS_CULL, CULL_NONE);
		deviceMgr.SetRS(RS_DEPTHTEST_FUNC, CMP_L);
		deviceMgr.SetRS(RS_WRITE_DEPTH, 1);
		pDeviceCtx->VSSetShader(geometryVS.GetShader(), 0, 0);
		pDeviceCtx->PSSetShader(geometryPS.GetShader(), 0, 0);
		pDeviceCtx->PSSetShaderResources(1, 1, pRandomTexture->GetShaderResourceViewPP());
		pDeviceCtx->PSSetSamplers(1, 1, ssLinearWrap.GetSamplerStatePP());

		XMMATRIX matInvP;

		GeometryState geometryState;
		{
			XMMATRIX matW = XMMatrixRotationX(t) * XMMatrixRotationY(t / 1.123f) * XMMatrixRotationZ(t / 1.234f) * XMMatrixTranslation(0.0f, -1.0f, 2.0f);

#if 1
			if (frame % 2)
			{
				SimdMat4x4 m = ToSimdMat4x4(matW);
				m = GetMirrorMatrix(m, SimdVec(0.f, -1.f, 0.f, 0.5f));
				matW = ToXMMATRIX(m);
			}
#endif

			XMMATRIX matV = XMMatrixIdentity();
			XMMATRIX matP = XMMatrixPerspectiveFovLH(3.14f/2.0f, 1.0f, 1.0f, 3.0f);
			geometryState.wvp = XMMatrixTranspose(matW * matV * matP);
			geometryState.w = XMMatrixTranspose(matW);
			XMVECTOR det;
			matInvP = XMMatrixInverse(&det, matP);
		}
		geometryCB.Update(pDeviceCtx, &geometryState);
		pDeviceCtx->VSSetConstantBuffers(0, 1, geometryCB.GetConstantBufferPP());

		scene.GetScratchHeap()->Push();
		{
			Mesh mesh;

			ShapeBuilder sb(scene.GetScratchHeap(), scene.GetScratchHeap());
			sb.PushScaling(Vec3(1.0f, 1.0f, 1.0f));
				sb.CreateCube(scene.GetScratchHeap(), mesh, FVF_XYZ | FVF_NORMAL | FVF_TEX1);
			sb.Pop();
			//sb.ConvertToIndexed(mesh, mesh);
			sb.CalculateNormals(mesh);
			sb.ConvertToNonIndexed(scene.GetScratchHeap(), mesh, mesh);
			ProcTexcoordMatrix2DAutoAlign procTexcoord;
			sb.CalculateTexcoords(mesh, 0, &procTexcoord);

			deviceMgr.MakeCurrent(pDeviceCtx);
			RenderMesh(pDevice, pDeviceCtx, &scene, mesh, geometryVS);
		}
		scene.GetScratchHeap()->Pop();

		pDeviceCtx->PSSetShaderResources(0, 8, s_ppNullShaderResources);

		// render deferred lights

		SetRenderTargets(pDeviceCtx, lightRenderTarget.GetRenderTarget(), 0, 0, 0, 1, 0);
		ClearRenderTarget(pDeviceCtx, lightRenderTarget.GetRenderTarget(), 0.0f, 0.0f, 0.0f, 1.0f);
		pDeviceCtx->VSSetShader(lightVS.GetShader(), 0, 0);
		pDeviceCtx->PSSetShader(lightPS.GetShader(), 0, 0);
		pDeviceCtx->PSSetShaderResources(0, 1, normalRenderTarget.GetShaderResourceViewPP());
		pDeviceCtx->PSSetSamplers(0, 1, ssPointClamp.GetSamplerStatePP());
		pDeviceCtx->PSSetShaderResources(1, 1, depthStencil.GetShaderResourceViewPP());
		pDeviceCtx->PSSetSamplers(1, 1, ssPointClamp.GetSamplerStatePP());
		pDeviceCtx->PSSetShaderResources(2, 1, pLightTexture->GetShaderResourceViewPP());
		pDeviceCtx->PSSetSamplers(2, 1, ssLinearWrap.GetSamplerStatePP());
		LightState lightState;
		{
			XMMATRIX matP = XMMatrixOrthographicOffCenterLH(-1.0f, +1.0f, +1.0f, -1.0f, -10.0f, +10.0f);
			lightState.wvp = XMMatrixTranspose(matP);
			lightState.invP = XMMatrixTranspose(matInvP);
		}
		lightCB.Update(pDeviceCtx, &lightState);
		pDeviceCtx->VSSetConstantBuffers(0, 1, lightCB.GetConstantBufferPP());
		RenderFullscreenQuad(pDevice, pDeviceCtx, &scene, lightVS, XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));

		pDeviceCtx->PSSetShaderResources(0, 8, s_ppNullShaderResources);

		// composite image into back buffer

		SetRenderTargets(pDeviceCtx, pBackBufferView, 0, 0, 0, 1, 0);
		ClearRenderTarget(pDeviceCtx, pBackBufferView, 0.0f, 0.0f, 0.5f, 1.0f);
		pDeviceCtx->VSSetShader(compositeVS.GetShader(), 0, 0);
		pDeviceCtx->PSSetShader(compositePS.GetShader(), 0, 0);
		pDeviceCtx->PSSetShaderResources(0, 1, albedoRenderTarget.GetShaderResourceViewPP());
		pDeviceCtx->PSSetSamplers(0, 1, ssPointClamp.GetSamplerStatePP());
		pDeviceCtx->PSSetShaderResources(1, 1, lightRenderTarget.GetShaderResourceViewPP());
		pDeviceCtx->PSSetSamplers(0, 1, ssPointClamp.GetSamplerStatePP());
		CompositeState compositeState;
		{
			XMMATRIX matP = XMMatrixOrthographicOffCenterLH(-1.0f, +1.0f, +1.0f, -1.0f, -10.0f, +10.0f);
			compositeState.wvp = XMMatrixTranspose(matP);
		}
		compositeCB.Update(pDeviceCtx, &compositeState);
		pDeviceCtx->VSSetConstantBuffers(0, 1, compositeCB.GetConstantBufferPP());
		RenderFullscreenQuad(pDevice, pDeviceCtx, &scene, compositeVS, XMFLOAT4(0.0f, 0.0f, (float)sx, (float)sy));
		pDeviceCtx->PSSetShaderResources(0, 8, s_ppNullShaderResources);
		V(device.GetSwapChain()->Present(0, 0));

		scene.End();
	}

	delete pLightTexture;
	pLightTexture = 0;

	delete pRandomTexture;
	pRandomTexture = 0;
	}

	SDL_Quit();
}

static void TestGodRays()
{
#if ENABLE_FULLSCREEN
	const int sx = 1920;
	const int sy = 1080;
#else
	const int sx = 1024;
	const int sy = 512;
#endif
	const bool fullscreen = sx == 1920;

	SDL_Init(SDL_INIT_EVERYTHING);

	// create window

	SDL_Surface * pSurface = SDL_SetVideoMode(sx, sy, 32, fullscreen ? 0 : 0);

	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	SDL_GetWMInfo(&wminfo);
	HWND hWnd = wminfo.window;

	SDL_ShowCursor(1);

	// create device & swap chain

	RenderDevice device;

	device.Initialize(sx, sy, hWnd, fullscreen);

	ID3D11Device * pDevice = device.GetDevice();
	ID3D11DeviceContext * pDeviceCtx = device.GetDeviceCtx();
	ID3D11RenderTargetView * pBackBufferView = device.GetBackBufferView();

	// scene

	Scene * pScene = new Scene();

	// create depth / stencil render target

	DepthStencil depthStencil(pDevice, sx, sy, DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_D16_UNORM, DXGI_FORMAT_R16_UNORM);

	// setup shaders

	VertexShader passthroughVS(pDevice, "test/shaders/shader.hlsl", "vs_passthrough", 0);
	VertexShader shaderVS(pDevice, "test/shaders/shader.hlsl", "vsmain", 0);
	PixelShader shaderPS(pDevice, "test/shaders/shader.hlsl", "psmain", 0);
	PixelShader blitPS(pDevice, "test/shaders/shader.hlsl", "ps_blit", 0);
	PixelShader blurPS(pDevice, "test/shaders/shader.hlsl", "ps_blur", 0);
	PixelShader blurAlphaPS(pDevice, "test/shaders/shader.hlsl", "ps_blur_alpha", 0);
	PixelShader godraysPS(pDevice, "test/shaders/shader.hlsl", "ps_godrays", 0);
	SamplerState linearClampSS(pDevice, D3D11_FILTER_MIN_MAG_MIP_LINEAR, true);
	SamplerState linearWrapSS(pDevice, D3D11_FILTER_MIN_MAG_MIP_LINEAR, false);
	SamplerState pointClampSS(pDevice, D3D11_FILTER_MIN_MAG_MIP_POINT, true);
	SamplerState pointWrapSS(pDevice, D3D11_FILTER_MIN_MAG_MIP_POINT, false);

	//

#if ENABLE_TESSALATION
	ID3D10Blob * pShaderHSBytes = 0;
	ID3D10Blob * pShaderDSBytes = 0;

	UINT shaderFlagsHS = D3D10_SHADER_DEBUG | D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_WARNINGS_ARE_ERRORS;
	UINT shaderFlagsDS = D3D10_SHADER_DEBUG | D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_WARNINGS_ARE_ERRORS;

	V(D3DX11CompileFromFile(TEXT("test/shaders/shader.hlsl"), 0, 0, "hsmain", "ps_5_0", shaderFlagsHS, 0, 0, &pShaderHSBytes, &pShaderErrorBytes, 0));
	if (pShaderErrorBytes)
	{
		std::string errorString;
		errorString.resize(pShaderErrorBytes->GetBufferSize());
		memcpy(&errorString[0], pShaderErrorBytes->GetBufferPointer(), errorString.size());
		printf("error: %s\n", errorString.c_str());
		pShaderErrorBytes->Release();
		pShaderErrorBytes = 0;
	}
	V(D3DX11CompileFromFile(TEXT("test/shaders/shader.hlsl"), 0, 0, "dsmain", "ds_5_0", shaderFlagsDS, 0, 0, &pShaderDSBytes, &pShaderErrorBytes, 0));
	if (pShaderErrorBytes)
	{
		std::string errorString;
		errorString.resize(pShaderErrorBytes->GetBufferSize());
		memcpy(&errorString[0], pShaderErrorBytes->GetBufferPointer(), errorString.size());
		printf("error: %s\n", errorString.c_str());
		pShaderErrorBytes->Release();
		pShaderErrorBytes = 0;
	}
	
	ID3D11HullShader * pShaderHS = 0;
	ID3D11DomainShader * pShaderDS = 0;

	V(pDevice->CreateHullShader(pShaderHSBytes->GetBufferPointer(), pShaderHSBytes->GetBufferSize(), 0, &pShaderHS));
	V(pDevice->CreateDomainShader(pShaderDSBytes->GetBufferPointer(), pShaderDSBytes->GetBufferSize(), 0, &pShaderDS));
#endif

	//

#if ENABLE_GS
	ID3D10Blob * pShaderGSBytes = 0;

	UINT shaderFlagsGS = D3D10_SHADER_DEBUG | D3D10_SHADER_ENABLE_STRICTNESS | D3D10_SHADER_WARNINGS_ARE_ERRORS;

	V(D3DX11CompileFromFile(TEXT("test/shaders/shader.hlsl"), 0, 0, "gsmain", "gs_4_0", shaderFlagsGS, 0, 0, &pShaderGSBytes, &pShaderErrorBytes, 0));
	if (pShaderErrorBytes)
	{
		std::string errorString;
		errorString.resize(pShaderErrorBytes->GetBufferSize());
		memcpy(&errorString[0], pShaderErrorBytes->GetBufferPointer(), errorString.size());
		printf("error: %s\n", errorString.c_str());
		pShaderErrorBytes->Release();
		pShaderErrorBytes = 0;
	}
	
	ID3D11GeometryShader * pShaderGS = 0;

	V(pDevice->CreateGeometryShader(pShaderGSBytes->GetBufferPointer(), pShaderGSBytes->GetBufferSize(), 0, &pShaderGS));
#endif

	//

	D3D11_INPUT_ELEMENT_DESC elems[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	ID3D11InputLayout * pInputLayout = 0;
	V(pDevice->CreateInputLayout(elems, _countof(elems), shaderVS.GetByteCode()->GetBufferPointer(), shaderVS.GetByteCode()->GetBufferSize(), &pInputLayout));

	//

	struct Vertex
	{
		float x, y, z;
	};

	const float psx = 60.f;
	const float psy = 20.f;

	const Vertex vertices[] =
	{
		{ -psx, -psy, .5f },
		{ +psx, -psy, .5f },
		{ +psx, +psy, .5f },
		{ -psx, +psy, .5f }
	};

	D3D11_BUFFER_DESC bufferVBDesc;
	ZeroMemory(&bufferVBDesc, sizeof(bufferVBDesc));
	bufferVBDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferVBDesc.ByteWidth = sizeof(vertices);
	bufferVBDesc.CPUAccessFlags = 0;
	bufferVBDesc.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA bufferVBInit;
	ZeroMemory(&bufferVBInit, sizeof(bufferVBInit));
	bufferVBInit.pSysMem = vertices;

	ID3D11Buffer * pBufferVB = 0;
	V(pDevice->CreateBuffer(&bufferVBDesc, &bufferVBInit, &pBufferVB));

	//

	const UINT16 indices[] = { 0, 1, 2, 2, 3, 0 };

	D3D11_BUFFER_DESC bufferIBDesc;
	ZeroMemory(&bufferIBDesc, sizeof(bufferIBDesc));
	bufferIBDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferIBDesc.ByteWidth = sizeof(indices);
	bufferIBDesc.CPUAccessFlags = 0;
	bufferIBDesc.Usage = D3D11_USAGE_IMMUTABLE;

	D3D11_SUBRESOURCE_DATA bufferIBInit;
	ZeroMemory(&bufferIBInit, sizeof(bufferIBInit));
	bufferIBInit.pSysMem = indices;

	ID3D11Buffer * pBufferIB = 0;
	V(pDevice->CreateBuffer(&bufferIBDesc, &bufferIBInit, &pBufferIB));

	//

	D3D11_BUFFER_DESC cbDesc;
	ZeroMemory(&cbDesc, sizeof(cbDesc));
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.ByteWidth = sizeof(InstanceData);
	cbDesc.CPUAccessFlags = 0;
	cbDesc.Usage = D3D11_USAGE_DEFAULT;

	ID3D11Buffer * pConstantBuffer = 0;
	V(pDevice->CreateBuffer(&cbDesc, NULL, &pConstantBuffer));

	//

	Texture2D * pRandomTexture = CreateRandomTexture(pDevice, DXGI_FORMAT_R8G8B8A8_UNORM, 256, 256);
	Texture2D * pBackgroundTexture = Texture2D::LoadFromFile(pDevice, "test/background.png");

	//

	ZeroMemory(sParticles, sizeof(sParticles));
	uint32_t particleCount = _countof(sParticles);

	for (uint32_t i = 0; i < particleCount; ++i)
	{
		sParticles[i].x = static_cast<float>(rand() % sx);
		sParticles[i].y = static_cast<float>(rand() % sy);
		bool godray = rand() % 3 == 0;
		if (godray)
		{
			sParticles[i].vx = ((rand() % 1000) / 999.0f - 0.5f) * 10.0f;
			sParticles[i].vy = ((rand() % 1000) / 999.0f - 0.5f) * 10.0f;
		}
		else
		{
			sParticles[i].vx = ((rand() % 1000) / 999.0f - 0.5f) * 50.0f;
			sParticles[i].vy = ((rand() % 1000) / 999.0f - 0.5f) * 50.0f;
		}
		sParticles[i].godray = godray;
	}

	ConstantBuffer cb(pDevice, sizeof(XMMATRIX));

	//

#if 0
	int effectSx = 256;
	int effectSy = 256;
#else
	int effectSx = sx;
	int effectSy = sy;
#endif

	RenderTarget * pEffectRTs[2];
	pEffectRTs[0] = new RenderTarget(pDevice, effectSx, effectSy, DXGI_FORMAT_R8G8B8A8_UNORM);
	pEffectRTs[1] = new RenderTarget(pDevice, effectSx, effectSy, DXGI_FORMAT_R8G8B8A8_UNORM);
#define SwapEffectRTs() do { RenderTarget * pRT = pEffectRTs[0]; pEffectRTs[0] = pEffectRTs[1]; pEffectRTs[1] = pRT; } while (false)

	//

	float t = 0.0f;

	float mousePos[2] = { 0.0f, 0.0f };

	uint32_t frameIdx = 0;

	bool stop = false;

	while (stop == false)
	{
		// process events

		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_KEYDOWN:
				if (e.key.keysym.sym == SDLK_ESCAPE)
					stop = true;
				break;
			case SDL_MOUSEMOTION:
				mousePos[0] = e.motion.x;
				mousePos[1] = e.motion.y;
				break;
			}
		}

		// update

		const float dt = 1.0f / 60.0f;

		// update particles

		for (uint32_t i = 0; i < particleCount; ++i)
		{
#if 1
			sParticles[i].x += sParticles[i].vx * dt;
			sParticles[i].y += sParticles[i].vy * dt;
			#define reflect(p, s, v) do { if (p < 0.0f) { p = -p; s = -s; } else if (p > v) { p = 2.0f * v - p; s = -s; } } while (false)
			reflect(sParticles[i].x, sParticles[i].vx, effectSx);
			reflect(sParticles[i].y, sParticles[i].vy, effectSy);
			#undef reflect
#endif
		}

		// render

		pScene->Begin();

		ID3D11RenderTargetView * pCurrentRTV = 0;

		// render particles to render target

		SetRenderTargets(pDeviceCtx, pEffectRTs[0]->GetRenderTarget(), 0, 0, 0, 1, 0);
		pCurrentRTV = pEffectRTs[0]->GetRenderTarget();

		{
			const float clearColor[4] = { 0.0f, 0.25f, 0.5f, 0.0f };
			pDeviceCtx->ClearRenderTargetView(pCurrentRTV, clearColor);
		}

		// render background texture

		pDeviceCtx->VSSetShader(passthroughVS.GetShader(), 0, 0);
		{
			XMMATRIX mat = XMMatrixOrthographicOffCenterLH(-1.0f, +1.0f, +1.0f, -1.0f, -10.0f, +10.0f);
			mat = XMMatrixTranspose(mat);
			cb.Update(pDeviceCtx, &mat);
			pDeviceCtx->VSSetConstantBuffers(0, 1, cb.GetConstantBufferPP());
		}
		pDeviceCtx->PSSetShader(blitPS.GetShader(), 0, 0);
		pDeviceCtx->PSSetShaderResources(0, 1, pBackgroundTexture->GetShaderResourceViewPP());
		pDeviceCtx->PSSetSamplers(0, 1, pointClampSS.GetSamplerStatePP());
		RenderFullscreenQuad(pDevice, pDeviceCtx, pScene, passthroughVS, XMFLOAT4(0.0f, 0.0f, (float)effectSx, (float)effectSy));
		pDeviceCtx->PSSetShaderResources(0, 8, s_ppNullShaderResources);

		// render particles

		const XMMATRIX matP = XMMatrixOrthographicOffCenterLH(0.0f, static_cast<float>(sx), static_cast<float>(sy), 0.0f, -100.0f, +100.0f);

		pDeviceCtx->IASetInputLayout(pInputLayout);
		const UINT stride = sizeof(Vertex);
		const UINT offset = 0;
		pDeviceCtx->IASetVertexBuffers(0, 1, &pBufferVB, &stride, &offset);
		pDeviceCtx->IASetIndexBuffer(pBufferIB, DXGI_FORMAT_R16_UINT, 0);
		pDeviceCtx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		
		pDeviceCtx->VSSetShader(shaderVS.GetShader(), 0, 0);
		pDeviceCtx->PSSetShader(shaderPS.GetShader(), 0, 0);
#if ENABLE_TESSALATION
		pDeviceCtx->HSSetShader(pShaderHS, 0, 0);
		pDeviceCtx->DSSetShader(pShaderDS, 0, 0);
#endif
#if ENABLE_GS
		pDeviceCtx->GSSetShader(pShaderGS, 0, 0);
#endif

		pDeviceCtx->VSSetConstantBuffers(0, 1, &pConstantBuffer);
		pDeviceCtx->PSSetConstantBuffers(0, 1, &pConstantBuffer);

		for (uint32_t p = 0; p < 2; ++p)
		for (uint32_t i = 0; i < particleCount; ++i)
		{
			if (p == 0 && !sParticles[i].godray)
				continue;
			if (p == 1 && sParticles[i].godray)
				continue;

			InstanceData id;

#if 1
			const XMMATRIX matR = !sParticles[i].godray ? XMMatrixRotationZ(t * i / 100.0f) : XMMatrixRotationZ(t * i / 300.0f);//XMMatrixIdentity();
#else
			const XMMATRIX matR = XMMatrixIdentity();
#endif
			const XMMATRIX matT = XMMatrixTranslation(sParticles[i].x, sParticles[i].y, 0.0f);
			id.transform = XMMatrixTranspose(matR * matT * matP);
			if (sParticles[i].godray)
				id.color = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.0f);
			else
				id.color = XMFLOAT4(0.25f, 0.5f, 1.0f, 0.0f);
			pDeviceCtx->UpdateSubresource(pConstantBuffer, 0, 0, &id, 0, 0);

			pDeviceCtx->DrawIndexed(6, 0, 0);
		}

		// apply effects

#if 1
		pDeviceCtx->VSSetConstantBuffers(0, 1, &pConstantBuffer);
		pDeviceCtx->PSSetConstantBuffers(0, 1, &pConstantBuffer);

		const XMMATRIX mat = XMMatrixOrthographicOffCenterLH(-1.0f, +1.0f, +1.0f, -1.0f, -100.0f, +100.0f);
		InstanceData id;
		id.transform = XMMatrixTranspose(mat);
		id.blurRadius = 5.0f;
		//id.sunPos = XMFLOAT2(sParticles[0].x / effectSx, sParticles[0].y / effectSy);
		//id.sunPos = XMFLOAT2(sx * 0.4f, sy * 0.2f);
		id.sunPos = XMFLOAT2(mousePos[0], mousePos[1]);
		pDeviceCtx->UpdateSubresource(pConstantBuffer, 0, 0, &id, 0, 0);

#if 1
		// god rays

		SwapEffectRTs();

		SetRenderTargets(pDeviceCtx, pEffectRTs[0]->GetRenderTarget(), 0, 0, 0, 1, 0);
		pCurrentRTV = pEffectRTs[0]->GetRenderTarget();

		pDeviceCtx->VSSetShader(passthroughVS.GetShader(), 0, 0);
		{
			XMMATRIX mat = XMMatrixOrthographicOffCenterLH(-1.0f, +1.0f, +1.0f, -1.0f, -10.0f, +10.0f);
			mat = XMMatrixTranspose(mat);
			cb.Update(pDeviceCtx, &mat);
			pDeviceCtx->VSSetConstantBuffers(0, 1, cb.GetConstantBufferPP());
		}
		pDeviceCtx->PSSetShader(godraysPS.GetShader(), 0, 0);
		pDeviceCtx->PSSetShaderResources(0, 1, pEffectRTs[1]->GetShaderResourceViewPP());
		pDeviceCtx->PSSetSamplers(0, 1, linearWrapSS.GetSamplerStatePP());
		pDeviceCtx->PSSetShaderResources(1, 1, pRandomTexture->GetShaderResourceViewPP());
		pDeviceCtx->PSSetSamplers(1, 1, pointWrapSS.GetSamplerStatePP());
		RenderFullscreenQuad(pDevice, pDeviceCtx, pScene, passthroughVS, XMFLOAT4(0.0f, 0.0f, (float)effectSx, (float)effectSy));
		pDeviceCtx->PSSetShaderResources(0, 8, s_ppNullShaderResources);
#endif

#if 1
		// blur alpha

		SwapEffectRTs();

		SetRenderTargets(pDeviceCtx, pEffectRTs[0]->GetRenderTarget(), 0, 0, 0, 1, 0);
		pCurrentRTV = pEffectRTs[0]->GetRenderTarget();

		pDeviceCtx->VSSetShader(passthroughVS.GetShader(), 0, 0);
		{
			XMMATRIX mat = XMMatrixOrthographicOffCenterLH(-1.0f, +1.0f, +1.0f, -1.0f, -10.0f, +10.0f);
			mat = XMMatrixTranspose(mat);
			cb.Update(pDeviceCtx, &mat);
			pDeviceCtx->VSSetConstantBuffers(0, 1, cb.GetConstantBufferPP());
		}
		pDeviceCtx->PSSetShader(blurAlphaPS.GetShader(), 0, 0);
		pDeviceCtx->PSSetShaderResources(0, 1, pEffectRTs[1]->GetShaderResourceViewPP());
		pDeviceCtx->PSSetSamplers(0, 1, linearClampSS.GetSamplerStatePP());
		pDeviceCtx->PSSetShaderResources(1, 1, pRandomTexture->GetShaderResourceViewPP());
		pDeviceCtx->PSSetSamplers(1, 1, pointWrapSS.GetSamplerStatePP());
		//for (uint32_t i = 0; i < 5; ++i)
		RenderFullscreenQuad(pDevice, pDeviceCtx, pScene, passthroughVS, XMFLOAT4(0.0f, 0.0f, (float)effectSx, (float)effectSy));
		pDeviceCtx->PSSetShaderResources(0, 8, s_ppNullShaderResources);
#endif

		// blit render target to back buffer

		SetRenderTargets(pDeviceCtx, pBackBufferView, 0, 0, 0, 1, depthStencil.GetDepthStencilView());
		pCurrentRTV = pBackBufferView;

#if 1
		{
			const float clearColor[4] = { 1.0f, 0.5f, 0.25f, 0.0f };
			pDeviceCtx->ClearRenderTargetView(pCurrentRTV, clearColor);
			pDeviceCtx->ClearDepthStencilView(depthStencil.GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0x00);
		}
#endif

		// blit

		pDeviceCtx->VSSetShader(passthroughVS.GetShader(), 0, 0);
		{
			XMMATRIX mat = XMMatrixOrthographicOffCenterLH(-1.0f, +1.0f, +1.0f, -1.0f, -10.0f, +10.0f);
			mat = XMMatrixTranspose(mat);
			cb.Update(pDeviceCtx, &mat);
			pDeviceCtx->VSSetConstantBuffers(0, 1, cb.GetConstantBufferPP());
		}
		pDeviceCtx->PSSetShader(blitPS.GetShader(), 0, 0);
		pDeviceCtx->PSSetShaderResources(0, 1, pEffectRTs[0]->GetShaderResourceViewPP());
		pDeviceCtx->PSSetSamplers(0, 1, pointClampSS.GetSamplerStatePP());
		RenderFullscreenQuad(pDevice, pDeviceCtx, pScene, passthroughVS, XMFLOAT4(0.0f, 0.0f, (float)effectSx, (float)effectSy));
		pDeviceCtx->PSSetShaderResources(0, 8, s_ppNullShaderResources);
#endif

		pScene->End();

		device.GetSwapChain()->Present(0, 0);

		if ((frameIdx % 100) == 0)
			printf("frame index: %u\n", frameIdx);

		t += dt;

		frameIdx++;
	}

	delete pRandomTexture;
	pRandomTexture = 0;

	delete pEffectRTs[0];
	pEffectRTs[0] = 0;
	delete pEffectRTs[1];
	pEffectRTs[1] = 0;

	pConstantBuffer->Release();
	pConstantBuffer = 0;

	pBufferIB->Release();
	pBufferIB = 0;
	pBufferVB->Release();
	pBufferVB = 0;

	pInputLayout->Release();
	pInputLayout = 0;

#if ENABLE_GS
	pShaderGS->Release();
	pShaderGS = 0;
	pShaderGSBytes->Release();
	pShaderGSBytes = 0;
#endif

#if ENABLE_TESSALATION
	pShaderHS->Release();
	pShaderHS = 0;
	pShaderDS->Release();
	pShaderDS = 0;

	pShaderHSBytes->Release();
	pShaderHSBytes = 0;
	pShaderDSBytes->Release();
	pShaderDSBytes = 0;
#endif

	delete pScene;
	pScene = 0;

	device.Shutdown();

	SDL_FreeSurface(pSurface);
	pSurface = 0;

	SDL_Quit();
}

#define TimingBlock() for (uint32_t i = 0; i != 1; i = 1)

static void RenderCube(
	ID3D11Device * pDevice,
	ID3D11DeviceContext * pDeviceCtx,
	Scene * pScene,
	VertexShader & vs,
	SimdVecArg center,
	SimdVecArg extents)
{
	pScene->GetScratchHeap()->Push();
	{
		Mesh mesh;

		ShapeBuilder sb(pScene->GetScratchHeap(), pScene->GetScratchHeap());
		sb.PushTranslation(Vec3(center(0), center(1), center(2)));
			sb.PushScaling(Vec3(extents(0), extents(1), extents(2)));
				sb.CreateCube(pScene->GetScratchHeap(), mesh, FVF_XYZ | FVF_TEX1);
			sb.Pop();
		sb.Pop();
		sb.ConvertToNonIndexed(pScene->GetScratchHeap(), mesh, mesh);

		RenderMesh(pDevice, pDeviceCtx, pScene, mesh, vs);
	}
	pScene->GetScratchHeap()->Pop();
}

static void RenderCircle(
	ID3D11Device* pDevice,
	ID3D11DeviceContext * pDeviceCtx,
	Scene * pScene,
	VertexShader & vs,
	SimdVecArg center,
	float radius)
{
	pScene->GetScratchHeap()->Push();
	{
		Mesh mesh;

		ShapeBuilder sb(pScene->GetScratchHeap(), pScene->GetScratchHeap());
		sb.PushTranslation(Vec3(center(0), center(1), center(2)));
			sb.PushScaling(Vec3(radius, radius, radius));
				sb.CreateCircle(pScene->GetScratchHeap(), 20, mesh, FVF_XYZ | FVF_TEX1);
			sb.Pop();
		sb.Pop();
		sb.ConvertToNonIndexed(pScene->GetScratchHeap(), mesh, mesh);

		RenderMesh(pDevice, pDeviceCtx, pScene, mesh, vs);
	}
	pScene->GetScratchHeap()->Pop();
}

static void RenderLine(
	ID3D11Device * pDevice,
	ID3D11DeviceContext * pDeviceCtx,
	Scene * pScene,
	VertexShader & vs,
	SimdVecArg p1,
	SimdVecArg p2)
{
	pScene->GetScratchHeap()->Push();
	{
		Mesh mesh;

		ShapeBuilder sb(pScene->GetScratchHeap(), pScene->GetScratchHeap());
		sb.CreateLine(
			pScene->GetScratchHeap(),
			Vec3(p1(0), p1(1), p1(2)),
			Vec3(p2(0), p2(1), p2(2)),
			0.01f,
			mesh, FVF_XYZ | FVF_TEX1);
		sb.ConvertToNonIndexed(pScene->GetScratchHeap(), mesh, mesh);

		RenderMesh(pDevice, pDeviceCtx, pScene, mesh, vs);
	}
	pScene->GetScratchHeap()->Pop();
}

static void TestAllocation()
{
#define REPORTING 0

#if 1
	uint32_t q = 0;

	uint32_t time1 = 0;
#if 1
	MemAllocatorGeneric allocator1(16);
	time1 -= timeGetTime();
	for (uint32_t i = 0; i < 100; ++i)
	{
		void * pAllocs[1000];
		for (uint32_t j = 0; j < 1000; ++j)
		{
			void * p = allocator1.Alloc(16);
			q += reinterpret_cast<uint32_t>(p);
			pAllocs[j] = p;
		}
#if REPORTING == 1
		for (IMemAllocationIterator * pAllocItr = allocator1.GetAllocationIterator(); pAllocItr->HasValue(); pAllocItr->Next())
			printf("alloc: %u\n", pAllocItr->Value()->m_size);
#endif
		for (uint32_t j = 0; j < 1000; ++j)
			allocator1.Free(pAllocs[j]);
	}
	time1 += timeGetTime();
#endif

	uint32_t time2 = 0;
#if 1
	MemAllocatorTransient allocator2(1024 * 1024 * 64);
	time2 -= timeGetTime();
	for (uint32_t j = 0; j < 1000; ++j)
	{
		for (uint32_t i = 0; i < 1000000; ++i)
		{
			void * p = allocator2.Alloc(16);
			q += reinterpret_cast<uint32_t>(p);
		}
#if REPORTING == 1
		for (IMemAllocationIterator * pAllocItr = allocator2.GetAllocationIterator(); pAllocItr->HasValue(); pAllocItr->Next())
			printf("alloc: %u\n", pAllocItr->Value()->m_size);
#endif
		allocator2.Reset();
	}
	time2 += timeGetTime();
#endif

	uint32_t time3 = 0;
#if 1
	MemAllocatorFixed<1000, 4> allocator3;
	time3 -= timeGetTime();
	for (uint32_t j = 0; j < 1000; ++j)
	{
		void * pAllocs[1000];
		for (uint32_t i = 0; i < 1000; ++i)
		{
			void * p = allocator3.Alloc(16);
			q += reinterpret_cast<uint32_t>(p);
			pAllocs[i] = p;
		}
#if REPORTING == 1
		for (IMemAllocationIterator * pAllocItr = allocator3.GetAllocationIterator(); pAllocItr->HasValue(); pAllocItr->Next())
			printf("alloc: %u\n", pAllocItr->Value()->m_size);
#endif
		for (uint32_t i = 0; i < 1000; ++i)
			allocator3.Free(pAllocs[i]);
	}
	time3 += timeGetTime();
#endif

	uint32_t time4 = 0;
#if 1
	MemAllocatorStack allocator4(1024 * 1024 * 64, 1000);
	time4 -= timeGetTime();
	for (uint32_t i = 0; i < 1000000; ++i)
	{
		void * pAllocs[1000];
		for (uint32_t j = 0; j < 1000; ++j)
		{
			pAllocs[j] = allocator4.Alloc(16);
		}
#if REPORTING == 1
		for (IMemAllocationIterator * pAllocItr = allocator4.GetAllocationIterator(); pAllocItr->HasValue(); pAllocItr->Next())
			printf("alloc: %u\n", pAllocItr->Value()->m_size);
#endif
		for (uint32_t j = 1000, k = 0; j != 0; --j, ++k)
			allocator4.Free(pAllocs[k]);
	}
	time4 += timeGetTime();
#endif

	printf("t1: %gus, t2: %gus (q=%u), t3: %gus, t4: %gus\n",
		time1 * 10.0f,
		time2 / 1000.0f,
		q,
		time3 / 1.0f,
		time4 / 1000.0f);
#endif
}

static void TestIntersection()
{
	int sx = 640;
	int sy = 480;

	SDL_Init(SDL_INIT_EVERYTHING);

	// create window

	SDL_Surface * pSurface = SDL_SetVideoMode(sx, sy, 32, 0);

	SDL_SysWMinfo wminfo;
	SDL_VERSION(&wminfo.version);
	SDL_GetWMInfo(&wminfo);
	HWND hWnd = wminfo.window;

	SDL_ShowCursor(1);

	// create device & swap chain

	RenderDevice device;
	device.Initialize(sx, sy, hWnd, false);
	ID3D11Device * pDevice = device.GetDevice();
	ID3D11DeviceContext * pDeviceCtx = device.GetDeviceCtx();
	StateManager deviceMgr(pDevice);

	// scene

	Scene scene;

	// depth buffer

	DepthStencil depthStencil(pDevice, sx, sy, DXGI_FORMAT_R16_TYPELESS, DXGI_FORMAT_D16_UNORM, DXGI_FORMAT_R16_UNORM);

	ID3D11RenderTargetView * pRTV = device.GetBackBufferView();

	VertexShader vs(pDevice, "test/shaders/deferred.hlsl", "vs_passthrough", 0);
	PixelShader ps(pDevice, "test/shaders/deferred.hlsl", "ps_passthrough", 0);
	ConstantBuffer cb(pDevice, sizeof(XMMATRIX));
	
	double t = 0.0;
	double dt = 1.0 / 60.0;

	{
		SimdVec p(1.0f, 2.0f, 3.0f, 1.0f);
		SimdVec n = SimdVec(1.0f, 1.0f, 0.0f).UnitVec3();
		ISPlane e(n, n.Dot3(p));
		SimdVec d = e.Dot4(p);
		printf("d: %g\n", d.X());
	}

	ISAABB boxes[4];
	boxes[0].Set(SimdVec(-1.0f, 0.0f, 0.0f), SimdVec(0.01f, 1.0f, 0.0f));
	boxes[1].Set(SimdVec(+1.0f, 0.0f, 0.0f), SimdVec(0.01f, 1.0f, 0.0f));
	boxes[2].Set(SimdVec(0.0f, -1.0f, 0.0f), SimdVec(1.0f, 0.01f, 0.0f));
	boxes[3].Set(SimdVec(0.0f, +1.0f, 0.0f), SimdVec(1.0f, 0.01f, 0.0f));
	ISSphere sphere(SimdVec(0.0f, 0.0f, 0.0f), 0.1f);
	ISOBB box(SimdVec(-0.5f, 0.0f, 0.0f), SimdVec(1.0f, 0.0f, 0.0f), SimdVec(0.0f, 1.0f, 0.0f), SimdVec(0.0f, 0.0f, 1.0f));
	ISConvex convex;
	SimdVec convexScale(0.2f);
	SimdVec convexPoints[8] =
	{
		SimdVec(-1.0f, -1.0f, -0.5f).Mul(convexScale),
		SimdVec(+1.0f, -1.0f, -0.5f).Mul(convexScale),
		SimdVec(+1.0f, +1.0f, -0.5f).Mul(convexScale),
		SimdVec(-1.0f, +1.0f, -0.5f).Mul(convexScale),
		SimdVec(-1.0f, -1.0f, +0.5f).Mul(convexScale),
		SimdVec(+1.0f, -1.0f, +0.5f).Mul(convexScale),
		SimdVec(+1.0f, +1.0f, +0.5f).Mul(convexScale),
		SimdVec(-1.0f, +1.0f, +0.5f).Mul(convexScale)
	};
	SimdMat4x4 convexMat;
	convexMat.MakeRotationZ(1.0f);
	for (uint32_t i = 0; i < 8; ++i)
		convexPoints[i] = convexMat.Mul3x4(convexPoints[i]);
	convex.SetFromFrustumPoints(convexPoints, 8);
	convex.Finalize();

	ISAABB userBox;

	float mouseX = 0.0f;
	float mouseY = 0.0f;

	bool stop = false;

	while (stop == false)
	{
		SDL_Event e;
		
		while (SDL_PollEvent(&e))
		{
			if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
				stop = true;
			if (e.type == SDL_MOUSEMOTION)
			{
				mouseX = (e.motion.x / (float)sx) * 2.0f - 1.0f;
				mouseY = (e.motion.y / (float)sy) * 2.0f - 1.0f;
			}
		}

		//

		userBox.Set(SimdVec(mouseX, mouseY, 0.0f), SimdVec(0.1f, 0.1f, 0.1f));

		//

		scene.Begin();

		//

		SetRenderTargets(pDeviceCtx, pRTV, 0, 0, 0, 1, depthStencil.GetDepthStencilView());

		//

		Vec3 color(
			static_cast<float>(sin(t)),
			static_cast<float>(cos(t)),
			static_cast<float>(sin(t)) + static_cast<float>(cos(t)));

		t += dt;

		ClearRenderTarget(pDeviceCtx, pRTV, color[0], color[1], color[2], 0.0f);
		pDeviceCtx->ClearDepthStencilView(depthStencil.GetDepthStencilView(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0x00);

		pDeviceCtx->VSSetShader(vs.GetShader(), 0, 0);
		pDeviceCtx->PSSetShader(ps.GetShader(), 0, 0);
		XMMATRIX mat = XMMatrixOrthographicOffCenterLH(-1.0f, +1.0f, +1.0f, -1.0f, -10.0f, +10.0f);
		mat = XMMatrixTranspose(mat);
		cb.Update(pDeviceCtx, &mat);
		pDeviceCtx->VSSetConstantBuffers(0, 1, cb.GetConstantBufferPP());

		//deviceMgr.SetDepthTest(D3D11_COMPARISON_ALWAYS, false);
		//deviceMgr.MakeCurrent(pDeviceCtx);
		//RenderFullscreenQuad(pDevice, pDeviceCtx, vs, XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f));

		for (uint32_t j = 0; j < 4; ++j)
		{
			if (!Intersects(userBox, boxes[j]))
			{
				RenderCube(
					pDevice,
					pDeviceCtx,
					&scene,
					vs,
					boxes[j].mCenter,
					boxes[j].mExt);
			}
		}

		SimdVec normal;

		if (!CSIntersect(userBox, sphere, normal))
		{
			RenderCircle(
				pDevice,
				pDeviceCtx,
				&scene,
				vs,
				sphere.mCenter,
				sphere.mRadius);
		}
		else
		{
			RenderCircle(
				pDevice,
				pDeviceCtx,
				&scene,
				vs,
				sphere.mCenter,
				sphere.mRadius);

			RenderLine(
				pDevice,
				pDeviceCtx,
				&scene,
				vs,
				sphere.mCenter,
				sphere.mCenter.Add(normal.Mul(SimdVec(0.2f))));
		}

/*		if (!Intersects(userBox, box))
		{
			// todo: render OBB
		}*/

		if (!Intersects(convex, userBox))
		{
			for (uint32_t j = 0; j < convex.mNumPoints; ++j)
			{
				for (uint32_t k = j + 1; k < convex.mNumPoints; ++k)
				{
					SimdVec p1 = convex.mPoints[j];
					SimdVec p2 = convex.mPoints[k];

					RenderLine(
						pDevice,
						pDeviceCtx,
						&scene,
						vs,
						p1,
						p2);
				}
			}
		}

		RenderCube(
			pDevice,
			pDeviceCtx,
			&scene,
			vs,
			userBox.mCenter,
			userBox.mExt);

		device.GetSwapChain()->Present(0, 0);

		scene.End();
	}

	return;

	TimingBlock()
	{
		uint32_t count = 0;
		for (uint32_t i = 0; i < 1000000; ++i)
		{
			ISAABB v1(SimdVec(10.0f, 20.0f, 30.0f), SimdVec(10.0f, 10.0f, 10.0f));
			ISAABB v2(SimdVec(20.0f, 30.0f, 40.0f), SimdVec(10.0f, 10.0f, 10.0f));
			bool is = Intersects(v1, v2);
			count += is ? 1 : 0;
		}
		printf("intersection count: %d\n", count);
	}

	TimingBlock()
	{
		uint32_t count = 0;
		for (uint32_t i = 0; i < 1000000; ++i)
		{
			ISAABB v1(SimdVec(10.0f, 20.0f, 30.0f), SimdVec(10.0f, 10.0f, 10.0f));
			ISSphere v2(SimdVec(20.0f, 30.0f, 40.0f), 20.0f);
			bool is = Intersects(v1, v2);
			count += is ? 1 : 0;
		}
		printf("intersection count: %d\n", count);
	}
}

static void CALLBACK TPCall(PTP_CALLBACK_INSTANCE pInstance, PVOID pContext, PTP_WORK pWork)
{
	printf("TPCall\n");
}

static void TestVirtualAlloc()
{
	{
		SYSTEM_INFO systemInfo;
		GetSystemInfo(&systemInfo);

		//void * pStart = 0;
		void * pStart = reinterpret_cast<void *>(0x60000000);
		//DWORD type = PAGE_NOACCESS;
		//DWORD type = PAGE_EXECUTE_READWRITE;
		//DWORD type = PAGE_EXECUTE;
		DWORD type = PAGE_READWRITE;
		DWORD size = 0x10000 * 64;

		char * pMem = 0;

		pMem = (char*)VirtualAlloc(pStart, size, MEM_RESERVE, type);
		VirtualFree(pMem, 0, MEM_RELEASE);

		pMem = (char*)VirtualAlloc(pStart, size, MEM_RESERVE | MEM_COMMIT, type);
		VirtualLock(pMem, size);
		for (DWORD i = 0; i < size; ++i)
			pMem[i] = 0;
		VirtualUnlock(pMem, size);

		pMem = (char*)VirtualAlloc(pStart, size, MEM_RESET, type);
		for (DWORD i = 0; i < size; ++i)
			pMem[i] = 0;

		VirtualFree(pMem, size, MEM_DECOMMIT);
		pMem = (char*)VirtualAlloc(pStart, size, MEM_COMMIT, type);
		for (DWORD i = 0; i < size; ++i)
			pMem[i] = 0;

		VirtualFree(pMem, size, MEM_RELEASE);
	}

	/*
	VirtualAllocEx; // allows allocation in address space of another process
	VirtualAllocExNuma; // allocated memory from so called NUMA nodes. Some kind of distributed computing?
	VirtualFreeEx; // allows freeing memory from another process
	VirtualLock; // ensure physical page allocation
	VirtualProtect; // change protection settings
	VirtualProtectEx;
	VirtualQuery; // get memory info
	VirtualQueryEx;
	VirtualUnlock;
	*/

	SetProcessWorkingSetSize(GetCurrentProcess(), 0x10000 * 8, 0x10000 * 32);

	DWORD heapSize = 0x10000;
	HANDLE heap = HeapCreate(0, heapSize, 0);

	char * pHeapMem = (char*)HeapAlloc(heap, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY, heapSize);
	for (DWORD i = 0; i < heapSize; ++i)
		pHeapMem[i] = 0;
	HeapFree(heap, HEAP_NO_SERIALIZE, pHeapMem);

	HeapDestroy(heap);

	const DWORD heapListSize = 10;
	HANDLE heapList[heapListSize];

	DWORD heapCount = GetProcessHeaps(heapListSize, heapList);

	for (DWORD i = 0; i < min(heapCount, heapListSize); ++i)
	{
		PROCESS_HEAP_ENTRY entry;
		ZeroMemory(&entry, sizeof(entry));

		while (HeapWalk(heapList[i], &entry))
		{
			printf("heap allocation: PTR=0x%08x SIZE=%06u OVERHEAD=%06d FLAGS=0x%02x\n", entry.lpData, entry.cbData, entry.cbOverhead,entry.wFlags);
		}
	}

	TP_POOL * pTP = CreateThreadpool(0);
	SetThreadpoolThreadMaximum(pTP, 16);

	for (DWORD i = 0; i < 128; ++i)
	{
		TP_WORK * pWork = CreateThreadpoolWork(TPCall, 0, 0);
		SubmitThreadpoolWork(pWork);
		CloseThreadpoolWork(pWork);
	}

	CloseThreadpool(pTP);

	SetProcessPriorityBoost(GetCurrentProcess(), FALSE);
	SetThreadPriorityBoost(GetCurrentThread(), FALSE);
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);

	const float inf = std::numeric_limits<float>::infinity();
}
