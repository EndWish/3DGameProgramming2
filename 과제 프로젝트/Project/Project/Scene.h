#pragma once
#include "Player.h"
#include "Light.h"

enum WORLD_OBJ_LAYER {
	PLAYER,
	TERRAIN,
	ENEMY,
	BILLBOARD,

	NUM
};

class Scene {
protected:

public:
	Scene();
	virtual ~Scene();

public:
	virtual void Init(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) = 0;
	virtual void ProcessKeyboardInput(const array<UCHAR, 256>& _keysBuffers, float _timeElapsed) = 0;
	virtual void AnimateObjects(double _timeElapsed) = 0;
	virtual void CheckCollision();
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) = 0;
};


///////////////////////////////////////////////////////////////////////////////
/// PlayScene
class PlayScene : public Scene {
protected:
	
	array<vector<shared_ptr<GameObject>>, WORLD_OBJ_LAYER::NUM> ppObjects;
	weak_ptr<Player> wpPlayer;

	shared_ptr<Light> pSun;

	ComPtr<ID3D12Resource> pLightsBuffer;
	vector<weak_ptr<Light>> pLights;
	shared_ptr<LightsMappedFormat> pMappedLights;

	XMFLOAT4 globalAmbient;
	shared_ptr<Camera> camera;

	//shared_ptr<TerrainMesh> terrainMesh;	//[юс╫ц]
	
public:
	PlayScene(int _stageNum);
	~PlayScene() final;

public:
	void LoadObjects(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	void Init(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) final;
	void ProcessKeyboardInput(const array<UCHAR, 256>& _keysBuffers, float _timeElapsed) final;
	void AnimateObjects(double _timeElapsed) final;
	void CheckCollision() final;
	void UpdateLightShaderVariables(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) final;

	void AddLight(const shared_ptr<Light>& _pLight);

	vector<shared_ptr<GameObject>>& GetObjectsLayered(WORLD_OBJ_LAYER _layer);
};