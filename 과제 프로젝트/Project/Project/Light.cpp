#include "stdafx.h"
#include "Light.h"
#include "GameObject.h"


Light::Light(const shared_ptr<GameObject>& _object) {
	ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);

	// object에 연결 후 position값 삭제. object의 위치+offset값을 position으로 넘김
	position = XMFLOAT3(0.0f, 10.f, -0.0f);
	range = 15;

	offset = XMFLOAT3(0.0f, 0.0f, 0.0f);	
	theta = 0;
	phi = 0;
	attenuation = XMFLOAT3(1.0f, 0.05f, 0.01f);	
	direction = XMFLOAT3(0.0f, -1.0f, 0.0f);
	falloff = 0;

	object = _object;

	// 1 = 점, 2 = 스포트, 3 = 직접
	lightType = 3;

	// 이 빛이 켜져있는 상태인지 확인
	enable = true;
}

Light::~Light() {

}

