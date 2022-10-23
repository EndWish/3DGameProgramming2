#include "stdafx.h"
#include "GameObject.h"
#include "Light.h"
#include "GameFramework.h"

//////////////////////////////////////////

array<XMFLOAT3, 8> GameObject::cube{
	XMFLOAT3(-0.5f,  -0.5f,  -0.5f),
	XMFLOAT3(-0.5f,  -0.5f,  +0.5f),
	XMFLOAT3(+0.5f,  -0.5f,  +0.5f),
	XMFLOAT3(+0.5f,  -0.5f,  -0.5f),
	XMFLOAT3(-0.5f,  +0.5f,  -0.5f),
	XMFLOAT3(-0.5f,  +0.5f,  +0.5f),
	XMFLOAT3(+0.5f,  +0.5f,  +0.5f),
	XMFLOAT3(+0.5f,  +0.5f,  -0.5f), };
array<UINT, 36> GameObject::cubeIndex{
	0,3,2, 0,2,1,
	4,5,6, 4,6,7,
	0,4,7, 0,7,3,
	1,2,6, 1,6,5,
	0,1,5, 0,5,4,
	2,3,7, 2,7,6,
};

shared_ptr<GameObject> GameObject::LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	TexturePicker::UseCountUp();

	// ���θ��� ������Ʈ�� �޸𸮰����� �Ҵ��Ѵ�.
	shared_ptr<GameObject> pNewObject = make_shared<GameObject>();

	// nameSize (UINT) / name(string)
	ReadStringBinary(pNewObject->name, _file);

	// localTransform(float4x4)
	_file.read((char*)&pNewObject->localPosition, sizeof(XMFLOAT3));
	_file.read((char*)&pNewObject->localScale, sizeof(XMFLOAT3));
	_file.read((char*)&pNewObject->localRotation, sizeof(XMFLOAT4));
	pNewObject->UpdateLocalTransform();

	// �޽��� �о�´�.
	pNewObject->pMesh = BasicMesh::LoadFromFile(_file, _pDevice, _pCommandList);

	//
	int nChildren;
	_file.read((char*)&nChildren, sizeof(int));
	pNewObject->pChildren.assign(nChildren, {});

	for (int i = 0; i < nChildren; ++i) {
		pNewObject->pChildren[i] = GameObject::LoadFromFile(_file, _pDevice, _pCommandList);
	}
	
	TexturePicker::UseCountDown();
	return pNewObject;
}

GameObject::GameObject() {
	name = "unknown";
	worldTransform = Matrix4x4::Identity();
	localTransform = Matrix4x4::Identity();
	localPosition = XMFLOAT3(0, 0, 0);
	localRotation = XMFLOAT4(0, 0, 0, 1);
	localScale = XMFLOAT3(1,1,1);
	boundingBox = BoundingOrientedBox();
	isOOBBBCover = false;
}
GameObject::~GameObject() {

}

void GameObject::Create() {

}

const string& GameObject::GetName() const {
	return name;
}

XMFLOAT3 GameObject::GetLocalRightVector() const {
	XMFLOAT3 rightVector = XMFLOAT3(1, 0, 0);
	rightVector = Vector3::Transform(rightVector, Matrix4x4::RotateQuaternion(localRotation));
	return rightVector;
}
XMFLOAT3 GameObject::GetLocalUpVector() const {
	XMFLOAT3 rightVector = XMFLOAT3(0, 1, 0);
	rightVector = Vector3::Transform(rightVector, Matrix4x4::RotateQuaternion(localRotation));
	return rightVector;
}
XMFLOAT3 GameObject::GetLocalLookVector() const {
	XMFLOAT3 rightVector = XMFLOAT3(0, 0, 1);
	rightVector = Vector3::Transform(rightVector, Matrix4x4::RotateQuaternion(localRotation));
	return rightVector;
}
XMFLOAT3 GameObject::GetLocalPosition() const {
	return localPosition;
}
XMFLOAT4 GameObject::GetLocalRotate() const {
	return localRotation;
}

void GameObject::MoveRight(float distance) {
	XMFLOAT3 moveVector = GetLocalRightVector();	// RightVector�� �����ͼ�
	moveVector = Vector3::Normalize(moveVector);	// �������ͷ� �ٲ���
	moveVector = Vector3::ScalarProduct(moveVector, distance);	// �̵��Ÿ���ŭ �����ش�.
	localPosition = Vector3::Add(localPosition, moveVector);
}
void GameObject::Move(const XMFLOAT3& _moveVector) {
	localPosition = Vector3::Add(localPosition, _moveVector);
}
void GameObject::MoveUp(float distance) {
	XMFLOAT3 moveVector = GetLocalUpVector();	// UpVector�� �����ͼ�
	moveVector = Vector3::Normalize(moveVector);	// �������ͷ� �ٲ���
	moveVector = Vector3::ScalarProduct(moveVector, distance);	// �̵��Ÿ���ŭ �����ش�.
	localPosition = Vector3::Add(localPosition, moveVector);
}
void GameObject::MoveFront(float distance) {
	XMFLOAT3 moveVector = GetLocalLookVector();	// LookVector�� �����ͼ�
	moveVector = Vector3::Normalize(moveVector);	// �������ͷ� �ٲ���
	moveVector = Vector3::ScalarProduct(moveVector, distance);	// �̵��Ÿ���ŭ �����ش�.
	localPosition = Vector3::Add(localPosition, moveVector);
}
void GameObject::Rotate(const XMFLOAT3& _axis, float _angle) {
	localRotation = Vector4::QuaternionMultiply(localRotation, Vector4::QuaternionRotation(_axis, _angle));
}
void GameObject::Rotate(const XMFLOAT4& _quat) {
	localRotation = Vector4::QuaternionMultiply(localRotation, _quat);
}

void GameObject::Revolve(const XMFLOAT3& _axis, float _angle) {
	XMFLOAT4X4 rotationMatrix = Matrix4x4::RotationAxis(_axis, _angle);
	localPosition = Vector3::Transform(localPosition, rotationMatrix);
}
void GameObject::SynchronousRotation(const XMFLOAT3& _axis, float _angle) {
	Revolve(_axis, _angle);
	localRotation = Vector4::QuaternionMultiply(localRotation, Vector4::QuaternionRotation(_axis, _angle));
}

XMFLOAT3 GameObject::GetWorldRightVector() const {
	return Vector3::Normalize(worldTransform._11, worldTransform._12, worldTransform._13);
}
XMFLOAT3 GameObject::GetWorldUpVector() const {
	return Vector3::Normalize(worldTransform._21, worldTransform._22, worldTransform._23);
}
XMFLOAT3 GameObject::GetWorldLookVector() const {
	return Vector3::Normalize(worldTransform._31, worldTransform._32, worldTransform._33);
}
XMFLOAT3 GameObject::GetWorldPosition() const {
	return XMFLOAT3(worldTransform._41, worldTransform._42, worldTransform._43);
}

const BoundingOrientedBox& GameObject::GetBoundingBox() const {
	return boundingBox;
}

shared_ptr<GameObject> GameObject::GetChild(const string& _name) const {
	for (auto pChild : pChildren) {
		cout << pChild->name << "\n";
		if (pChild->name == _name)
			return pChild;
		else {
			auto result = pChild->GetChild(_name);
			if (result)
				return result;
		}
	}
	return nullptr;
}

XMFLOAT3 GameObject::GetCollisionNormal(const shared_ptr<GameObject>& _other) {
	XMFLOAT3 toOther = Vector3::Subtract(_other->GetWorldPosition(), GetWorldPosition());
	float dist = Vector3::Distance(toOther);

	// �ﰢ���� ��ȯ��ų ����� �����.
	XMFLOAT4X4 triangleTransform = Matrix4x4::ScaleTransform(XMFLOAT3(1,1,1));	// S
	triangleTransform = Matrix4x4::Multiply(triangleTransform, Matrix4x4::ScaleTransform(_other->GetBoundingBox().Extents));	// S
	triangleTransform = Matrix4x4::Multiply(triangleTransform, Matrix4x4::RotateQuaternion(_other->GetBoundingBox().Orientation));	// R
	triangleTransform = Matrix4x4::Multiply(triangleTransform, Matrix4x4::MoveTransform(_other->GetWorldPosition())); // T

	for (int i = 0; i < 12; ++i) {
		// _other�� �浹�ڽ� ���� �ﰢ���� �����.
		array<XMFLOAT3, 3> triangle{ cube[cubeIndex[i * 3]], cube[cubeIndex[i * 3 + 1]], cube[cubeIndex[i * 3 + 2]] };
		for (XMFLOAT3& point : triangle) {
			point = Vector3::Transform(point, triangleTransform);
		}
		XMFLOAT3 triangleNormal = Vector3::Cross(triangle[0], triangle[1], triangle[2]);
		// �ﰢ���� ������ ���������� �������� �ٶ󺸴� ������ �ȴ�.
		if (Vector3::Dot(toOther, triangleNormal) < 0) {
			// �ٶ󺸴� ���϶� �ﰢ���� ������ �浹�� ��� �ﰢ���� �������͸� �����Ѵ�.
			XMFLOAT3 position = GetWorldPosition();
			if (TriangleTests::Intersects(XMLoadFloat3(&position),	// ������
				XMLoadFloat3(&toOther),	// ����
				XMLoadFloat3(&triangle[0]),	// �ﰢ���� �� ������
				XMLoadFloat3(&triangle[1]),	// �ﰢ���� �� ������
				XMLoadFloat3(&triangle[2]),	// �ﰢ���� �� ������
				dist)) {
				cout << Vector3::Normalize(triangleNormal) << "\n";
				return Vector3::Normalize(triangleNormal);
			}
			else {
				cout << "#�ﰢ���� ������ ����\n";
			}
		}
		else {
			cout << "#�ٶ󺸴� ������ �ƴ�\n";
		}
	}
	cout << "����\n";
	return XMFLOAT3(0, 0, 0);
}

void GameObject::SetName(const string& _name) {
	name = _name;
}

void GameObject::SetLocalPosition(const XMFLOAT3& _position) {
	localPosition = _position;
}

void GameObject::SetLocalRotation(const XMFLOAT4& _rotation) {
	localRotation = _rotation;
}

void GameObject::SetLocalScale(const XMFLOAT3& _scale) {
	localScale = _scale;
}


void GameObject::SetChild(const shared_ptr<GameObject> _pChild) {
	// �Ծ��� ���̰�, �θ� ���� ��� �θ�� ���� ������Ų��.
	if (auto pPreParent = _pChild->pParent.lock()) {
		pPreParent->pChildren.erase(ranges::find(pPreParent->pChildren, _pChild));
	}

	// ���� �ڽ����� �Ծ�
	pChildren.push_back(_pChild);

	// �ڽ��� �θ� ���� ����
	_pChild->pParent = shared_from_this();
}



void GameObject::SetMesh(const shared_ptr<Mesh>& _pMesh) {
	pMesh = _pMesh;
}

void GameObject::UpdateLocalTransform() {
	localTransform = Matrix4x4::Identity();
	// S

	localTransform._11 = localScale.x;
	localTransform._22 = localScale.y;
	localTransform._33 = localScale.z;
	// SxR
	localTransform = Matrix4x4::Multiply(localTransform, Matrix4x4::RotateQuaternion(localRotation));
	// xT
	localTransform._41 = localPosition.x;
	localTransform._42 = localPosition.y;
	localTransform._43 = localPosition.z;
}
void GameObject::UpdateWorldTransform() {
	//UpdateLocalTransform();

	if (auto pParentLock = pParent.lock()) {	// �θ� ���� ���
		worldTransform = Matrix4x4::Multiply(localTransform, pParentLock->worldTransform);
	}
	else {	// �θ� ���� ���
		worldTransform = localTransform;
	}

	// �ڽĵ鵵 worldTransform�� ������Ʈ ��Ų��.
	for (auto& pChild : pChildren) {
		pChild->UpdateWorldTransform();
	}
}



void GameObject::UpdateOOBB() {
	if (pMesh) {
		// Mesh�� OOBB�� ���� ������Ʈ�� transform������ ��ȯ
		pMesh->GetOOBB().Transform(boundingBox, XMLoadFloat4x4(&worldTransform));
		// OOBB�� ȸ������ ����ȭ
		XMStoreFloat4(&boundingBox.Orientation, XMQuaternionNormalize(XMLoadFloat4(&boundingBox.Orientation)));
	}
	for (const auto& pChild : pChildren) {
		pChild->UpdateOOBB();
	}
}

void GameObject::UpdateObject() {
	UpdateLocalTransform();
	UpdateWorldTransform();
	UpdateOOBB();
}

pair<shared_ptr<GameObject>, shared_ptr<GameObject>> GameObject::CheckCollision(const shared_ptr<GameObject>& _other) {

	if (pMesh) {
		if (_other->pMesh && boundingBox.Intersects(_other->boundingBox)) {
			cout << pMesh->GetName() << ", " << _other->pMesh->GetName() << "�浹!!\n";
			return { shared_from_this(), _other };
		}

	}
	for (const auto& pChild : _other->pChildren) {
		auto result = CheckCollision(pChild);
		// �浹 ���� ��� nullptr�� �ƴ� ������ pair�� ���ϵȴ�.
		if (result.first && result.second) {
			return result;
		}
	}
	for (const auto& pChild : pChildren) {
		auto result = pChild->CheckCollision(_other);
		if (result.first && result.second) {
			return result;
		}
	}
	return {nullptr, nullptr};
}

void GameObject::Animate(double _timeElapsed) {


	for (const auto& pChild : pChildren) {
		pChild->Animate(_timeElapsed);
	}
}

void GameObject::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	if (pMesh) {	// �޽��� ���� ��쿡�� �������� �Ѵ�.
		UpdateShaderVariable(_pCommandList);
		// ����� ���̴��� �׷��Ƚ� ������������ �����Ѵ� [�������]
		
		pMesh->Render(_pCommandList);
	}
	for (const auto& pChild : pChildren) {
		pChild->Render(_pCommandList);
	}

}

void GameObject::RenderHitBox(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, HitBoxMesh& _hitBox) {
	
	if (pMesh) {	// �޽��� ���� ��쿡�� �������� �Ѵ�.
		UpdateHitboxShaderVariable(_pCommandList);
		// ����� ���̴��� �׷��Ƚ� ������������ �����Ѵ� [�������]
		_hitBox.Render(_pCommandList);
	}
	for (const auto& pChild : pChildren) {
		pChild->RenderHitBox(_pCommandList, _hitBox);
	}
}


void GameObject::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	XMFLOAT4X4 world;
	XMStoreFloat4x4(&world, XMMatrixTranspose(XMLoadFloat4x4(&worldTransform)));
	_pCommandList->SetGraphicsRoot32BitConstants(1, 16, &world, 0);
}

void GameObject::UpdateHitboxShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	if (pMesh) {
		BoundingOrientedBox boundingBox = pMesh->GetOOBB();
		XMFLOAT4X4 world = Matrix4x4::ScaleTransform(Vector3::ScalarProduct(pMesh->GetOOBB().Extents, 2.0f));
		XMFLOAT4X4 translate = Matrix4x4::Identity();
		translate._41 += boundingBox.Center.x;
		translate._42 += boundingBox.Center.y;
		translate._43 += boundingBox.Center.z;
		world = Matrix4x4::Multiply(Matrix4x4::Multiply(world, worldTransform), translate);
		XMStoreFloat4x4(&world, XMMatrixTranspose(XMLoadFloat4x4(&world)));
		//world = Matrix4x4::Identity();
		_pCommandList->SetGraphicsRoot32BitConstants(1, 16, &world, 0);
	}
}

void GameObject::CopyObject(const GameObject& _other) {
	name = _other.name;

	worldTransform = _other.worldTransform;

	localTransform = _other.localTransform;
	localPosition = _other.localPosition;
	localScale = _other.localScale;
	localRotation = _other.localRotation;

	// ���� �����ؾ� �Ѵ�.

	boundingBox = _other.boundingBox;
	isOOBBBCover = _other.isOOBBBCover;

	pMesh = _other.pMesh;

	for (int i = 0; i < _other.pChildren.size(); ++i) {
		shared_ptr<GameObject> child = make_shared<GameObject>();
		child->CopyObject(*_other.pChildren[i]);
		SetChild(child);
	}
}



/////////////////////////// GameObjectManager /////////////////////
unordered_map<string, shared_ptr<GameObject>> GameObjectManager::storage;

bool GameObjectManager::LoadFromFile(const string& _name, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	
	if (!storage.contains(_name)) {	// �޸𸮿� ������Ʈ�� ���� ���
		
		ifstream file("GameObject/" + _name, ios::binary);	// ������ ����
		shared_ptr<GameObject> newObject = GameObject::LoadFromFile(file, _pDevice, _pCommandList);	// ������Ʈ �� ���� ������(�Ž�-���׸���) ������ �о�´�.
		newObject->UpdateObject();	// eachTransfrom�� �°� �� ������ ������Ʈ���� worldTransform�� ����, oobb����
		storage[_name] = newObject;

		return true;
	}
	
	return false;
}

void GameObjectManager::ReleaseObject(const string& _name) {
	storage.erase(_name);
}

void GameObjectManager::ReleaseAllObject() {
	storage.clear();
}

shared_ptr<GameObject> GameObjectManager::GetGameObject(const string& _name, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {

	LoadFromFile(_name, _pDevice, _pCommandList);	// �̹� �޸𸮰� �ö�� �ִٸ� �˾Ƽ� �ҷ����� ����
	
	// ���丮�� �� ������Ʈ ������ ���� ������Ʈ�� �����Ͽ� �����Ѵ�.
	shared_ptr<GameObject> Object = make_shared<GameObject>();
	Object->CopyObject(*storage[_name]);
	return Object;
}

bool GameObjectManager::AddObject(shared_ptr<GameObject> _newObject) {
	if (!_newObject)
		return false;

	if (!storage.contains(_newObject->GetName())) {
		storage[_newObject->GetName()] = _newObject;
		return true;
	}
	return false;
}
