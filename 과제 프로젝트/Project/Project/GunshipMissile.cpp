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
	targetLayer = WORLD_OBJ_LAYER::NONE;
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

void GunshipMissile::Animate(float _timeElapsed) {
	lifeTime -= _timeElapsed;
	if (lifeTime <= 0) {
		DeleteMe();
		return;
	}

	speed = min(speed + acceleration * _timeElapsed, maxSpeed);
	MoveFront(speed * _timeElapsed);
	UpdateObject();
	GameObject::Animate(_timeElapsed);
}
void GunshipMissile::ProcessCollision(float _timeElapsed) {
	if (targetLayer != WORLD_OBJ_LAYER::NONE) {
		auto& pObjects = ((shared_ptr<PlayScene>&)GameFramework::Instance().GetCurrentScene())->GetObjectsLayered(targetLayer);
		for (auto pObject : pObjects) {
			if (pObject && CheckCollision(pObject)) {
				static_pointer_cast<GunshipEnemy>(pObject)->Attacked(100.f);
				DeleteMe();
				cout << "Ãæµ¹\n";
				return;
			}
		}
	}
}
