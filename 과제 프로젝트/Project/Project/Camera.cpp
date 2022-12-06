#include "stdafx.h"
#include "Camera.h"
#include "GameFramework.h"

Camera::Camera() {
	viewTransform = Matrix4x4::Identity();
	projectionTransform = Matrix4x4::Identity();
	viewPort = { 0,0, 1920, 1080, 0, 1 };
	scissorRect = { 0,0, 1920, 1080 };

	type = CAMERA_TYPE::THIRD;
}

Camera::~Camera() {

}

void Camera::Create(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	GameObject::Create();
	GameFramework& gameFramework = GameFramework::Instance();	// gameFramework의 래퍼런스를 가져온다.

	name = "카메라";


	auto [width, height] = gameFramework.GetClientSize();
	viewPort = { 0,0, (float)width, (float)height, 0, 1 };
	scissorRect = { 0,0, width, height };

	UINT cbElementSize = (sizeof(VS_CameraMappedFormat) + 255) & (~255);
	ComPtr<ID3D12Resource> temp;
	pCameraBuffer = CreateBufferResource(_pDevice, _pCommandList, NULL, cbElementSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, temp);
	pCameraBuffer->Map(0, NULL, (void**)&pMappedCamera);

	UpdateViewTransform();
	UpdateProjectionTransform(0.2f, 2000.0f, float(width) / height, 75.0f);
}

void Camera::SetViewPortAndScissorRect(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	_pCommandList->RSSetViewports(1, &viewPort);
	_pCommandList->RSSetScissorRects(1, &scissorRect);
}

void Camera::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	XMFLOAT4X4 view;
	XMStoreFloat4x4(&view, XMMatrixTranspose(XMLoadFloat4x4(&viewTransform)));	// 쉐이더는 열?우선 행렬이기 때문에 전치행렬로 바꾸어서 보내준다.
	memcpy(&pMappedCamera->view, &view, sizeof(XMFLOAT4X4));

	XMFLOAT4X4 projection;
	XMStoreFloat4x4(&projection, XMMatrixTranspose(XMLoadFloat4x4(&projectionTransform)));	// 쉐이더는 열?우선 행렬이기 때문에 전치행렬로 바꾸어서 보내준다.
	memcpy(&pMappedCamera->projection, &projection, sizeof(XMFLOAT4X4));

	XMFLOAT3 worldPosition = GetWorldPosition();
	memcpy(&pMappedCamera->position, &worldPosition, sizeof(XMFLOAT3));

	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress = pCameraBuffer->GetGPUVirtualAddress();
	_pCommandList->SetGraphicsRootConstantBufferView(0, gpuVirtualAddress);
}

void Camera::UpdateViewTransform() {
	XMFLOAT3 worldPosition = GetWorldPosition();

	XMFLOAT3 lookAtWorld = Vector3::Add(worldPosition, GetWorldLookVector());
	viewTransform = Matrix4x4::LookAtLH(worldPosition, lookAtWorld, GetWorldUpVector());
}

void Camera::UpdateProjectionTransform(float _nearDistance, float _farDistance, float _aspectRatio, float _fovAngle) {
	projectionTransform = Matrix4x4::PerspectiveFovLH(XMConvertToRadians(_fovAngle), _aspectRatio, _nearDistance, _farDistance);

}

void Camera::UpdateWorldTransform() {
	if (type == CAMERA_TYPE::FIRST) {
		GameObject::UpdateWorldTransform();
	}
	else if(type == CAMERA_TYPE::THIRD) {
		if (auto pParentLock = wpParent.lock()) {	// 부모가 있을 경우
			worldTransform = Matrix4x4::Multiply(localTransform, Matrix4x4::MoveTransform(pParentLock->GetWorldPosition()));
		}
		else {	// 부모가 없을 경우
			worldTransform = localTransform;
		}
	}
	UpdateViewTransform();
}

void Camera::SetType(CAMERA_TYPE _type, const XMFLOAT3& _localPos) {
	type = _type;
	localPosition = _localPos;
}

CAMERA_TYPE Camera::GetType() const {
	return type;
}


