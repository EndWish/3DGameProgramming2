#include "stdafx.h"
#include "GunshipEnemy.h"
#include "GameFramework.h"

GunshipEnemy::GunshipEnemy() {
	
}
GunshipEnemy::~GunshipEnemy() {

}

void GunshipEnemy::SetTarget(shared_ptr<GameObject> _target) {
	wpTarget = _target;
}

void GunshipEnemy::Animate(float _timeElapsed) {
	shared_ptr<GameObject> pTarget = wpTarget.lock();

	if (!pTarget) {	// 타깃이 없을 경우
		auto& pPlayerObjects = ((shared_ptr<PlayScene>&)GameFramework::Instance().GetCurrentScene())->GetObjectsLayered(WORLD_OBJ_LAYER::PLAYER);
		
		for (auto& pPlayer : pPlayerObjects) {
			if (pPlayer) {	// 플레이어가 있을 경우
				float distance = Vector3::Distance(Vector3::Subtract(pPlayer->GetWorldPosition(), GetWorldPosition()));
				if (distance < 5000.f) {
					SetTarget(pPlayer);
					break;
				}
			}
		}
	}

	else {	// 타깃이 있을 경우
		XMFLOAT3 targetPos = pTarget->GetWorldPosition();
		XMFLOAT3 myPos = GetWorldPosition();
		XMFLOAT3 targetDir = Vector3::Subtract(targetPos, myPos);
		float distance = Vector3::Distance(targetDir);
		
		// 수평이동
		if (distance < 100.f) {	// 타겟과 가까우면 회전만 한다.
			float comebackSpeed = hSpeed; 
			hSpeed = 0;
			MoveHorizontal(targetDir, _timeElapsed);
			hSpeed = comebackSpeed;
		}
		else if(distance < 10000.f) {	// 타겟과 멀면 앞으로 가면서 회전도 한다.
			MoveHorizontal(targetDir, _timeElapsed);
		}
		else {	// 타겟과 너무 멀면 놓친다.
			wpTarget.reset();
		}

		// 상하 이동을 한다.
		if (myPos.y + 1 < targetPos.y) {
			MoveVertical(true, _timeElapsed);
		}
		else if (targetPos.y < myPos.y - 1) {
			MoveVertical(false, _timeElapsed);
		}

		UpdateObject();
	}

	shared_ptr<GameObject> pRootParent = GetRootParent();
	// 같은 적과의 충돌체크
	if (!wpParent.lock()) {
		auto& pEnemyObjects = ((shared_ptr<PlayScene>&)GameFramework::Instance().GetCurrentScene())->GetObjectsLayered(WORLD_OBJ_LAYER::ENEMY);
		for (auto& pEnemy : pEnemyObjects) {
			if (pEnemy && pEnemy != shared_from_this() && CheckCollision(pEnemy)) {	// 자기 자신은 제외
				XMFLOAT3 SelfToOther = Vector3::Subtract(pEnemy->GetWorldPosition(), GetWorldPosition());
				SelfToOther = Vector3::ScalarProduct(Vector3::Normalize(SelfToOther), 0.1f);
				pEnemy->Move(SelfToOther);
				pEnemy->UpdateObject();
				Move(Vector3::ScalarProduct(SelfToOther, -1));
				UpdateObject();
			}
		}
	}
	
	XMFLOAT3 localPos = pRootParent->GetLocalPosition();	// 나의 loaclPos == worldPos
	if (localPos.x < 0.f || 2000.f < localPos.x || localPos.z < 0.f || 2000.f < localPos.z) {
		localPos.x = clamp(localPos.x, 0.f, 2000.f);
		localPos.z = clamp(localPos.z, 0.f, 2000.f);
		pRootParent->SetLocalPosition(localPos);
		pRootParent->UpdateObject();
	}

	Gunship::Animate(_timeElapsed);
}

void GunshipEnemy::Attacked(float _dmg) {
	Unit::Attacked(_dmg);
	if (hp <= 0) {
		VS_ParticleMappedFormat particle;
		particle.boardSize = { 10,10 };
		particle.lifetime = 0.8f;
		particle.position = GetWorldPosition();
		particle.type = 0;
		for (int i = 0; i < 10; ++i) {
			particle.velocity = XMFLOAT3(rand() % 200 - 100, rand() % 200 - 100, rand() % 200 - 100);
			Shader::AddParticle(particle);
		}

		DeleteMe();
	}
		
}
