// header.h: 표준 시스템 포함 파일
// 또는 프로젝트 특정 포함 파일이 들어 있는 포함 파일입니다.
//

#pragma once

#pragma comment(linker, "/entry:wWinMainCRTStartup /subsystem:console")    // 콘솔창 띄우기( 테스트를 위한 용도 )
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 거의 사용되지 않는 내용을 Windows 헤더에서 제외합니다.
#define MAX_LIGHTS 100
#define GRAVITY 9.8
//#define DEBUG

// Windows 헤더 파일
#include <windows.h>
// C 런타임 헤더 파일입니다.
#include <stdlib.h>
#include <malloc.h>
#include <memory.h>
#include <tchar.h>
#include <exception>
#include <string>
#include <wrl.h>
#include <shellapi.h>

#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_4.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>

#include <mmsystem.h>

#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")

// io
#include <iostream>

// 스마트 포인터 
#include <memory>

// 컨테이너
#include <vector>
#include <array>
#include <unordered_map>
#include <ranges>
#include <queue>
#include <stack>
#include <format>
#include <fstream>
#include <random>

#include <algorithm>

using namespace std;
using namespace DirectX;
using namespace DirectX::PackedVector;

using Microsoft::WRL::ComPtr;

#define PARAMETER_STANDARD_TEXTURE		4	// 루트 시그니쳐에서 확인

// file로 부터 string을 읽는다.
void ReadStringBinary(string& _dest, ifstream& _file);

// 리소스 생성
ComPtr<ID3D12Resource> CreateBufferResource(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, void* _pData, UINT _byteSize, D3D12_HEAP_TYPE _heapType, D3D12_RESOURCE_STATES _resourceStates, const ComPtr<ID3D12Resource>& _pUploadBuffer);
ComPtr<ID3D12Resource> CreateTextureResourceFromDDSFile(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, const wstring _fileName, const ComPtr<ID3D12Resource>& _pUploadBuffer, D3D12_RESOURCE_STATES d3dResourceStates);

// 리소스 배리어
void SynchronizeResourceTransition(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, const ComPtr<ID3D12Resource>& _pResource, D3D12_RESOURCE_STATES stateBefore, D3D12_RESOURCE_STATES stateAfter);

//xmfloat 출력하기
std::ostream& operator<<(std::ostream& os, const XMFLOAT3& f3);
std::ostream& operator<<(std::ostream& os, const XMFLOAT4& f4);
std::ostream& operator<<(std::ostream& os, const XMFLOAT4X4& f4x4);

extern UINT gnRtvDescriptorIncrementSize;
extern UINT gnDsvDescriptorIncrementSize;
extern UINT gnCbvSrvDescriptorIncrementSize;

extern random_device rd;

enum WORLD_OBJ_LAYER {
	PLAYER,
	TERRAIN,
	ENEMY,
	BILLBOARD,
	PLAYER_ATTACK,
	ENEMY_ATTACK,
	NO_COLLIDER,
	NUM,
	NONE,
};

enum class SHADER_TYPE : UINT {
	BASIC,
	AlphaBlending,
	HitBox,
	Terrain,
	BillBoard,
	ParticleStreamOut,
	ParticleDraw,
	NUM,
	NONE,
};

enum class CAMERA_TYPE {
	FIRST,
	THIRD
};

namespace Vector3 {

	inline XMFLOAT3 Normalize(const XMFLOAT3& _vector) {
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMVector3Normalize(XMLoadFloat3(&_vector)));
		return(result);
	}
	inline XMFLOAT3 Normalize(float _x, float _y, float _z) {
		XMFLOAT3 result(_x, _y, _z);
		XMStoreFloat3(&result, XMVector3Normalize(XMLoadFloat3(&result)));
		return(result);
	}

	inline XMFLOAT3 ScalarProduct(const XMFLOAT3& _vector, float _scalar) {
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMLoadFloat3(&_vector) * _scalar);
		return result;
	}
	inline XMFLOAT3 Add(const XMFLOAT3& _vector1, const XMFLOAT3& _vector2) {
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMLoadFloat3(&_vector1) + XMLoadFloat3(&_vector2));
		return result;
	}
	inline XMFLOAT3 Add(const XMFLOAT3& _vector1, const  XMFLOAT3& _vector2, float _scalar) {
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMLoadFloat3(&_vector1) + (XMLoadFloat3(&_vector2) * _scalar));
		return result;
	}
	inline XMFLOAT3 Subtract(const XMFLOAT3& _vector1, const  XMFLOAT3& _vector2) {
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMLoadFloat3(&_vector1) - XMLoadFloat3(&_vector2));
		return result;
	}
	inline XMFLOAT3 Transform(const XMFLOAT3& _vector, const XMFLOAT4X4& _matrix) {
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMVector3Transform(XMLoadFloat3(&_vector), XMLoadFloat4x4(&_matrix)));
		return result;
	}

	inline float Distance(const XMFLOAT3& _vector1, const  XMFLOAT3& _vector2) {
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMVector3Length(XMLoadFloat3(&_vector1) - XMLoadFloat3(&_vector2)));
		return result.x;
	}
	inline float Distance(const XMFLOAT3& _vector) {
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMVector3Length(XMLoadFloat3(&_vector)));
		return result.x;
	}
	inline float Distance2(const XMFLOAT3& _vector1, const  XMFLOAT3& _vector2) {
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMVector3LengthSq(XMLoadFloat3(&_vector1) - XMLoadFloat3(&_vector2)));
		return result.x;
	}
	inline float Distance2(const XMFLOAT3& _vector) {
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMVector3LengthSq(XMLoadFloat3(&_vector)));
		return result.x;
	}

	inline XMFLOAT3 Cross(const XMFLOAT3& _vector1, const XMFLOAT3& _vector2) {
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMVector3Cross(XMLoadFloat3(&_vector1), XMLoadFloat3(&_vector2)));
		return result;
	}
	inline XMFLOAT3 Cross(const XMFLOAT3& _point1, const XMFLOAT3& _point2, const XMFLOAT3& _point3) {
		XMFLOAT3 result;
		XMVECTOR vector1 = XMLoadFloat3(&_point2) - XMLoadFloat3(&_point1);
		XMVECTOR vector2 = XMLoadFloat3(&_point3) - XMLoadFloat3(&_point2);
		XMStoreFloat3(&result, XMVector3Cross(vector1, vector2));
		return result;
	}
	inline float Dot(const XMFLOAT3& _vector1, const XMFLOAT3& _vector2) {
		XMFLOAT3 result;
		XMStoreFloat3(&result, XMVector3Dot(XMLoadFloat3(&_vector1), XMLoadFloat3(&_vector2)));
		return result.x;
	}

	inline float Angle(const XMVECTOR& xmvVector1, const XMVECTOR& xmvVector2, bool betweenNormals) {
		XMVECTOR xmvAngle;
		if (betweenNormals) {
			xmvAngle = XMVector3AngleBetweenNormals(xmvVector1, xmvVector2);
		}
		else {
			xmvAngle = XMVector3AngleBetweenNormals(XMVector3Normalize(xmvVector1), XMVector3Normalize(xmvVector2));
			//xmvAngle = XMVector3AngleBetweenVectors(xmvVector1, xmvVector2);
		}
		return XMConvertToDegrees(XMVectorGetX(xmvAngle));
	}

	inline float Angle(const XMFLOAT3& xmf3Vector1, const XMFLOAT3& xmf3Vector2, bool betweenNormals) {
		return(Angle(XMLoadFloat3(&xmf3Vector1), XMLoadFloat3(&xmf3Vector2), betweenNormals));
	}

}

namespace Vector4 {
	inline XMFLOAT4 QuaternionMultiply(const XMFLOAT4& _quaternion1, const XMFLOAT4& _quaternion2) {
		XMFLOAT4 result;
		XMStoreFloat4(&result, XMQuaternionMultiply(XMLoadFloat4(&_quaternion1), XMLoadFloat4(&_quaternion2)));
		return result;
	}

	inline XMFLOAT4 QuaternionRotation(const XMFLOAT3& _axis, float _angle) {
		XMFLOAT4 result;
		XMStoreFloat4(&result, XMQuaternionRotationAxis(XMLoadFloat3(&_axis), XMConvertToRadians(_angle)));
		return result;
	}

	inline XMFLOAT4 Transform(const XMFLOAT4& _vector, const XMFLOAT4X4& _matrix) {
		XMFLOAT4 result;
		XMStoreFloat4(&result, XMVector4Transform(XMLoadFloat4(&_vector), XMLoadFloat4x4(&_matrix)));
		return result;
	}

}

namespace Matrix4x4 {
	inline XMFLOAT4X4 Identity() {
		XMFLOAT4X4 result;
		XMStoreFloat4x4(&result, XMMatrixIdentity());
		return result;
	}

	inline XMFLOAT4X4 Multiply(const XMFLOAT4X4& _matrix1, const XMFLOAT4X4& _matrix2) {
		XMFLOAT4X4 result;
		XMStoreFloat4x4(&result, XMLoadFloat4x4(&_matrix1) * XMLoadFloat4x4(&_matrix2));
		return(result);
	}

	inline XMFLOAT4X4 RotationAxis(const XMFLOAT3& _axis, float _angle) {
		XMFLOAT4X4 result;
		XMMATRIX rotateMatrix = XMMatrixRotationAxis(XMLoadFloat3(&_axis), XMConvertToRadians(_angle));
		XMStoreFloat4x4(&result, rotateMatrix);
		return result;
	}

	inline XMFLOAT4X4 RotateQuaternion(const XMFLOAT4& _quaternion) {
		XMFLOAT4X4 result;
		XMMATRIX rotateMatrix = XMMatrixRotationQuaternion(XMLoadFloat4(&_quaternion));
		XMStoreFloat4x4(&result, rotateMatrix);
		return result;
	}

	inline XMFLOAT4X4 RotatePitchYawRoll(float _pitch, float _yaw, float _roll) {
		XMFLOAT4X4 result;
		XMMATRIX rotateMatrix = XMMatrixRotationRollPitchYaw(XMConvertToRadians(_pitch), XMConvertToRadians(_yaw), XMConvertToRadians(_roll));
		XMStoreFloat4x4(&result, rotateMatrix);
		return result;
	}

	inline XMFLOAT4X4 LookAtLH(const XMFLOAT3& _eyePosition, const XMFLOAT3& _lookAtPosition, const XMFLOAT3& _upDirection) {
		XMFLOAT4X4 result;
		XMStoreFloat4x4(&result, XMMatrixLookAtLH(XMLoadFloat3(&_eyePosition), XMLoadFloat3(&_lookAtPosition), XMLoadFloat3(&_upDirection)));
		return(result);
	}
	inline XMFLOAT4X4 PerspectiveFovLH(float _fovAngleY, float _aspectRatio, float _nearZ, float _farZ) {
		XMFLOAT4X4 result;
		XMStoreFloat4x4(&result, XMMatrixPerspectiveFovLH(_fovAngleY, _aspectRatio, _nearZ, _farZ));
		return result;
	}
	inline XMFLOAT4X4 ScaleTransform(const XMFLOAT3& _scale) {
		XMFLOAT4X4 result;
		XMStoreFloat4x4(&result, XMMatrixScaling(_scale.x, _scale.y, _scale.z));
		return result;
	}
	inline XMFLOAT4X4 MoveTransform(const XMFLOAT3& _move) {
		XMFLOAT4X4 result;
		XMStoreFloat4x4(&result,XMMatrixTranslationFromVector(XMLoadFloat3(&_move)));
		return result;
	}
	inline XMFLOAT4X4 MoveTransform(float x, float y, float z) {
		XMFLOAT4X4 result;
		XMStoreFloat4x4(&result, XMMatrixTranslation(x, y, z));
		return result;
	}
}

