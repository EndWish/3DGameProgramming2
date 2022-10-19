#pragma once

class GameObject;

class Light
{
public:
	// 각 항들
	XMFLOAT4 ambient;
	XMFLOAT4 diffuse;
	XMFLOAT4 specular;

	XMFLOAT3 position;
	float range;

	XMFLOAT3 offset;	// 빛을 내는 물체의 중심으로부터 떨어진 값
	float theta; // 외부 원을 그리는 각, 스포트라이트에서 사용
	XMFLOAT3 attenuation;
	float phi; // 내부 원을 그리는 각, 스포트라이트에서 사용
	XMFLOAT3 direction;
	float falloff;	// phi와 theta에 대한 감쇠 비율

	// 이 빛을 내고 있는 오브젝트의 포인터
private:
	weak_ptr<GameObject> object;

public:
	// 1 = 점, 2 = 스포트, 3 = 직접
	int lightType;

	// 이 빛이 켜져있는 상태인지 확인
	bool enable;
	XMFLOAT2 padding2;
	// 패딩 규칙 준수
public:
	Light(const shared_ptr<GameObject>& _object);
	~Light();

};


struct LightsMappedFormat {
	array<Light, MAX_LIGHTS> lights;
	XMFLOAT4 globalAmbient;
	int nLight;
};