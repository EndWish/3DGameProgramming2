#include "stdafx.h"
#include "Gunship.h"
#include "GameFramework.h"


Gunship::Gunship() {
	hSpeed = 100.f;
	vSpeed = 30.f;
	rSpeed = 360.f;

	missileMaxCoolTime = 1.f;
	missileCoolTime = 0.f;
}

Gunship::~Gunship() {
}

void Gunship::Create() {
	Helicopter::Create();

	// Apache모델을 가진 오브젝트를 복사해 와서 자식으로 붙인다.
	SetChild(GameObjectManager::GetGameObject("Gunship"));

	pRotor = GetChild("GameObject_Rotor");
	pBackRotor = GetChild("GameObject_Back_Rotor");

	if (!pPrivateOOBB) {
		shared_ptr<BoundingOrientedBox> pNewPrivateOOBB = make_shared<BoundingOrientedBox>();
		pNewPrivateOOBB->Center = XMFLOAT3(0.f, 0.667923f, -2.f);
		pNewPrivateOOBB->Extents = XMFLOAT3(9.f, 4.f, 11.f);
		SetPrivateOOBB(pNewPrivateOOBB, OOBB_TYPE::PRIVATEOOBB_COVER);
	}
}

void Gunship::Animate(float _timeElapsed) {
	missileCoolTime -= _timeElapsed;

	// 날개 회전
	if (pRotor) {
		pRotor->Rotate(XMFLOAT3(0, 1, 0), 360 * 4 * _timeElapsed);
		pRotor->UpdateObject();
	}

	if (pBackRotor) {
		pBackRotor->Rotate(XMFLOAT3(1, 0, 0), 360 * 5 * _timeElapsed);
		pBackRotor->UpdateObject();
	}

	Helicopter::Animate(_timeElapsed);
}

bool Gunship::IsCoolDown() {
	return missileCoolTime <= 0;
}
