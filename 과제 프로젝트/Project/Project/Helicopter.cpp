#include "stdafx.h"
#include "Helicopter.h"
#include "GameFramework.h"

Helicopter::Helicopter() {
	hSpeed = 100.f;
	vSpeed = 50.f;
	rSpeed = 360.f;
}

Helicopter::~Helicopter() {

}

void Helicopter::MoveHorizontal(XMFLOAT3 _dir, float _timeElapsed) {
	// �躤�Ϳ� Ÿ�ٺ��͸� xz��鿡 �����Ѵ�.
	_dir.y = 0;
	XMFLOAT3 origin = GetLocalLookVector();
	origin.y = 0;

	XMFLOAT3 axis = Vector3::Cross(origin, _dir);
	float minAngle = Vector3::Angle(origin, _dir, false);
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

void Helicopter::Animate(float _timeElapsed) {
	// �� ���� �浹 üũ
	shared_ptr<GameObject> pRootParent = GetRootParent();
	XMFLOAT3 localPos = pRootParent->GetLocalPosition();	// ���� loaclPos == worldPos
	if (localPos.x < 0.f || 2000.f < localPos.x || localPos.z < 0.f || 2000.f < localPos.z) {
		localPos.x = clamp(localPos.x, 0.f, 2000.f);
		localPos.z = clamp(localPos.z, 0.f, 2000.f);
		pRootParent->SetLocalPosition(localPos);
		pRootParent->UpdateObject();
	}

	if (!IsOOBBTypeDisabled(OOBBType)) {
		// boundingBox �� ���� �κ��� ���� ����
		auto& pTerrainObjects = ((shared_ptr<PlayScene>&)GameFramework::Instance().GetCurrentScene())->GetObjectsLayered(WORLD_OBJ_LAYER::TERRAIN);
		array<XMFLOAT3, 8> corners;
		boundingBox.GetCorners(corners.data());

		for (const auto& terrain : pTerrainObjects) {
			const HeightMapImage& heightMapImage = static_pointer_cast<TerrainMesh>(terrain->GetMesh())->GetHeightMapImage();
			float depth = 0;
			for (const XMFLOAT3& corner : corners) {
				float TerrainHeight = heightMapImage.GetHeight(corner.x, corner.z);
				if (corner.y < TerrainHeight) {
					depth = max(depth, TerrainHeight - corner.y);
				}
			}

			if (FLT_EPSILON < depth) {
				pRootParent->Move(XMFLOAT3(0, depth, 0));
				pRootParent->UpdateObject();
			}
		}
	}


	GameObject::Animate(_timeElapsed);
}
