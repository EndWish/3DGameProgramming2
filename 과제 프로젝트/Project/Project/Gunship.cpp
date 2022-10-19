#include "stdafx.h"
#include "Gunship.h"
#include "GameFramework.h"

Gunship::Gunship() {

}

Gunship::~Gunship() {
}

void Gunship::Create() {
	Helicopter::Create();

	const ComPtr<ID3D12Device>& pDevice = GameFramework::GetDevice();
	const ComPtr<ID3D12GraphicsCommandList>& pCommandList = GameFramework::GetCommandList();

	// Apache모델을 가진 오브젝트를 복사해 와서 자식으로 붙인다.
	SetChild(GameObjectManager::GetGameObject("Gunship", pDevice, pCommandList));

	pRotor = GetChild("GameObject_Rotor");
	pBackRotor = GetChild("GameObject_Back_Rotor");

}

void Gunship::Animate(double _timeElapsed) {
	if (pRotor) {
		pRotor->Rotate(XMFLOAT3(0, 1, 0), 360 * 4 * (float)_timeElapsed);
		pRotor->UpdateObject();
	}

	if (pBackRotor) {
		pBackRotor->Rotate(XMFLOAT3(1, 0, 0), 360 * 5 * (float)_timeElapsed);
		pBackRotor->UpdateObject();
	}

	Helicopter::Animate(_timeElapsed);
}