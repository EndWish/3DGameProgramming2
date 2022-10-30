#pragma once
#include "GameObject.h"
#include "Unit.h"

class Helicopter : public GameObject, public Unit {
protected:
	float hSpeed;
	float vSpeed;
	float rSpeed;

public:
	Helicopter();
	virtual ~Helicopter();

	virtual void MoveHorizontal(XMFLOAT3 _dir, float _timeElapsed);
	virtual void MoveVertical(bool _up, float _timeElapsed);
	virtual void Animate(float _timeElapsed);
};

