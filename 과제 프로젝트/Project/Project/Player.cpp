#include "stdafx.h"
#include "Player.h"
#include "GameFramework.h"

Player::Player() {
}

Player::~Player() {

}

shared_ptr<Camera> Player::GetCamera() const {
	return pCamera;
}

void Player::Create() {
	Gunship::Create();

	const ComPtr<ID3D12Device>& pDevice = GameFramework::GetDevice();
	const ComPtr<ID3D12GraphicsCommandList>& pCommandList = GameFramework::GetCommandList();

	pCamera = make_shared<Camera>();
	pCamera->Create(pDevice, pCommandList);
	pCamera->SetLocalPosition(XMFLOAT3(0.f, 0.f, -20.f));
	SetChild(pCamera);
	pCamera->UpdateObject();

}
