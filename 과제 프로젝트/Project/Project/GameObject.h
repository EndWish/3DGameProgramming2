#pragma once
#include "Mesh.h"

class Light;

enum OOBB_TYPE : char {
	DISABLED,
	DISABLED_FINAL,
	MESHOOBB,
	MESHOOBB_FINAL,
	PRIVATEOOBB,
	PRIVATEOOBB_FINAL,
	PRIVATEOOBB_COVER,
};
bool IsOOBBTypeFinal(OOBB_TYPE _OOBBType);
bool IsOOBBTypeDisabled(OOBB_TYPE _OOBBType);

class GameObject : public enable_shared_from_this<GameObject> {
protected:
	static array<XMFLOAT3, 8> cube;
	static array<UINT, 36> cubeIndex;
public:
	static shared_ptr<GameObject> LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
protected:
	// ���� GameObject�� ������ �߰��ߴٸ�, ���� �����ڵ� �����ض�

	string name;

	// ������ǥ�� ���� : eachTransform �� �ٲ�� �׻� ����ȭ ���ش�.
	XMFLOAT4X4 worldTransform;
	

	// �θ���ǥ�� ����
	XMFLOAT4X4 localTransform;
	XMFLOAT3 localPosition;
	XMFLOAT3 localScale;
	XMFLOAT4 localRotation;

	// ��ü�� ������ �ִ� ���� ������
	shared_ptr<Light> pLight;

	OOBB_TYPE OOBBType;
	shared_ptr<BoundingOrientedBox> pPrivateOOBB;
	BoundingOrientedBox boundingBox;
	
	weak_ptr<GameObject> wpParent;
	vector<shared_ptr<GameObject>> pChildren;

	shared_ptr<Mesh> pMesh;

public:
	GameObject();
	virtual ~GameObject();
	virtual void Create();
	
	// get set �Լ�
	// Get�̸�
	const string& GetName() const;

	// �θ���ǥ����� ���͵��� ��´�.
	XMFLOAT3 GetLocalRightVector() const;
	XMFLOAT3 GetLocalUpVector() const;
	XMFLOAT3 GetLocalLookVector() const;
	XMFLOAT4 GetLocalRotate() const;
	XMFLOAT3 GetLocalPosition() const;

	// ���� �̵�
	void MoveRight(float distance);
	void Move(const XMFLOAT3& _moveVector);
	void MoveUp(float distance);
	void MoveFront(float distance);

	// ���� ȸ��
	void Rotate(const XMFLOAT3& _axis, float _angle);
	void Rotate(const XMFLOAT4& _quat);
	virtual void Revolve(const XMFLOAT3& _axis, float _angle);	// ����
	virtual void SynchronousRotation(const XMFLOAT3& _axis, float _angle);	//���ֱ� ����
	
	// ������ǥ�� ���� �ڽ��� ��ġ�� �����Ѵ�.
	XMFLOAT3 GetWorldPosition() const;
	XMFLOAT3 GetWorldRightVector() const;
	XMFLOAT3 GetWorldUpVector() const;
	XMFLOAT3 GetWorldLookVector() const;

	OOBB_TYPE GetOOBBType() const;
	const BoundingOrientedBox& GetBoundingBox() const;	// �ڽ��� �ٿ�� �ڽ��� ���۷����� �����Ѵ�.
	shared_ptr<GameObject> GetChild(const string& _name) const;	// �ڽ��� ã�� �����Ѵ�.
	shared_ptr<GameObject> GetRootParent();	// ��Ʈ ������ �����Ѵ�.

	// �̸� ����
	void SetName(const string& _name);
	
	void SetLocalPosition(const XMFLOAT3& _position);	// ��ġ�� ������ �̵���Ų��.
	void SetLocalRotation(const XMFLOAT4& _rotation);	// Ư�� ȸ������ �����Ѵ�.
	void SetLocalScale(const XMFLOAT3& _scale);	// Ư�� Scale���� �����Ѵ�.

	void SetChild(const shared_ptr<GameObject> _pChild);	// �ڽ��� �߰��Ѵ�.
	void SetMesh(const shared_ptr<Mesh>& _pMesh, OOBB_TYPE _OOBBType);	// �޽��� �����Ѵ�.
	void SetPrivateOOBB(const shared_ptr<BoundingOrientedBox>& _pPrivateOOBB, OOBB_TYPE _OOBBType);	// ����OOBB�� �����Ѵ�.
	void SetOOBBType(OOBB_TYPE _OOBBType);

	void UpdateLocalTransform();
	virtual void UpdateWorldTransform();
	void UpdateWorldTransformAll();	// localTransform�� �̿��Ͽ� worldTransform�� �ֽ�ȭ�ϰ� �ڽİ�ü������ ��������� ȣ��
	void UpdateOOBB();
	void UpdateOOBBAll();		// OOBB�� �ֽ�ȭ �ϰ� �ڽĵ鵵 �Ȱ��� �Ѵ�.
	void UpdateObject();	// �� 3���� update�� �ѹ��� �Ѵ�.

	// �浹 üũ
	bool CheckCollision(const shared_ptr<GameObject>& _other) const;

	// �ִϸ��̼�
	virtual void Animate(double _timeElapsed);

	// ����
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	void RenderHitBox(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, HitBoxMesh& _hitBox);
	// ���� ��ȯ����� ���̴��� �Ѱ��ش�.
	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	void UpdateHitboxShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	
	void CopyObject(const GameObject& _other);
};

class GameObjectManager {
private:
	static unordered_map<string, shared_ptr<GameObject>> storage;

public:
	static bool LoadFromFile(const string& _name, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	static void ReleaseObject(const string& _name);
	static void ReleaseAllObject();
	static shared_ptr<GameObject> GetGameObject(const string& _name, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	static bool AddObject(shared_ptr<GameObject> _newObject);
};

