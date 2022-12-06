#include "stdafx.h"
#include "Apache.h"

Apache::Apache() {
	hSpeed = 100.f;
	vSpeed = 30.f;
	rSpeed = 360.f;

	missileMaxCoolTime = 1.f;
	missileCoolTime = 0.f;
	missileRainMaxCoolTime = 5.f;
	missileRainCoolTime = 0.f;

}

Apache::~Apache() {
}

void Apache::Create() {
	Helicopter::Create();

	// Apache모델을 가진 오브젝트를 복사해 와서 자식으로 붙인다.
	SetChild(GameObjectManager::GetGameObject("Apache"));

	pRotor = GetChild("GameObject_rotor");
	pBackRotor[0] = GetChild("GameObject_black m 6");
	pBackRotor[1] = GetChild("GameObject_black m 7");

	if (!pPrivateOOBB) {
		shared_ptr<BoundingOrientedBox> pNewPrivateOOBB = make_shared<BoundingOrientedBox>();
		pNewPrivateOOBB->Center = XMFLOAT3(0.f, 0.667923f, -2.f);
		pNewPrivateOOBB->Extents = XMFLOAT3(9.f, 4.f, 11.f);
		SetPrivateOOBB(pNewPrivateOOBB, OOBB_TYPE::PRIVATEOOBB_COVER);
	}
}

void Apache::Animate(float _timeElapsed) {
	missileCoolTime -= _timeElapsed;
	missileRainCoolTime -= _timeElapsed;

	// 날개 회전
	if (pRotor) {
		
		pRotor->Rotate(pRotor->GetLocalUpVector(), 360 * 4 * _timeElapsed);
		pRotor->UpdateObject();
	}

	if (pBackRotor[0]) {
		pBackRotor[0]->Rotate(XMFLOAT3(1,0,0), 360 * 5 * _timeElapsed);
		pBackRotor[0]->UpdateObject();
	}

	if (pBackRotor[1]) {
		pBackRotor[1]->Rotate(XMFLOAT3(1, 0, 0), 360 * 5 * _timeElapsed);
		pBackRotor[1]->UpdateObject();
	}

	Helicopter::Animate(_timeElapsed);
}

bool Apache::IsCoolDown() {
	return missileCoolTime <= 0;
}
bool Apache::IsRainCoolDown() {
	return missileRainCoolTime <= 0;
}