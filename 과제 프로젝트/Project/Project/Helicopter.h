#pragma once
#include "GameObject.h"

class Helicopter : public GameObject {
protected:
	float hSpeed;
	float vSpeed;
	float rSpeed;

public:
	Helicopter();
	virtual ~Helicopter();

	virtual void Create();

	virtual void MoveHorizontal(XMFLOAT3 _dir, float _timeElapsed);
	virtual void MoveVertical(bool _up, float _timeElapsed);
	virtual void Animate(double _timeElapsed);
};

