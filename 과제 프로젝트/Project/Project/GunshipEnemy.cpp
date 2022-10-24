#include "stdafx.h"
#include "GunshipEnemy.h"
#include "GameFramework.h"

void GunshipEnemy::SetTarget(shared_ptr<GameObject> _target) {
	wpTarget = _target;
}

void GunshipEnemy::Animate(double _timeElapsed) {
	shared_ptr<GameObject> pTarget = wpTarget.lock();

	if (!pTarget) {	// Ÿ���� ���� ���
		auto& pPlayerObjects = ((shared_ptr<PlayScene>&)GameFramework::Instance().GetCurrentScene())->GetObjectsLayered(WORLD_OBJ_LAYER::PLAYER);
		
		for (auto& pPlayer : pPlayerObjects) {
			if (pPlayer) {	// �÷��̾ ���� ���
				float distance = Vector3::Distance(Vector3::Subtract(pPlayer->GetWorldPosition(), GetWorldPosition()));
				if (distance < 500.f) {
					SetTarget(pPlayer);
					break;
				}
			}
		}
	}

	else {	// Ÿ���� ���� ���
		XMFLOAT3 targetPos = pTarget->GetWorldPosition();
		XMFLOAT3 myPos = GetWorldPosition();
		XMFLOAT3 targetDir = Vector3::Subtract(targetPos, myPos);
		float distance = Vector3::Distance(targetDir);
		
		// �����̵�
		if (distance < 30.f) {	// Ÿ�ٰ� ������ ȸ���� �Ѵ�.
			float comebackSpeed = hSpeed; 
			hSpeed = 0;
			MoveHorizontal(targetDir, _timeElapsed);
			hSpeed = comebackSpeed;
		}
		else if(distance < 1000.f) {	// Ÿ�ٰ� �ָ� ������ ���鼭 ȸ���� �Ѵ�.
			MoveHorizontal(targetDir, _timeElapsed);
		}
		else {	// Ÿ�ٰ� �ʹ� �ָ� ��ģ��.
			wpTarget.reset();
		}

		// ���� �̵��� �Ѵ�.
		if (myPos.y + 1 < targetPos.y) {
			MoveVertical(true, _timeElapsed);
		}
		else if (targetPos.y < myPos.y - 1) {
			MoveVertical(false, _timeElapsed);
		}

		UpdateObject();
	}

	shared_ptr<GameObject> pRootParent = GetRootParent();
	// ���� ������ �浹üũ
	if (!wpParent.lock()) {
		auto& pEnemyObjects = ((shared_ptr<PlayScene>&)GameFramework::Instance().GetCurrentScene())->GetObjectsLayered(WORLD_OBJ_LAYER::ENEMY);
		for (auto& pEnemy : pEnemyObjects) {
			if (pEnemy && pEnemy != shared_from_this() && CheckCollision(pEnemy)) {	// �ڱ� �ڽ��� ����
				XMFLOAT3 SelfToOther = Vector3::Subtract(pEnemy->GetWorldPosition(), GetWorldPosition());
				SelfToOther = Vector3::ScalarProduct(Vector3::Normalize(SelfToOther), 0.1f);
				pEnemy->Move(SelfToOther);
				pEnemy->UpdateObject();
				Move(Vector3::ScalarProduct(SelfToOther, -1));
				UpdateObject();
			}
		}
	}
	
	XMFLOAT3 localPos = pRootParent->GetLocalPosition();	// ���� loaclPos == worldPos
	if (localPos.x < 0.f || 2000.f < localPos.x || localPos.z < 0.f || 2000.f < localPos.z) {
		localPos.x = clamp(localPos.x, 0.f, 2000.f);
		localPos.z = clamp(localPos.z, 0.f, 2000.f);
		pRootParent->SetLocalPosition(localPos);
		pRootParent->UpdateObject();
	}

	Gunship::Animate(_timeElapsed);
}
