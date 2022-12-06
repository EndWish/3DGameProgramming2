#include "stdafx.h"
#include "GunshipMissile.h"
#include "GameFramework.h"
#include "Unit.h"
#include "GunshipEnemy.h"

GunshipMissile::GunshipMissile(float _speed, float _maxSpeed , float _acceleration) {
	speed = _speed;
	maxSpeed = _maxSpeed;
	acceleration = _acceleration;
	lifeTime = 3.f;

	smokeGenerateMaxCoolTime = 0.05f;
	smokeGenerateCoolTime = 0.f;

	targetLayer = WORLD_OBJ_LAYER::NONE;
	attackType = 0;	// 0: 일직선, 1: 유도
}
GunshipMissile::~GunshipMissile() {

}
void GunshipMissile::Create() {
	GameObject::Create();
	SetMesh( GameObjectManager::GetGameObject("GunshipMissile")->GetMesh(), OOBB_TYPE::MESHOOBB);
	UpdateOOBB();
}

void GunshipMissile::SetTargetLayer(WORLD_OBJ_LAYER _layer) {
	targetLayer = _layer;
}

void GunshipMissile::SetAttackType(int _attackType) {
	attackType = _attackType;
}

void GunshipMissile::Animate(float _timeElapsed) {
	lifeTime -= _timeElapsed;
	if (lifeTime <= 0) {
		DeleteMe();
		return;
	}

	// 미사일 이동
	speed = min(speed + acceleration * _timeElapsed, maxSpeed);

	if(attackType == 1) {
		auto pTarget = wpTarget.lock();
		if (pTarget) {
			XMFLOAT3 targetPos = pTarget->GetWorldPosition();
			XMFLOAT3 toTarget = Vector3::Subtract(targetPos, GetWorldPosition());
			XMFLOAT3 lookVector = GetWorldLookVector();
			
			XMFLOAT3 axis = Vector3::Cross(lookVector, toTarget);
			float minAngle = Vector3::Angle(lookVector, toTarget, false);
			if (abs(axis.y) <= numeric_limits<float>::epsilon()) {	// 외적이 불가능한 경우 (두 벡터가 평행한 경우)
				axis = XMFLOAT3(0, 1, 0);
			}
			localRotation = Vector4::QuaternionMultiply(localRotation, Vector4::QuaternionRotation(axis, min(360.f * _timeElapsed, minAngle)));
		}
		else {
			if (targetLayer != WORLD_OBJ_LAYER::NONE) {
				auto& pObjects = ((shared_ptr<PlayScene>&)GameFramework::Instance().GetCurrentScene())->GetObjectsLayered(targetLayer);
				for (auto pObject : pObjects) {
					if (pObject && Vector3::Distance2(pObject->GetWorldPosition(), GetWorldPosition()) <= 250000.f) {
						wpTarget = pObject;
						return;
					}
				}
			}
		}
	}
	MoveFront(speed * _timeElapsed);
	UpdateObject();

	// 연기 생성
	smokeGenerateCoolTime -= _timeElapsed;
	if (smokeGenerateCoolTime <= 0) {
		smokeGenerateCoolTime = smokeGenerateMaxCoolTime;
		VS_ParticleMappedFormat particle;
		particle.boardSize = { 8, 8 };
		particle.lifetime = 2.f;
		particle.position = GetWorldPosition();
		particle.type = PARTICLE_TYPE_SMOKE;
		particle.velocity = { 0,0,0 };
		Shader::AddParticle(particle);
	}

	GameObject::Animate(_timeElapsed);
}
void GunshipMissile::ProcessCollision(float _timeElapsed) {
	if (targetLayer != WORLD_OBJ_LAYER::NONE) {
		auto& pObjects = ((shared_ptr<PlayScene>&)GameFramework::Instance().GetCurrentScene())->GetObjectsLayered(targetLayer);
		for (auto pObject : pObjects) {
			if (pObject && CheckCollision(pObject)) {
				static_pointer_cast<GunshipEnemy>(pObject)->Attacked(100.f);
				DeleteMe();;
				return;
			}
		}
	}
}
