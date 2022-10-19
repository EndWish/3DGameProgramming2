#pragma once
#include "GameObject.h"
#include "Gunship.h"
#include "Camera.h"

class Player : public Gunship {
private:
	shared_ptr<Camera> pCamera;

public:
	Player();
	virtual ~Player();

public:
	shared_ptr<Camera> GetCamera() const;

	virtual void Create();

};

