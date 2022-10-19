#pragma once
#include "Helicopter.h"

class Gunship : public Helicopter {
protected:
	shared_ptr<GameObject> pRotor, pBackRotor;

public:
	Gunship();
	virtual ~Gunship();


	virtual void Create();
	virtual void Animate(double _timeElapsed);

};

