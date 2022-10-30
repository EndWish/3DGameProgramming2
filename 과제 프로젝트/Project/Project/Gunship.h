#pragma once
#include "Helicopter.h"
#include "GunshipMissile.h"

class Gunship : public Helicopter {
protected:
	shared_ptr<GameObject> pRotor, pBackRotor;
	float missileMaxCoolTime;
	float missileCoolTime;

public:
	Gunship();
	virtual ~Gunship();

	virtual void Create();
	virtual void Animate(float _timeElapsed);

	bool IsCoolDown();
};

