#pragma once
#include "GameObject.h"

class Helicopter : public GameObject {
protected:
	float hSpeed = 300.f;
	float vSpeed = 100.f;
	float rSpeed = 720.f;

public:
	Helicopter();
	virtual ~Helicopter();

	virtual void Create();

	virtual void MoveHorizontal(XMFLOAT3 _dir, float _timeElapsed);
	virtual void MoveVertical(bool _up, float _timeElapsed);

};

