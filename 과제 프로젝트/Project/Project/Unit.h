#pragma once
class Unit {
protected:
	float hp;
	float hpm;

public:
	Unit();
	~Unit();

	// get set �Լ�
	virtual void Attacked(float _dmg);
	bool IsDead();

};

