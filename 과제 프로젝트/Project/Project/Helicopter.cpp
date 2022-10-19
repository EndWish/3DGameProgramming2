#include "stdafx.h"
#include "Helicopter.h"
#include "GameFramework.h"

Helicopter::Helicopter() {
	
}

Helicopter::~Helicopter() {
}

void Helicopter::Create() {
	
}

void Helicopter::MoveHorizontal(XMFLOAT3 _dir, float _timeElapsed) {
	// 룩벡터와 타겟벡터를 xz평면에 투영한다.
	_dir.y = 0;
	XMFLOAT3 origin = GetLocalLookVector();
	origin.y = 0;

	XMFLOAT3 axis = Vector3::Cross(origin, _dir);
	float minAngle = Vector3::Angle(origin, _dir);
	if (abs(axis.y) <= numeric_limits<float>::epsilon()) {	// 외적이 불가능한 경우 (두 벡터가 평행한 경우)
		axis = XMFLOAT3(0, 1, 0);
	}

	localRotation = Vector4::QuaternionMultiply(localRotation, Vector4::QuaternionRotation(axis, min(rSpeed * _timeElapsed, minAngle)));
	MoveFront(hSpeed * _timeElapsed);
}

void Helicopter::MoveVertical(bool _up, float _timeElapsed) {
	if (_up) {
		Move(XMFLOAT3(0, vSpeed * _timeElapsed, 0));
	}
	else {
		Move(XMFLOAT3(0, - vSpeed * _timeElapsed, 0));
	}

	
}
