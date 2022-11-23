#include "stdafx.h"
#include "Player.h"
#include "GameFramework.h"

Player::Player() {
	hp = 100000.f;
	hpm = 100000.f;

	hSpeed = 300.f;
	vSpeed = 100.f;
	rSpeed = 720.f;

	missileMaxCoolTime = 0.2f;
}

Player::~Player() {

}

shared_ptr<Camera> Player::GetCamera() const {
	return pCamera;
}

void Player::Create() {
	Apache::Create();

	const ComPtr<ID3D12Device>& pDevice = GameFramework::GetDevice();
	const ComPtr<ID3D12GraphicsCommandList>& pCommandList = GameFramework::GetCommandList();

	pCamera = make_shared<Camera>();
	pCamera->Create(pDevice, pCommandList);
	//pCamera->SetLocalPosition(XMFLOAT3(0.f, 0.f, -20.f));
	pCamera->SetType(CAMERA_TYPE::FIRST, XMFLOAT3(0.45f, 1.f, 3.f));
	SetChild(pCamera);
	pCamera->UpdateObject();

}

bool Player::TryFireMissile() {
	if (IsCoolDown()) {
		shared_ptr<GunshipMissile> pMissile = make_shared<GunshipMissile>(300.f, 800.f, 200.f);
		pMissile->Create();
		pMissile->SetLocalPosition(localPosition);
		pMissile->SetLocalRotation(localRotation);
		pMissile->UpdateObject();
		pMissile->SetTargetLayer(WORLD_OBJ_LAYER::ENEMY);
		static_pointer_cast<PlayScene>(GameFramework::Instance().GetCurrentScene())->AddObject(pMissile, WORLD_OBJ_LAYER::PLAYER_ATTACK);
		missileCoolTime = missileMaxCoolTime;
		return true;
	}
	return false;
}