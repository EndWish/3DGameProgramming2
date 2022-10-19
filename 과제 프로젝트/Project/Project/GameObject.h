#pragma once
#include "Mesh.h"

class Light;


class GameObject : public enable_shared_from_this<GameObject> {
protected:
	static array<XMFLOAT3, 8> cube;
	static array<UINT, 36> cubeIndex;
public:
	static shared_ptr<GameObject> LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
protected:
	// 만약 GameObject에 변수를 추가했다면, 복사 생성자도 수정해라

	string name;

	// 월드좌표계 기준 : eachTransform 이 바뀌면 항상 동기화 해준다.
	XMFLOAT4X4 worldTransform;
	

	// 부모좌표계 기준
	XMFLOAT4X4 localTransform;
	XMFLOAT3 localPosition;
	XMFLOAT3 localScale;
	XMFLOAT4 localRotation;

	// 물체가 가지고 있는 빛의 포인터
	shared_ptr<Light> pLight;

	BoundingOrientedBox boundingBox;
	// true일경우 하위 오브젝트들을 모두 포함하는 바운딩박스 객체임
	bool isOOBBBCover;

	weak_ptr<GameObject> pParent;
	vector<shared_ptr<GameObject>> pChildren;

	shared_ptr<Mesh> pMesh;

public:
	GameObject();
	virtual ~GameObject();
	// 게임 오브젝트 복사 생성자
	virtual void Create();
	
	// get set 함수
	const string& GetName() const;
	// 부모좌표계기준 벡터들을 얻는다.
	XMFLOAT3 GetLocalRightVector() const;
	XMFLOAT3 GetLocalUpVector() const;
	XMFLOAT3 GetLocalLookVector() const;
	XMFLOAT4 GetLocalRotate() const;
	XMFLOAT3 GetLocalPosition() const;
	// 로컬 이동
	void MoveRight(float distance);
	void Move(const XMFLOAT3& _moveVector);
	void MoveUp(float distance);
	void MoveFront(float distance);

	void Rotate(const XMFLOAT3& _axis, float _angle);
	void Rotate(const XMFLOAT4& _quat);

	virtual void Revolve(const XMFLOAT3& _axis, float _angle);
	virtual void SynchronousRotation(const XMFLOAT3& _axis, float _angle);
	// 월드좌표계 기준 자신의 위치를 리턴한다.
	XMFLOAT3 GetWorldPosition() const;
	XMFLOAT3 GetWorldRightVector() const;
	XMFLOAT3 GetWorldUpVector() const;
	XMFLOAT3 GetWorldLookVector() const;

	// 자신의 바운딩 박스의 래퍼런스를 리턴한다.
	const BoundingOrientedBox& GetBoundingBox() const;
	// 자식을 찾아 리턴한다.
	shared_ptr<GameObject> GetChild(const string& _name) const;
	// 
	XMFLOAT3 GetCollisionNormal(const shared_ptr<GameObject>& _other);

	// 위치를 강제로 이동시킨다.
	void SetLocalPosition(const XMFLOAT3& _position);
	// 특정 회전값을 대입한다.
	void SetLocalRotation(const XMFLOAT4& _rotation);
	// 특정 Scale값을 대입한다.
	void SetLocalScale(const XMFLOAT3& _scale);


	// 자식을 추가한다.
	void SetChild(const shared_ptr<GameObject> _pChild);
	// 메쉬를 설정한다.
	void SetMesh(const shared_ptr<Mesh>& _pMesh);

	void UpdateLocalTransform();
	// eachTransform를 가지고 worldTransform를 업데이트 한다.
	virtual void UpdateWorldTransform();
	// 변환행렬을 적용하고 worldTransform을 업데이트 한다.

	//  OOBB 갱신
	void UpdateOOBB();

	// 오브젝트 내용 전체적으로 갱신
	void UpdateObject();

	// 충돌 체크
	pair<shared_ptr<GameObject>, shared_ptr<GameObject>> CheckCollision(const shared_ptr<GameObject>& _other);

	// 애니메이션
	virtual void Animate(double _timeElapsed);

	// 렌더
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	void RenderHitBox(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, HitBoxMesh& _hitBox);
	// 월드 변환행렬을 쉐이더로 넘겨준다.
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

};

