#pragma once
class Unit {
protected:
	float hp;
	float hpm;

public:
	Unit();
	~Unit();

	// get set ÇÔ¼ö
	virtual void Attacked(float _dmg);
	bool IsDead();

};

