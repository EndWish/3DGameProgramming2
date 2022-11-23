#pragma once
#include "GameObject.h"
#include "Apache.h"
#include "GunshipMissile.h"
#include "Camera.h"

class Player : public Apache {
private:
	shared_ptr<Camera> pCamera;

public:
	Player();
	virtual ~Player();

public:
	shared_ptr<Camera> GetCamera() const;

	virtual void Create();

	bool TryFireMissile();
};

