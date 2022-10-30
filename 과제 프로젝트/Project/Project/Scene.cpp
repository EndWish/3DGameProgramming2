#include "stdafx.h"
#include "Scene.h"
#include "Timer.h"
#include "GameFramework.h"
#include "Gunship.h"
#include "GunshipEnemy.h"

Scene::Scene() {
	
}

Scene::~Scene() {
	
}

///////////////////////////////////////////////////////////////////////////////
/// PlayScene
PlayScene::PlayScene(int _stageNum) {
	globalAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
}

void PlayScene::LoadObjects(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	// 지형 생성하기
	VS_MaterialMappedFormat terrainMaterialColorsBuffer;
	terrainMaterialColorsBuffer.diffuse = XMFLOAT4(0, 0, 0, 1);
	terrainMaterialColorsBuffer.specular = XMFLOAT4(0, 0, 0, 1);
	terrainMaterialColorsBuffer.ambient = XMFLOAT4(0, 0, 0, 1);
	terrainMaterialColorsBuffer.emissive = XMFLOAT4(0, 0, 0, 1);

	array<string, TEXTURETYPENUM> terrainTextureFileNames{
		"Terrain01AlbedoTexture.dds",		//MATERIAL_ALBEDO_MAP_INDEX
		"",							//MATERIAL_SPECULAR_MAP_INDEX
		"",							//MATERIAL_NORMAL_MAP_INDEX
		"",							//MATERIAL_METALLIC_MAP_INDEX
		"",							//MATERIAL_EMISSION_MAP_INDEX
		"Detail_Texture_7.dds",		//MATERIAL_DETAIL_ALBEDO_MAP_INDEX
		"" };						//MATERIAL_DETAIL_NORMAL_MAP_INDEX

	shared_ptr<TerrainMesh> terrainMesh = TerrainMesh::LoadFromFile("terrainHeightMap01.raw", XMINT2(249, 149), XMFLOAT3(2000.f, 500.f, 2000.f), 1.f, terrainMaterialColorsBuffer, 30.f, terrainTextureFileNames, _pDevice, _pCommandList);
	shared_ptr<GameObject> pTerrainObject = make_shared<GameObject>();
	pTerrainObject->Create();
	pTerrainObject->SetMesh(terrainMesh, OOBB_TYPE::DISABLED_FINAL);
	pTerrainObject->SetName("Terrain1");
	GameObjectManager::AddObject(pTerrainObject);
	
	// Tree 빌보드 불러와서 저장
	shared_ptr<BillBoardMesh> billBoardMesh = BillBoardMesh::LoadFromFile("TreeMesh", "Tree1.dds", XMFLOAT2(0, 45), XMFLOAT2(100, 100), _pDevice, _pCommandList);
	shared_ptr<GameObject> pTreeObject = make_shared<GameObject>();
	pTreeObject->Create();
	pTreeObject->SetMesh(billBoardMesh, OOBB_TYPE::DISABLED_FINAL);
	pTreeObject->SetName("Tree1");
	GameObjectManager::AddObject(pTreeObject);

	// 건쉽
	GameObjectManager::LoadFromFile("Gunship", _pDevice, _pCommandList);	
	GameObjectManager::LoadFromFile("GunshipMissile", _pDevice, _pCommandList);

	// 호수
	GameObjectManager::LoadFromFile("Lake", _pDevice, _pCommandList);

}

void PlayScene::Init(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	GameFramework& gameFramework = GameFramework::Instance();

	LoadObjects(_pDevice, _pCommandList);

	// 플레이어 생성
	shared_ptr<Player> pPlayer = make_shared<Player>();
	pPlayer->Create();
	camera = pPlayer->GetCamera();	// 카메라 설정
	AddObject(pPlayer, WORLD_OBJ_LAYER::PLAYER);
	wpPlayer = pPlayer;	// palyer를 (weak_ptr)wpPlayer 에 등록

	// 지형 생성하기
	shared_ptr<GameObject> pTerrainObject = GameObjectManager::GetGameObject("Terrain1");
	shared_ptr<TerrainMesh> pTerrainMesh = static_pointer_cast<TerrainMesh>(pTerrainObject->GetMesh());
	AddObject(pTerrainObject, WORLD_OBJ_LAYER::TERRAIN);

	// 나무 생성
	uniform_real_distribution<float> urd(0, 2'000.f);
	for (int i = 0; i < 1'000; ++i) {
		shared_ptr<GameObject> newTree = GameObjectManager::GetGameObject("Tree1", _pDevice, _pCommandList);
		XMFLOAT3 randomPos = XMFLOAT3(urd(rd), 0, urd(rd));
		randomPos.y = pTerrainMesh->GetHeightMapImage().GetHeight(randomPos.x, randomPos.z);
		if (randomPos.y < 200.f)
			continue;

		newTree->SetLocalPosition(randomPos);
		newTree->UpdateObject();

		AddObject(newTree, WORLD_OBJ_LAYER::BILLBOARD);
	}

	// 호수
	shared_ptr<GameObject> pLakeObject = GameObjectManager::GetGameObject("Lake");
	pLakeObject->SetLocalPosition(XMFLOAT3(1000.f, 200.f, 1000.f));
	pLakeObject->UpdateLocalTransform();
	pLakeObject->UpdateWorldTransform();
	AddObject(pLakeObject, WORLD_OBJ_LAYER::NO_COLLIDER);

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
	shared_ptr<Player> pPlayer = wpPlayer.lock();

	if (pPlayer) {
		// 키보드 처리
		if (_keysBuffers['E'] & 0xF0) {

		}
		if (_keysBuffers['Q'] & 0xF0) {
			pPlayer->TryFireMissile();
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
				pPlayer->GetCamera()->SynchronousRotation(XMFLOAT3(0, 1, 0), mxDelta / 3.f);
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
	
}

void PlayScene::AnimateObjects(float _timeElapsed) {
	//// 적기의 수가 적을 경우 생성한다.
	//if (ppObjects[WORLD_OBJ_LAYER::ENEMY].size() < 30) {
	//	uniform_real_distribution<float> urd(0, 2000.f);

	//	shared_ptr<GameObject> newEnemy = make_shared<GunshipEnemy>();
	//	newEnemy->Create();
	//	newEnemy->SetLocalPosition(XMFLOAT3(urd(rd), 400.f, urd(rd)));
	//	newEnemy->UpdateObject();
	//	AddObject(newEnemy, WORLD_OBJ_LAYER::ENEMY);
	//}

	for (auto& pObjects : ppObjects)
		for (auto& pObject : pObjects)
			if(pObject) pObject->Animate(_timeElapsed);
}
void PlayScene::ProcessCollision(float _timeElapsed) {
	for (auto& pObjects : ppObjects)
		for (auto& pObject : pObjects)
			if (pObject) pObject->ProcessCollision(_timeElapsed);
}
void PlayScene::EraseNullptrElements() {
	static int layerCount = 0;
	auto pred = [](const shared_ptr<GameObject>& _pObject) { return bool(!_pObject); };
	auto removeStartIt = ranges::remove_if(ppObjects[layerCount], pred).begin();	// 반환값이용해서 삭제
	ppObjects[layerCount].erase(removeStartIt, ppObjects[layerCount].end());
	if (layerCount == WORLD_OBJ_LAYER::PLAYER_ATTACK) {
		cout << ppObjects[layerCount].size() << "\n";
	}

	layerCount = (++layerCount) % WORLD_OBJ_LAYER::NUM;
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
	for (auto& pObject : ppObjects[WORLD_OBJ_LAYER::PLAYER])	// 플레이어 드로우
		if(pObject) pObject->Render(_pCommandList);
	for (auto& pObject : ppObjects[WORLD_OBJ_LAYER::ENEMY])	// 적기 드로우
		if(pObject) pObject->Render(_pCommandList);
	for (auto& pObject : ppObjects[WORLD_OBJ_LAYER::PLAYER_ATTACK])
		if (pObject) pObject->Render(_pCommandList);
	for (auto& pObject : ppObjects[WORLD_OBJ_LAYER::ENEMY_ATTACK])
		if (pObject) pObject->Render(_pCommandList);
	
	// 빌보드 그리기
	Shader::GetBillBoardShader()->PrepareRender(_pCommandList);
	for (auto& pObject : ppObjects[WORLD_OBJ_LAYER::BILLBOARD])
		if (pObject) pObject->Render(_pCommandList);

	// 터레인 그리기
	Shader::GetTerrainShader()->PrepareRender(_pCommandList);
	for (auto& pObject : ppObjects[WORLD_OBJ_LAYER::TERRAIN])
		if(pObject) pObject->Render(_pCommandList);

	// NO-COLLIDER 그리기
	Shader::GetAlphaBlendingShader()->PrepareRender(_pCommandList);
	for (auto& pObject : ppObjects[WORLD_OBJ_LAYER::NO_COLLIDER])
		if (pObject) pObject->Render(_pCommandList);
		
	// 히트박스 렌더링
	Shader::GetHitBoxShader()->PrepareRender(_pCommandList);
	for (const auto& pObject : ppObjects[WORLD_OBJ_LAYER::PLAYER])
		if (pObject) pObject->RenderHitBox(_pCommandList, *HitBoxMesh::GetHitBoxMesh());
	for (const auto& pObject : ppObjects[WORLD_OBJ_LAYER::ENEMY])
		if (pObject) pObject->RenderHitBox(_pCommandList, *HitBoxMesh::GetHitBoxMesh());
	for (const auto& pObject : ppObjects[WORLD_OBJ_LAYER::PLAYER_ATTACK])
		if (pObject) pObject->RenderHitBox(_pCommandList, *HitBoxMesh::GetHitBoxMesh());

}

void PlayScene::AddLight(const shared_ptr<Light>& _pLight) {
	pLights.push_back(_pLight);
}
void PlayScene::AddObject(const shared_ptr<GameObject>& _pObject, WORLD_OBJ_LAYER _layer) {
	ppObjects[_layer].push_back(_pObject);
	_pObject->SetLayer(_layer);
}
void PlayScene::DeleteObject(shared_ptr<GameObject> _pObject, WORLD_OBJ_LAYER _layer) {
	auto it = ranges::find(ppObjects[_layer], _pObject);
	if(it != ppObjects[_layer].end())
		it->reset();
}

vector<shared_ptr<GameObject>>& PlayScene::GetObjectsLayered(WORLD_OBJ_LAYER _layer) {
	return ppObjects[_layer];
}
