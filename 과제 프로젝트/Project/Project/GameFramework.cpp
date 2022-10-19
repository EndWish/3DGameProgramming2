#include "stdafx.h"
#include "GameFramework.h"

unique_ptr<GameFramework> GameFramework::spInstance;

void GameFramework::Create(HINSTANCE _hInstance, HWND _hMainWnd) {
	
	if (!spInstance) {		// 프레임 워크 인스턴스가 생성된적이 없을 경우 
		spInstance = make_unique<GameFramework>();
		GameFramework& gameFramework = *spInstance;

		gameFramework.instanceHandle = _hInstance;
		gameFramework.windowHandle = _hMainWnd;

		gameFramework.CreateDirect3DDevice();    // 가장먼저 디바이스를 생성해야 명령 대기열이나 서술자 힙 등을 생성할 수 있다.
		gameFramework.CreateCommandQueueList();
		gameFramework.CreateRtvAndDsvDescriptorHeaps();
		gameFramework.CreateSwapChain();    // DxgiFactory, CommandQueue, RtvDescriptorHeap 이 미리 만들어져 있어야 한다.
		gameFramework.CreateRenderTargetViews();
		gameFramework.CreateDepthStencilView();
		gameFramework.CreateGraphicsRootSignature();

		gameFramework.pCommandList->Reset(gameFramework.pCommandAllocator.Get(), NULL);
		
		::gnCbvSrvDescriptorIncrementSize = gameFramework.pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		::gnRtvDescriptorIncrementSize = gameFramework.pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		::gnDsvDescriptorIncrementSize = gameFramework.pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		// 최초씬 생성
		shared_ptr<Scene> startScene = make_shared<PlayScene>(1);
		gameFramework.PushScene(startScene);

		// 쉐이더 생성
		Shader::MakeBasicShader(gameFramework.pDevice, gameFramework.pRootSignature);
		Shader::MakeTerrainShader(gameFramework.pDevice, gameFramework.pRootSignature);
		Shader::MakeBillBoardShader(gameFramework.pDevice, gameFramework.pRootSignature);
		Shader::MakeHitBoxShader(gameFramework.pDevice, gameFramework.pRootSignature);
		

		// 히트박스용 메쉬 생성
		HitBoxMesh::MakeHitBoxMesh(gameFramework.pDevice, gameFramework.pCommandList);

		gameFramework.pCommandList->Close();
		vector<ComPtr<ID3D12CommandList>> pCommandLists = { gameFramework.pCommandList.Get() };
		gameFramework.pCommandQueue->ExecuteCommandLists(1, pCommandLists.data()->GetAddressOf());

		gameFramework.WaitForGpuComplete();

		//gameFramework.m_gameTimer.Reset();    // 타이머 리셋
	}

}

void GameFramework::Destroy() {

	if (spInstance) {
		GameFramework& gameFramework = *spInstance;

		CloseHandle(gameFramework.fenceEvent);
	}

	// HitBoxMesh제거
	
	// GameObjectManger 릴리즈
	GameObjectManager::ReleaseAllObject();
}

GameFramework& GameFramework::Instance() {
	return *spInstance;
}

const ComPtr<ID3D12Device>& GameFramework::GetDevice() {
	return spInstance->pDevice;
}
const ComPtr<ID3D12GraphicsCommandList>& GameFramework::GetCommandList() {
	return spInstance->pCommandList;
}
const ComPtr<ID3D12CommandQueue>& GameFramework::GetCommandQueue() {
	return spInstance->pCommandQueue;
}
const ComPtr<ID3D12CommandAllocator>& GameFramework::GetCommandAllocator() {
	return spInstance->pCommandAllocator;
}

GameFramework::GameFramework() {
	instanceHandle = NULL;
	windowHandle = NULL;

	// MSAA 다중 샘플링
	msaa4xEnable = false;
	msaa4xLevel = 0;

	drawHitBox = true;
	clientHeight = 1920;
	clientWidth = 1080;
	dsvDescriptorIncrementSize = 0;
	fenceValues.fill(0);
	rtvDescriptorIncrementSize = 0;

	// 현재 스왑체인의 후면 버퍼의 인덱스
	swapChainBufferCurrentIndex = 0;
	fenceEvent = NULL;
	//PushScene(make_shared<Scene>());
}
GameFramework::~GameFramework() {

}

void GameFramework::CreateDirect3DDevice() {
	HRESULT hResult;

	UINT nDxgiFactoryFlag = 0;

	// pDxgiFactory에 팩토리 생성
	hResult = CreateDXGIFactory2(nDxgiFactoryFlag, __uuidof(IDXGIFactory4), (void**)pDxgiFactory.GetAddressOf());

	ComPtr<IDXGIAdapter1> pAdapter = NULL;

	for (int i = 0; DXGI_ERROR_NOT_FOUND != pDxgiFactory->EnumAdapters1(i, pAdapter.GetAddressOf()); ++i) {
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc;
		pAdapter->GetDesc1(&dxgiAdapterDesc);		// 어댑터의 정보를 가져온다.

		if (dxgiAdapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { // 어댑터가 소프트웨어이면 건너뜀.
			continue;
		}

		if (SUCCEEDED(D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), (void**)pDevice.GetAddressOf()))) { // 적절한 어댑터 찾을 경우
			break;
		}
	}

	if (!pAdapter) {// 적절한 어댑터가 없을 경우 
		pDxgiFactory->EnumWarpAdapter(__uuidof(IDXGIFactory4), (void**)pAdapter.GetAddressOf());
		hResult = D3D12CreateDevice(pAdapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), (void**)pDevice.GetAddressOf());
	}

	// 다중 샘플링 정보
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msaaQualityLevel;
	msaaQualityLevel.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	msaaQualityLevel.SampleCount = 4;
	msaaQualityLevel.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msaaQualityLevel.NumQualityLevels = 0;
	hResult = pDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msaaQualityLevel, sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
	msaa4xLevel = msaaQualityLevel.NumQualityLevels;

	// 1 초과일시 msaa 가능
	msaa4xEnable = (msaa4xLevel > 1) ? true : false;

	// 펜스 
	hResult = pDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)pFence.GetAddressOf());

	for (UINT64 fenceValue : fenceValues) {
		fenceValue = 0;
	}

	fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
}
void GameFramework::CreateCommandQueueList() {
	HRESULT hResult;

	// 명령 대기열 생성
	D3D12_COMMAND_QUEUE_DESC commandQueueDesc;
	ZeroMemory(&commandQueueDesc, sizeof(D3D12_COMMAND_QUEUE_DESC));
	commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	hResult = pDevice->CreateCommandQueue(&commandQueueDesc, __uuidof(ID3D12CommandQueue), (void**)pCommandQueue.GetAddressOf());

	// 명령 할당자 생성
	hResult = pDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)pCommandAllocator.GetAddressOf());

	// 명령 리스트 생성
	hResult = pDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, pCommandAllocator.Get(), NULL, __uuidof(ID3D12CommandList), (void**)pCommandList.GetAddressOf());
	
	hResult = pCommandList->Close();

}
void GameFramework::CreateRtvAndDsvDescriptorHeaps() {
	// 렌더타겟 서술자 힙 생성
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
	ZeroMemory(&descriptorHeapDesc, sizeof(D3D12_DESCRIPTOR_HEAP_DESC));
	descriptorHeapDesc.NumDescriptors = nSwapChainBuffer;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	descriptorHeapDesc.NodeMask = 0;
	HRESULT hResult = pDevice->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&pRtvDescriptorHeap);
	rtvDescriptorIncrementSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// 깊이 스텐실 서술자 힙 생성
	descriptorHeapDesc.NumDescriptors = 1;
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	hResult = pDevice->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&pDsvDescriptorHeap);
	dsvDescriptorIncrementSize = pDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

}
void GameFramework::CreateSwapChain()
{
	RECT rcClient;
	GetClientRect(windowHandle, &rcClient);    // 클라이언트 크기를 rcClient에 저장
	clientWidth = rcClient.right - rcClient.left;
	clientHeight = rcClient.bottom - rcClient.top;

	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	::ZeroMemory(&dxgiSwapChainDesc, sizeof(dxgiSwapChainDesc));
	dxgiSwapChainDesc.BufferCount = nSwapChainBuffer;    // 스왑체인(렌더 타켓) 버퍼의 개수
	dxgiSwapChainDesc.BufferDesc.Width = clientWidth;    // 윈도우 클라이언트 영역의 가로 크기 
	dxgiSwapChainDesc.BufferDesc.Height = clientHeight;    // 윈도우 클라이언트 영역의 세로 크기 
	dxgiSwapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;    // 픽셀을 RGBA에 8bit 형태로 그리기
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Numerator = 60;    // RefreshRate : 화면 갱신률(프레임) //분자
	dxgiSwapChainDesc.BufferDesc.RefreshRate.Denominator = 1;    // 분모
	dxgiSwapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;    // 출력용 버퍼로 쓰겠다고 지정하는것.        읽기전용, 쉐이더의 입력으로 사용, 공유, 무순서화 접근 등의 형태가 있다.
	dxgiSwapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;    // 전면 버퍼와 후면버퍼를 교체하고 버퍼 내용을 폐기(유지 하지 않으면 속도가 빠라짐)한다.
	dxgiSwapChainDesc.OutputWindow = windowHandle;    // 출력할 윈도우
	dxgiSwapChainDesc.SampleDesc.Count = (msaa4xEnable) ? 4 : 1;
	dxgiSwapChainDesc.SampleDesc.Quality = (msaa4xEnable) ? (msaa4xLevel - 1) : 0;
	dxgiSwapChainDesc.Windowed = TRUE;    // 창모드를 쓸껀지?
	dxgiSwapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	HRESULT hResult = pDxgiFactory->CreateSwapChain(pCommandQueue.Get(), &dxgiSwapChainDesc, (IDXGISwapChain**)pDxgiSwapChain.GetAddressOf());

	swapChainBufferCurrentIndex = pDxgiSwapChain->GetCurrentBackBufferIndex();

	hResult = pDxgiFactory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER);

}
void GameFramework::CreateRenderTargetViews() {
	HRESULT hResult;
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDescriptorHandle = pRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();    // 서술자 힙을 통해 시작주소를 가져온다.
	for (int i = 0; i < nSwapChainBuffer; ++i) {
		hResult = pDxgiSwapChain->GetBuffer(i, __uuidof(ID3D12Resource), (void**)&pRenderTargetBuffers[i]);    // 렌더타겟 버퍼를 생성한다.
		pDevice->CreateRenderTargetView(pRenderTargetBuffers[i].Get(), NULL, rtvCPUDescriptorHandle);    // 렌더타겟 뷰를 서술자 힙에 생성(적재)
		rtvCPUDescriptorHandle.ptr += rtvDescriptorIncrementSize;    // 다음번 주소로 이동
	}
}
void GameFramework::CreateDepthStencilView() {
	HRESULT hResult;
	// 깊이-스텐실 버퍼(리소스)를 만들기 위한 정보들 입력
	D3D12_RESOURCE_DESC resourceDesc;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = clientWidth;
	resourceDesc.Height = clientHeight;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	resourceDesc.SampleDesc.Count = (msaa4xEnable) ? 4 : 1;
	resourceDesc.SampleDesc.Quality = (msaa4xEnable) ? (msaa4xLevel - 1) : 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

	D3D12_HEAP_PROPERTIES heapProperties;
	::ZeroMemory(&heapProperties, sizeof(D3D12_HEAP_PROPERTIES));
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.CreationNodeMask = 1;
	heapProperties.VisibleNodeMask = 1;
	D3D12_CLEAR_VALUE clearValue;
	clearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	clearValue.DepthStencil.Depth = 1.0f;
	clearValue.DepthStencil.Stencil = 0;

	// 깊이-스텐실 버퍼(리소스) 생성
	hResult = pDevice->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue, __uuidof(ID3D12Resource), (void**)&pDepthStencilBuffer);

	D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc;
	::ZeroMemory(&depthStencilViewDesc, sizeof(D3D12_DEPTH_STENCIL_VIEW_DESC));
	depthStencilViewDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilViewDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	depthStencilViewDesc.Flags = D3D12_DSV_FLAG_NONE;

	D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUDescriptorHandle = pDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	pDevice->CreateDepthStencilView(pDepthStencilBuffer.Get(), &depthStencilViewDesc, dsvCPUDescriptorHandle);    // 깊이-스텐실 뷰를 서술자 힙에 생성(적제) (뷰==서술자?)
}
void GameFramework::CreateGraphicsRootSignature() {

	// 서술자 테이블을 만들기 위한 Ranges
	D3D12_DESCRIPTOR_RANGE pd3dDescriptorRanges[7];

	pd3dDescriptorRanges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[0].NumDescriptors = 1;
	pd3dDescriptorRanges[0].BaseShaderRegister = 6; //t6: gtxtAlbedoTexture
	pd3dDescriptorRanges[0].RegisterSpace = 0;
	pd3dDescriptorRanges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[1].NumDescriptors = 1;
	pd3dDescriptorRanges[1].BaseShaderRegister = 7; //t7: gtxtSpecularTexture
	pd3dDescriptorRanges[1].RegisterSpace = 0;
	pd3dDescriptorRanges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[2].NumDescriptors = 1;
	pd3dDescriptorRanges[2].BaseShaderRegister = 8; //t8: gtxtNormalTexture
	pd3dDescriptorRanges[2].RegisterSpace = 0;
	pd3dDescriptorRanges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[3].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[3].NumDescriptors = 1;
	pd3dDescriptorRanges[3].BaseShaderRegister = 9; //t9: gtxtMetallicTexture
	pd3dDescriptorRanges[3].RegisterSpace = 0;
	pd3dDescriptorRanges[3].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[4].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[4].NumDescriptors = 1;
	pd3dDescriptorRanges[4].BaseShaderRegister = 10; //t10: gtxtEmissionTexture
	pd3dDescriptorRanges[4].RegisterSpace = 0;
	pd3dDescriptorRanges[4].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[5].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[5].NumDescriptors = 1;
	pd3dDescriptorRanges[5].BaseShaderRegister = 11; //t11: gtxtDetailAlbedoTexture
	pd3dDescriptorRanges[5].RegisterSpace = 0;
	pd3dDescriptorRanges[5].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	pd3dDescriptorRanges[6].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	pd3dDescriptorRanges[6].NumDescriptors = 1;
	pd3dDescriptorRanges[6].BaseShaderRegister = 12; //t12: gtxtDetailNormalTexture
	pd3dDescriptorRanges[6].RegisterSpace = 0;
	pd3dDescriptorRanges[6].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	// 루트 파라매터
	HRESULT hResult;
	D3D12_ROOT_PARAMETER pRootParameters[11];

	// 변수 부분
	pRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pRootParameters[0].Descriptor.ShaderRegister = 1; //Camera //shader.hlsl의 레지스터 번호 (예시 register(b1) )
	pRootParameters[0].Descriptor.RegisterSpace = 0;
	pRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	pRootParameters[1].Constants.Num32BitValues = 16;
	pRootParameters[1].Constants.ShaderRegister = 2; //GameObject
	pRootParameters[1].Constants.RegisterSpace = 0;
	pRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pRootParameters[2].Descriptor.ShaderRegister = 3; // Lights
	pRootParameters[2].Descriptor.RegisterSpace = 0;
	pRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	pRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	pRootParameters[3].Descriptor.ShaderRegister = 4; // Material
	pRootParameters[3].Descriptor.RegisterSpace = 0;
	pRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	// 텍스쳐 부분
	pRootParameters[4].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pRootParameters[4].DescriptorTable.NumDescriptorRanges = 1;		//t1: gtxtAlbedoTexture
	pRootParameters[4].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[0]);
	pRootParameters[4].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pRootParameters[5].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pRootParameters[5].DescriptorTable.NumDescriptorRanges = 1;		//t2: gtxtSpecularTexture
	pRootParameters[5].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[1]);
	pRootParameters[5].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pRootParameters[6].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pRootParameters[6].DescriptorTable.NumDescriptorRanges = 1;		//t3: gtxtNormalTexture
	pRootParameters[6].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[2]);
	pRootParameters[6].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pRootParameters[7].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pRootParameters[7].DescriptorTable.NumDescriptorRanges = 1;		//t4: gtxtMetallicTexture
	pRootParameters[7].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[3]);
	pRootParameters[7].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pRootParameters[8].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pRootParameters[8].DescriptorTable.NumDescriptorRanges = 1;		//t5: gtxtEmissionTexture
	pRootParameters[8].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[4]);
	pRootParameters[8].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pRootParameters[9].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pRootParameters[9].DescriptorTable.NumDescriptorRanges = 1;		//t6: gtxtDetailAlbedoTexture
	pRootParameters[9].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[5]);
	pRootParameters[9].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pRootParameters[10].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	pRootParameters[10].DescriptorTable.NumDescriptorRanges = 1;		//t7: gtxtDetailNormalTexture
	pRootParameters[10].DescriptorTable.pDescriptorRanges = &(pd3dDescriptorRanges[6]);
	pRootParameters[10].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 샘플러 생성
	D3D12_STATIC_SAMPLER_DESC pd3dSamplerDescs[2];

	pd3dSamplerDescs[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	pd3dSamplerDescs[0].MipLODBias = 0;
	pd3dSamplerDescs[0].MaxAnisotropy = 1;
	pd3dSamplerDescs[0].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[0].MinLOD = 0;
	pd3dSamplerDescs[0].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[0].ShaderRegister = 0;
	pd3dSamplerDescs[0].RegisterSpace = 0;
	pd3dSamplerDescs[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	pd3dSamplerDescs[1].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	pd3dSamplerDescs[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	pd3dSamplerDescs[1].MipLODBias = 0;
	pd3dSamplerDescs[1].MaxAnisotropy = 1;
	pd3dSamplerDescs[1].ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
	pd3dSamplerDescs[1].MinLOD = 0;
	pd3dSamplerDescs[1].MaxLOD = D3D12_FLOAT32_MAX;
	pd3dSamplerDescs[1].ShaderRegister = 1;
	pd3dSamplerDescs[1].RegisterSpace = 0;
	pd3dSamplerDescs[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// 루트 시그니쳐 생성
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT; // | D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS | D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS;
	D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc;
	::ZeroMemory(&rootSignatureDesc, sizeof(D3D12_ROOT_SIGNATURE_DESC));
	rootSignatureDesc.NumParameters = _countof(pRootParameters);
	rootSignatureDesc.pParameters = pRootParameters;
	rootSignatureDesc.NumStaticSamplers = _countof(pd3dSamplerDescs);;
	rootSignatureDesc.pStaticSamplers = pd3dSamplerDescs;
	rootSignatureDesc.Flags = rootSignatureFlags;

	ComPtr<ID3DBlob> pSignatureBlob = NULL;
	ComPtr<ID3DBlob> pErrorBlob = NULL;
	hResult = D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &pSignatureBlob, &pErrorBlob);
	hResult = pDevice->CreateRootSignature(0, pSignatureBlob->GetBufferPointer(), pSignatureBlob->GetBufferSize(), __uuidof(ID3D12RootSignature), (void**)&pRootSignature);
}

// get, set 함수
pair<int, int> GameFramework::GetClientSize() {
	return { clientWidth , clientHeight };
}
bool GameFramework::GetDrawHitBox() const {
	return drawHitBox;
}
const shared_ptr<Scene>& GameFramework::GetCurrentScene() const {
	//if (!pScenes.empty()) {
		return pScenes.top();
	//}
}
bool GameFramework::GetLeftMouseDrag() const {
	return leftMouseDrag;
}
POINT GameFramework::GetClickedLeftMousePos() const {
	return clickedLeftMousePos;
}

void GameFramework::FrameAdvance() {

	gameTimer.Tick(.0f);
	ProcessInput();
	// 씬 진행(애니메이트). 스택의 맨 위 원소에 대해 진행
	if (!pScenes.empty()) {
		pScenes.top()->AnimateObjects(gameTimer.GetTimeElapsed());
		pScenes.top()->CheckCollision();
		// 씬의 오브젝트 충돌처리 [수정]
	}


	// 명령 할당자와 명령 리스트를 리셋한다.
	HRESULT hResult = pCommandAllocator->Reset();
	hResult = pCommandList->Reset(pCommandAllocator.Get(), NULL);
	// 현재 렌더 타겟에 대한 Present가 끝나기를 기다림.  (PRESENT = 프리젠트 상태, RENDER_TARGET = 렌더 타겟 상태
	D3D12_RESOURCE_BARRIER resourceBarrier;
	ZeroMemory(&resourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	resourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	resourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	resourceBarrier.Transition.pResource = pRenderTargetBuffers[swapChainBufferCurrentIndex].Get();
	resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

	pCommandList->ResourceBarrier(1, &resourceBarrier);
	
	// 현재 렌더 타겟 뷰의 CPU 주소 계산
	D3D12_CPU_DESCRIPTOR_HANDLE rtvCPUDescriptorHandle = pRtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	rtvCPUDescriptorHandle.ptr += (rtvDescriptorIncrementSize * swapChainBufferCurrentIndex);

	//깊이-스텐실 서술자의 CPU 주소를 계산한다. 
	D3D12_CPU_DESCRIPTOR_HANDLE dsvCPUDescriptorHandle = pDsvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();

	//렌더 타겟 뷰(서술자)와 깊이-스텐실 뷰(서술자)를 출력-병합 단계(OM)에 연결한다. 
	pCommandList->OMSetRenderTargets(1, &rtvCPUDescriptorHandle, TRUE, &dsvCPUDescriptorHandle);

	float pClearColor[4] = {0.0f, 0.1f, 0.3f, 1.0f};
	pCommandList->ClearRenderTargetView(rtvCPUDescriptorHandle, pClearColor, 0, NULL);

	//원하는 값으로 깊이-스텐실(뷰)을 지운다. 
	pCommandList->ClearDepthStencilView(dsvCPUDescriptorHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, NULL);

	//그래픽 루트 시그너쳐를 파이프라인에 연결(설정)한다.
	pCommandList->SetGraphicsRootSignature(pRootSignature.Get());

	//씬 렌더링  (Scene에서 카메라, 플레이어를 관리한다.)
	if (!pScenes.empty()) {
		pScenes.top()->Render(pCommandList);
	}

	// 현재 렌더 타겟에 대한 렌더링이 끝나기를 기다린다.
	resourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	resourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	resourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pCommandList->ResourceBarrier(1, &resourceBarrier);
	//명령 리스트를 닫힌 상태로 만든다. 
	hResult = pCommandList->Close();
	//명령 리스트를 명령 큐에 추가하여 실행한다. 
	vector<ComPtr<ID3D12CommandList>> pCommandLists = { pCommandList.Get() };
	pCommandQueue->ExecuteCommandLists(1, pCommandLists.data()->GetAddressOf());


	//GPU가 모든 명령 리스트를 실행할 때 까지 기다린다. 
	WaitForGpuComplete();

	//	스왑체인을 프리젠트한다.
	DXGI_PRESENT_PARAMETERS dxgiPresentParameters;
	dxgiPresentParameters.DirtyRectsCount = 0;
	dxgiPresentParameters.pDirtyRects = NULL;
	dxgiPresentParameters.pScrollRect = NULL;
	dxgiPresentParameters.pScrollOffset = NULL;
	pDxgiSwapChain->Present1(1, 0, &dxgiPresentParameters);

	// 다음 프레임으로 이동, (다음 버퍼로 이동)
	MoveToNextFrame();

	// FPS 표시
	wstring titleString = L"FPS : " + to_wstring(gameTimer.GetFPS());
	SetWindowText(windowHandle, (LPCWSTR)titleString.c_str());
}
void GameFramework::WaitForGpuComplete() {
	UINT64 fenceValue = ++fenceValues[swapChainBufferCurrentIndex]; // 현재 버퍼의 펜스값을 증가
	HRESULT hResult = pCommandQueue->Signal(pFence.Get(), fenceValue);
	if (pFence->GetCompletedValue() < fenceValue) {
		hResult = pFence->SetEventOnCompletion(fenceValue, fenceEvent);	
		WaitForSingleObject(fenceEvent, INFINITE);					// GPU가 처리를 끝낼때 까지 기다림
	}
}
void GameFramework::MoveToNextFrame() {
	swapChainBufferCurrentIndex = pDxgiSwapChain->GetCurrentBackBufferIndex(); // 다음 후면 버퍼로 변경
	WaitForGpuComplete();
}
void GameFramework::ChangeSwapChainState() {
	
	WaitForGpuComplete();
	BOOL fullScreenState;

	// 현재 모드 반전
	pDxgiSwapChain->GetFullscreenState(&fullScreenState, NULL);
	pDxgiSwapChain->SetFullscreenState(~fullScreenState, NULL);


	DXGI_MODE_DESC dxgiTargetParameters;
	dxgiTargetParameters.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	dxgiTargetParameters.Width = clientWidth;
	dxgiTargetParameters.Height = clientHeight;
	dxgiTargetParameters.RefreshRate.Numerator = 60;
	dxgiTargetParameters.RefreshRate.Denominator = 1;
	dxgiTargetParameters.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	dxgiTargetParameters.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	pDxgiSwapChain->ResizeTarget(&dxgiTargetParameters);
	for (int i = 0; i < nSwapChainBuffer; i++) {
		if (pRenderTargetBuffers[i]) {
			pRenderTargetBuffers[i].Reset();
		}
	}

	DXGI_SWAP_CHAIN_DESC dxgiSwapChainDesc;
	pDxgiSwapChain->GetDesc(&dxgiSwapChainDesc);
	pDxgiSwapChain->ResizeBuffers(nSwapChainBuffer, clientWidth,
		clientHeight, dxgiSwapChainDesc.BufferDesc.Format, dxgiSwapChainDesc.Flags);
	swapChainBufferCurrentIndex = pDxgiSwapChain->GetCurrentBackBufferIndex();
	CreateRenderTargetViews();
}


void GameFramework::ProcessInput() {

	static array<UCHAR, 256> keysBuffers;
	bool processedByScene = false;

	if (GetKeyboardState((PBYTE)keysBuffers.data())) {	// 키보드로 부터 입력데이터를 받으면
		if (keysBuffers['F'] & 0xF0) {
			ChangeSwapChainState();
		}
		// 일시정지
		if (keysBuffers['P'] & 0xF0) {
			//PushScene(make_shared<Scene>("pause"));
		}
		// 재시작
		if (keysBuffers['R'] & 0xF0) {
			//PopScene();
		}

		// 씬의 키보드입력 처리
		if (!pScenes.empty()) {
			pScenes.top()->ProcessKeyboardInput(keysBuffers, (float)gameTimer.GetTimeElapsed());
		}
	}

}
void GameFramework::OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam) {

	switch (nMessageID) {
	case WM_LBUTTONDOWN:
		::SetCapture(hWnd);
		::GetCursorPos(&clickedLeftMousePos);
		leftMouseDrag = true;
		break;
	case WM_LBUTTONUP:
		::ReleaseCapture();
		leftMouseDrag = false;
		break;
	case WM_MOUSEMOVE:
		break;
	default:
		break;
	}
}

// 씬 다루기 위한 함수
void GameFramework::PushScene(const shared_ptr<Scene>& _pScene) {
	pScenes.push(_pScene);
	pScenes.top()->Init(pDevice, pCommandList);
}
void GameFramework::PopScene() {
	if (!pScenes.empty()) {
		pScenes.pop();
	}
}
void GameFramework::ChangeScene(const shared_ptr<Scene>& _pScene) {
	PopScene();
	PushScene(_pScene);
}
void GameFramework::ClearScene() {

	while (!pScenes.empty()) {
			pScenes.pop();
	}
}

