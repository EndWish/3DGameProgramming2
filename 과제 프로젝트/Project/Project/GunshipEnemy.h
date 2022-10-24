#pragma once
#include "Gunship.h"
class GunshipEnemy : public Gunship {
protected:
	weak_ptr<GameObject> wpTarget;

public:
	//get set �Լ�
	void SetTarget(shared_ptr<GameObject> _target);

	// Animation
	virtual void Animate(double _timeElapsed);

};

