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

	nReloadedMissileRain = 0;
	missileRainMaxTerm = 0.07f;
	missileRainTerm = 0.f;
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
	//pCamera->SetType(CAMERA_TYPE::THIRD, XMFLOAT3(0.f, 0.f, -20.f));
	SetChild(pCamera);
	pCamera->UpdateObject();

}

void Player::Animate(float _timeElapsed) {
	if (0 < nReloadedMissileRain) {
		missileRainTerm -= _timeElapsed;
		while (missileRainTerm <= 0 && 0 < nReloadedMissileRain) {
			// 미사일 생성
			shared_ptr<GunshipMissile> pMissile = make_shared<GunshipMissile>(300.f, 600.f, 180.f);
			pMissile->Create();
			pMissile->SetLocalPosition(localPosition);
			pMissile->SetLocalRotation(Vector4::RandomQuaternion() );
			pMissile->SetAttackType(1);	//유도
			pMissile->UpdateObject();
			pMissile->SetTargetLayer(WORLD_OBJ_LAYER::ENEMY);
			static_pointer_cast<PlayScene>(GameFramework::Instance().GetCurrentScene())->AddObject(pMissile, WORLD_OBJ_LAYER::PLAYER_ATTACK);

			--nReloadedMissileRain;
			missileRainTerm += missileRainMaxTerm;
		}
	}

	Apache::Animate(_timeElapsed);
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

bool Player::TryFireMissileRain() {
	if (IsRainCoolDown()) {
		nReloadedMissileRain = 15;
		missileRainCoolTime = missileRainMaxCoolTime;
		return true;
	}
	return false;;
}


