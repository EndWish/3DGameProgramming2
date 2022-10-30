#include "stdafx.h"
#include "Unit.h"

Unit::Unit() {
	hp = 100.f;
	hpm = 100.f;
}
Unit::~Unit() {

}

void Unit::Attacked(float _dmg) {
	hp -= _dmg;
}
bool Unit::IsDead() {
	return hp <= 0;
}
