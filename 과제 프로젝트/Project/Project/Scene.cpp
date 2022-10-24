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

void Scene::CheckCollision() {

}

///////////////////////////////////////////////////////////////////////////////
/// PlayScene
PlayScene::PlayScene(int _stageNum) {
	globalAmbient = XMFLOAT4(0.1f, 0.1f, 0.1f, 1.0f);
}

void PlayScene::LoadObjects(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	// �ǽ�
	GameObjectManager::LoadFromFile("Gunship", _pDevice, _pCommandList);	
	
	// Tree ������ �ҷ�����
	shared_ptr<BillBoardMesh> billBoardMesh = BillBoardMesh::LoadFromFile("TreeMesh", "Tree1.dds", XMFLOAT2(0, 45), XMFLOAT2(100, 100), _pDevice, _pCommandList);
	shared_ptr<GameObject> pTreeObject = make_shared<GameObject>();
	pTreeObject->Create();
	pTreeObject->SetMesh(billBoardMesh, OOBB_TYPE::DISABLED_FINAL);
	pTreeObject->SetName("Tree1");
	GameObjectManager::AddObject(pTreeObject);

}

void PlayScene::Init(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	GameFramework& gameFramework = GameFramework::Instance();

	LoadObjects(_pDevice, _pCommandList);

	// �÷��̾� ����
	shared_ptr<Player> pPlayer = make_shared<Player>();
	pPlayer->Create();
	camera = pPlayer->GetCamera();	// ī�޶� ����
	ppObjects[WORLD_OBJ_LAYER::PLAYER].push_back(pPlayer);
	wpPlayer = pPlayer;	// palyer�� (weak_ptr)wpPlayer �� ���

	// ���� �����ϱ�
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

	shared_ptr<TerrainMesh> terrainMesh = TerrainMesh::LoadFromFile("terrainHeightMap01.raw", XMINT2(249, 149), XMFLOAT3(2000.f, 500.f, 2000.f), 10.f, terrainMaterialColorsBuffer,
		500.f, terrainTextureFileNames, _pDevice, _pCommandList);
	shared_ptr<GameObject> pTerrainObject = make_shared<GameObject>();
	pTerrainObject->Create();
	pTerrainObject->SetMesh(terrainMesh, OOBB_TYPE::DISABLED_FINAL);
	ppObjects[WORLD_OBJ_LAYER::TERRAIN].push_back(pTerrainObject);

	// ���� ����
	uniform_real_distribution<float> urd(0, 2000.f);
	for (int i = 0; i < 1'000; ++i) {
		shared_ptr<GameObject> newTree = GameObjectManager::GetGameObject("Tree1", _pDevice, _pCommandList);
		XMFLOAT3 randomPos = XMFLOAT3(urd(rd), 0, urd(rd));
		randomPos.y = terrainMesh->GetHeightMapImage().GetHeight(randomPos.x, randomPos.z);
		newTree->SetLocalPosition(randomPos);
		newTree->UpdateObject();

		//newGrass->SetLocalPosition();
		ppObjects[WORLD_OBJ_LAYER::BILLBOARD].push_back(newTree);
	}

	// ���� ���� ����
	UINT ncbElementBytes = ((sizeof(LightsMappedFormat) + 255) & ~255); //256�� ���
	pLightsBuffer = ::CreateBufferResource(_pDevice, _pCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, ComPtr<ID3D12Resource>());
	pLightsBuffer->Map(0, NULL, (void**)&pMappedLights);
	
	// �¾�(����) �߰�
	pSun = make_shared<Light>(nullptr);
	AddLight(pSun);

}

PlayScene::~PlayScene() {

}

void PlayScene::ProcessKeyboardInput(const array<UCHAR, 256>& _keysBuffers, float _timeElapsed) {
	GameFramework& gameFramework = GameFramework::Instance();
	shared_ptr<Player> pPlayer = wpPlayer.lock();

	if (pPlayer) {
		// Ű���� ó��
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

		// ���콺 ó��
		float mxDelta = 0.0f, myDelta = 0.0f;
		POINT currentMousePos;
		if (gameFramework.GetLeftMouseDrag()) {	// ���� ���콺�� �巡�� ���� ���
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

		// ����
		pPlayer->UpdateObject();
	}
	
}

void PlayScene::AnimateObjects(double _timeElapsed) {
	// ������ ���� ���� ��� �����Ѵ�.
	if (ppObjects[WORLD_OBJ_LAYER::ENEMY].size() < 5) {
		shared_ptr<GameObject> newEnemy = make_shared<GunshipEnemy>();
		newEnemy->Create();
		newEnemy->UpdateObject();
		ppObjects[WORLD_OBJ_LAYER::ENEMY].push_back(newEnemy);
	}

	for (auto& pObjects : ppObjects)
		for (auto& pObject : pObjects)
			pObject->Animate(_timeElapsed);
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
	// �����ӿ�ũ���� ������ ���� ��Ʈ�ñ״�ó�� set
	
	// ī�޶� ���̴� ���� ������Ʈ
	camera->SetViewPortAndScissorRect(_pCommandList);
	camera->UpdateShaderVariable(_pCommandList);

	// ���� ���̴� ���� ������Ʈ
	UpdateLightShaderVariables(_pCommandList);

	// �⺻ ���̴� Ȱ��ȭ
	GameFramework& gameFramework = GameFramework::Instance();
	Shader::GetBasicShader()->PrepareRender(_pCommandList);
	for (auto& pObject : ppObjects[WORLD_OBJ_LAYER::PLAYER])	// �÷��̾� ��ο�
		pObject->Render(_pCommandList);
	for (auto& pObject : ppObjects[WORLD_OBJ_LAYER::ENEMY])	// ���� ��ο�
		pObject->Render(_pCommandList);
	
	// �ͷ��� �׸���
	Shader::GetTerrainShader()->PrepareRender(_pCommandList);
	for (auto& pObject : ppObjects[WORLD_OBJ_LAYER::TERRAIN])	// ���� ��ο�
		pObject->Render(_pCommandList);

	// ������ �׸���
	Shader::GetBillBoardShader()->PrepareRender(_pCommandList);
	if (shared_ptr<Player> pPlayer = wpPlayer.lock()) {		// ������ ���� �հ��� ���� ������
		const XMFLOAT3& playerPos = pPlayer->GetWorldPosition();
		auto sortFunc = [playerPos](const XMFLOAT3& pos1, const XMFLOAT3& pos2) { return Vector3::Distance2(pos1, playerPos) > Vector3::Distance2(pos2, playerPos); };
		ranges::sort(ppObjects[WORLD_OBJ_LAYER::BILLBOARD], sortFunc, &GameObject::GetWorldPosition);
	}
	for (auto& pObject : ppObjects[WORLD_OBJ_LAYER::BILLBOARD]) {	// ���� ��ο�
		pObject->Render(_pCommandList);
	}	
		

	// ��Ʈ�ڽ� ������
	Shader::GetHitBoxShader()->PrepareRender(_pCommandList);
	for (const auto& object : ppObjects[WORLD_OBJ_LAYER::PLAYER]) {
		object->RenderHitBox(_pCommandList, *HitBoxMesh::GetHitBoxMesh());
	}

}

void PlayScene::AddLight(const shared_ptr<Light>& _pLight) {
	pLights.push_back(_pLight);
}

vector<shared_ptr<GameObject>>& PlayScene::GetObjectsLayered(WORLD_OBJ_LAYER _layer) {
	return ppObjects[_layer];
}
