#pragma once
#include "GameObject.h"

class GunshipMissile : public GameObject {
protected:
	float acceleration;
	float speed;
	float maxSpeed;
	float lifeTime;
	WORLD_OBJ_LAYER targetLayer;

public:
	GunshipMissile(float _speed, float _maxSpeed, float _acceleration);
	virtual ~GunshipMissile();
	virtual void Create();

	// get set ÇÔ¼ö
	void SetTargetLayer(WORLD_OBJ_LAYER _layer);

	virtual void Animate(float _timeElapsed);
	virtual void ProcessCollision(float _timeElapsed);
	
};

