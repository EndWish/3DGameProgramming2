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
	// �躤�Ϳ� Ÿ�ٺ��͸� xz��鿡 �����Ѵ�.
	_dir.y = 0;
	XMFLOAT3 origin = GetLocalLookVector();
	origin.y = 0;

	XMFLOAT3 axis = Vector3::Cross(origin, _dir);
	float minAngle = Vector3::Angle(origin, _dir);
	if (abs(axis.y) <= numeric_limits<float>::epsilon()) {	// ������ �Ұ����� ��� (�� ���Ͱ� ������ ���)
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
