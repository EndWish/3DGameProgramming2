#pragma once
#include "Player.h"
#include "Light.h"

class Scene {
protected:

public:
	Scene();
	virtual ~Scene();

public:
	virtual void Init(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) = 0;
	virtual void ProcessKeyboardInput(const array<bool, 256>& _keysDownStateBuffers, const array<bool, 256>& _keysDownBuffers, const array<bool, 256>& _keysUpBuffers, float _timeElapsed) = 0;
	virtual void AnimateObjects(float _timeElapsed) = 0;
	virtual void ProcessCollision(float _timeElapsed) = 0;
	virtual void EraseNullptrElements() = 0;
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

	// OOBB ±×¸®±â
	bool renderOOBBBox = true;
	
public:
	PlayScene(int _stageNum);
	~PlayScene() final;

public:
	void LoadObjects(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	void Init(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) final;
	void ProcessKeyboardInput(const array<bool, 256>& _keysDownStateBuffers, const array<bool, 256>& keysDownBuffers, const array<bool, 256>& keysUpBuffers, float _timeElapsed) final;
	void AnimateObjects(float _timeElapsed) final;
	void ProcessCollision(float _timeElapsed) final;
	void EraseNullptrElements() final;
	void UpdateLightShaderVariables(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) final;

	void AddLight(const shared_ptr<Light>& _pLight);
	void AddObject(const shared_ptr<GameObject>& _pObject, WORLD_OBJ_LAYER _layer);
	void DeleteObject(shared_ptr<GameObject> _pObject, WORLD_OBJ_LAYER _layer);

	vector<shared_ptr<GameObject>>& GetObjectsLayered(WORLD_OBJ_LAYER _layer);
};