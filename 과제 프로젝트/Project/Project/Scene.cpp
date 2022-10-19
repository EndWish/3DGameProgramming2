#include "stdafx.h"
#include "Scene.h"
#include "Timer.h"
#include "GameFramework.h"
#include "Gunship.h"

Scene::Scene() {
	
}

Scene::~Scene() {
	
}

void Scene::CheckCollision() {

}

///////////////////////////////////////////////////////////////////////////////
/// PlayScene
PlayScene::PlayScene(int _stageNum) {
	globalAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
}

void PlayScene::Init(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	GameFramework& gameFramework = GameFramework::Instance();

	GameObjectManager::LoadFromFile("Gunship", _pDevice, _pCommandList);
	//GameObjectManager::ReleaseObject("Apache");	// 미리 로드를 하지 않고 했을대 날개가 사라짐 -> CommandQueue를 진행시켜야 하지 않을까 싶다.
	
	// 카메라 설정

	// 플레이어 생성
	pPlayer = make_shared<Player>();
	pPlayer->Create();
	camera = pPlayer->GetCamera();

	// 오브젝트 생성
	VS_MaterialMappedFormat terrainMaterialColorsBuffer;
	
	terrainMaterialColorsBuffer.diffuse = XMFLOAT4(0, 0, 0, 1);
	terrainMaterialColorsBuffer.specular = XMFLOAT4(0, 0, 0, 1);
	terrainMaterialColorsBuffer.ambient = XMFLOAT4(0, 0, 0, 1);
	terrainMaterialColorsBuffer.emissive = XMFLOAT4(0, 0, 0, 1);
	
	array<string, TEXTURETYPENUM> terrainTextureFileNames{ 
		"Detail_Texture_7.dds",		//MATERIAL_ALBEDO_MAP_INDEX
		"",							//MATERIAL_SPECULAR_MAP_INDEX
		"",							//MATERIAL_NORMAL_MAP_INDEX
		"",							//MATERIAL_METALLIC_MAP_INDEX
		"",							//MATERIAL_EMISSION_MAP_INDEX
		"Detail_Texture_1.dds",		//MATERIAL_DETAIL_ALBEDO_MAP_INDEX
		""};						//MATERIAL_DETAIL_NORMAL_MAP_INDEX

	shared_ptr<TerrainMesh> terrainMesh = TerrainMesh::LoadFromFile("terrainHeightMap01.raw", XMINT2(249, 149), XMFLOAT3(2000.f, 500.f, 2000.f), 1.f, terrainMaterialColorsBuffer,
		500.f, terrainTextureFileNames, _pDevice, _pCommandList);
	pTerrainObject = make_shared<GameObject>();
	pTerrainObject->Create();
	pTerrainObject->SetMesh(terrainMesh);

	// 빌보드 
	shared_ptr<BillBoardMesh> billBoardMesh = BillBoardMesh::LoadFromFile("grassMesh", "grass1.dds", XMFLOAT2(10, 10), _pDevice, _pCommandList);
	pBillBoardObject = make_shared<GameObject>();
	pBillBoardObject->Create();
	pBillBoardObject->SetMesh(billBoardMesh);

	// 조명 버퍼 생성
	UINT ncbElementBytes = ((sizeof(LightsMappedFormat) + 255) & ~255); //256의 배수
	pLightsBuffer = ::CreateBufferResource(_pDevice, _pCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, ComPtr<ID3D12Resource>());
	pLightsBuffer->Map(0, NULL, (void**)&pMappedLights);
	
	// 태양(조명) 추가
	pSun = make_shared<Light>(nullptr);
	AddLight(pSun);

}

PlayScene::~PlayScene() {

}

void PlayScene::ProcessKeyboardInput(const array<UCHAR, 256>& _keysBuffers, float _timeElapsed) {
	GameFramework& gameFramework = GameFramework::Instance();

	// 키보드 처리
	if (_keysBuffers['E'] & 0xF0) {

	}
	if (_keysBuffers['Q'] & 0xF0) {
		
	}
	if (_keysBuffers['W'] & 0xF0) {
		XMFLOAT3 cameraLook = pPlayer->GetCamera()->GetWorldLookVector();
		pPlayer->MoveHorizontal(cameraLook, _timeElapsed);
	}
	if (_keysBuffers['S'] & 0xF0) {
		XMFLOAT3 cameraBack = pPlayer->GetCamera()->GetWorldLookVector();
		cameraBack = Vector3::ScalarProduct(cameraBack, -1);
		pPlayer->MoveHorizontal(cameraBack, _timeElapsed);
	}
	if (_keysBuffers['D'] & 0xF0) {
		XMFLOAT3 cameraRight = pPlayer->GetCamera()->GetWorldRightVector();
		pPlayer->MoveHorizontal(cameraRight, _timeElapsed);
	}
	if (_keysBuffers['A'] & 0xF0) {
		XMFLOAT3 cameraLeft = pPlayer->GetCamera()->GetWorldRightVector();
		cameraLeft = Vector3::ScalarProduct(cameraLeft, -1);
		pPlayer->MoveHorizontal(cameraLeft, _timeElapsed);
	}
	if (_keysBuffers[VK_SPACE] & 0xF0) {
		pPlayer->MoveVertical(true, _timeElapsed);
	}
	if (_keysBuffers[VK_LCONTROL] & 0xF0) {
		pPlayer->MoveVertical(false, _timeElapsed);
	}

	// 마우스 처리
	float mxDelta = 0.0f, myDelta = 0.0f;
	POINT currentMousePos;
	if (gameFramework.GetLeftMouseDrag()) {	// 왼쪽 마우스를 드래그 했을 경우
		SetCursor(NULL);
		GetCursorPos(&currentMousePos);
		POINT clickedLeftMousePos = gameFramework.GetClickedLeftMousePos();
		mxDelta = (float)(currentMousePos.x - clickedLeftMousePos.x);
		myDelta = (float)(currentMousePos.y - clickedLeftMousePos.y);
		SetCursorPos(clickedLeftMousePos.x, clickedLeftMousePos.y);

		if (mxDelta != 0.0f) {
			//XMFLOAT3 upVector = pPlayer->GetCamera()->GetLocalUpVector();
			pPlayer->GetCamera()->SynchronousRotation(XMFLOAT3(0,1,0), mxDelta / 3.f);
			pPlayer->GetCamera()->UpdateLocalTransform();
		}
		if (myDelta != 0.0f) {
			XMFLOAT3 rightVector = pPlayer->GetCamera()->GetLocalRightVector();
			pPlayer->GetCamera()->SynchronousRotation(rightVector, myDelta / 3.f);
			pPlayer->GetCamera()->UpdateLocalTransform();
		}
		pPlayer->GetCamera()->UpdateLocalTransform();
	}

	// 적용
	pPlayer->UpdateObject();
}

void PlayScene::AnimateObjects(double _timeElapsed) {
	pPlayer->Animate(_timeElapsed);
}
void PlayScene::CheckCollision() {
	
}

void PlayScene::UpdateLightShaderVariables(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	int nLight = (UINT)pLights.size();

	for (int i = 0; i < nLight; ++i) {
		
		memcpy(&pMappedLights->lights[i], pLights[i].lock().get(), sizeof(Light));
	}

	memcpy(&pMappedLights->globalAmbient, &globalAmbient, sizeof(XMFLOAT4));

	memcpy(&pMappedLights->nLight, &nLight, sizeof(int));

	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress = pLightsBuffer->GetGPUVirtualAddress();
	_pCommandList->SetGraphicsRootConstantBufferView(2, gpuVirtualAddress);

}

void PlayScene::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	// 프레임워크에서 렌더링 전에 루트시그니처를 set
	
	// 카메라 쉐이더 변수 업데이트
	camera->SetViewPortAndScissorRect(_pCommandList);
	camera->UpdateShaderVariable(_pCommandList);

	// 조명 쉐이더 변수 업데이트
	UpdateLightShaderVariables(_pCommandList);

	// 기본 쉐이더 활성화
	GameFramework& gameFramework = GameFramework::Instance();
	Shader::GetBasicShader()->PrepareRender(_pCommandList);
	pPlayer->Render(_pCommandList);	// 플레이어 드로우
	
	// 터레인 그리기
	Shader::GetTerrainShader()->PrepareRender(_pCommandList);
	pTerrainObject->Render(_pCommandList);

	// 빌보드 그리기
	Shader::GetBillBoardShader()->PrepareRender(_pCommandList);
	pBillBoardObject->Render(_pCommandList);

	// 히트박스 렌더링
	//Shader::GetShader()->PrepareRender(_pCommandList);
	//pGameObject->RenderHitBox(_pCommandList, *HitBoxMesh::GetHitBoxMesh());

}

void PlayScene::AddLight(const shared_ptr<Light>& _pLight) {
	pLights.push_back(_pLight);
}
