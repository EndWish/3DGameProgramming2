#pragma once
#include "GameObject.h"
#include "Apache.h"
#include "GunshipMissile.h"
#include "Camera.h"

class Player : public Apache {
private:
	shared_ptr<Camera> pCamera;
	int nReloadedMissileRain;
	float missileRainMaxTerm, missileRainTerm;

public:
	Player();
	virtual ~Player();

public:
	shared_ptr<Camera> GetCamera() const;

	virtual void Create();
	virtual void Animate(float _timeElapsed);

	bool TryFireMissile();
	bool TryFireMissileRain();
};

