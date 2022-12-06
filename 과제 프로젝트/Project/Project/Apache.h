#pragma once
#include "Helicopter.h"

class Apache : public Helicopter {
protected:
	shared_ptr<GameObject> pRotor, pBackRotor[2];
	float missileMaxCoolTime, missileCoolTime;
	float missileRainMaxCoolTime, missileRainCoolTime;

public:
	Apache();
	virtual ~Apache();

	virtual void Create();
	virtual void Animate(float _timeElapsed);

	bool IsCoolDown();
	bool IsRainCoolDown();
};