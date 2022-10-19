#pragma once

class GameObject;

class Light
{
public:
	// �� �׵�
	XMFLOAT4 ambient;
	XMFLOAT4 diffuse;
	XMFLOAT4 specular;

	XMFLOAT3 position;
	float range;

	XMFLOAT3 offset;	// ���� ���� ��ü�� �߽����κ��� ������ ��
	float theta; // �ܺ� ���� �׸��� ��, ����Ʈ����Ʈ���� ���
	XMFLOAT3 attenuation;
	float phi; // ���� ���� �׸��� ��, ����Ʈ����Ʈ���� ���
	XMFLOAT3 direction;
	float falloff;	// phi�� theta�� ���� ���� ����

	// �� ���� ���� �ִ� ������Ʈ�� ������
private:
	weak_ptr<GameObject> object;

public:
	// 1 = ��, 2 = ����Ʈ, 3 = ����
	int lightType;

	// �� ���� �����ִ� �������� Ȯ��
	bool enable;
	XMFLOAT2 padding2;
	// �е� ��Ģ �ؼ�
public:
	Light(const shared_ptr<GameObject>& _object);
	~Light();

};


struct LightsMappedFormat {
	array<Light, MAX_LIGHTS> lights;
	XMFLOAT4 globalAmbient;
	int nLight;
};