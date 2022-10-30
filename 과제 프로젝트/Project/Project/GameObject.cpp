#include "stdafx.h"
#include "GameObject.h"
#include "Light.h"
#include "GameFramework.h"

bool IsOOBBTypeFinal(OOBB_TYPE _OOBBType) {
	return _OOBBType == DISABLED_FINAL || _OOBBType == MESHOOBB_FINAL || _OOBBType == PRIVATEOOBB_FINAL;
}

bool IsOOBBTypeDisabled(OOBB_TYPE _OOBBType) {
	return _OOBBType == DISABLED || _OOBBType == DISABLED_FINAL;
}

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

	// 새로만들 오브젝트의 메모리공간을 할당한다.
	shared_ptr<GameObject> pNewObject = make_shared<GameObject>();

	// nameSize (UINT) / name(string)
	ReadStringBinary(pNewObject->name, _file);

	// localTransform(float4x4)
	_file.read((char*)&pNewObject->localPosition, sizeof(XMFLOAT3));
	_file.read((char*)&pNewObject->localScale, sizeof(XMFLOAT3));
	_file.read((char*)&pNewObject->localRotation, sizeof(XMFLOAT4));
	pNewObject->UpdateLocalTransform();

	// 메쉬를 읽어온다.
	pNewObject->pMesh = BasicMesh::LoadFromFile(_file, _pDevice, _pCommandList);
	if (pNewObject->pMesh)
		pNewObject->SetOOBBType(OOBB_TYPE::MESHOOBB);

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
	layer = NONE;
	name = "unknown";
	worldTransform = Matrix4x4::Identity();
	localTransform = Matrix4x4::Identity();
	localPosition = XMFLOAT3(0, 0, 0);
	localRotation = XMFLOAT4(0, 0, 0, 1);
	localScale = XMFLOAT3(1,1,1);
	boundingBox = BoundingOrientedBox();
	OOBBType = DISABLED;
}
GameObject::~GameObject() {

}

void GameObject::Create() {

}

const string& GameObject::GetName() const {
	return name;
}

const shared_ptr<Mesh> GameObject::GetMesh() const {
	return pMesh;
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
	XMFLOAT3 moveVector = GetLocalRightVector();	// RightVector를 가져와서
	moveVector = Vector3::Normalize(moveVector);	// 단위벡터로 바꾼후
	moveVector = Vector3::ScalarProduct(moveVector, distance);	// 이동거리만큼 곱해준다.
	localPosition = Vector3::Add(localPosition, moveVector);
}
void GameObject::Move(const XMFLOAT3& _moveVector) {
	localPosition = Vector3::Add(localPosition, _moveVector);
}
void GameObject::MoveUp(float distance) {
	XMFLOAT3 moveVector = GetLocalUpVector();	// UpVector를 가져와서
	moveVector = Vector3::Normalize(moveVector);	// 단위벡터로 바꾼후
	moveVector = Vector3::ScalarProduct(moveVector, distance);	// 이동거리만큼 곱해준다.
	localPosition = Vector3::Add(localPosition, moveVector);
}
void GameObject::MoveFront(float distance) {
	XMFLOAT3 moveVector = GetLocalLookVector();	// LookVector를 가져와서
	moveVector = Vector3::Normalize(moveVector);	// 단위벡터로 바꾼후
	moveVector = Vector3::ScalarProduct(moveVector, distance);	// 이동거리만큼 곱해준다.
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

OOBB_TYPE GameObject::GetOOBBType() const {
	return OOBBType;
}
const BoundingOrientedBox& GameObject::GetBoundingBox() const {
	return boundingBox;
}
shared_ptr<GameObject> GameObject::GetChild(const string& _name) const {
	for (auto pChild : pChildren) {
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
shared_ptr<GameObject> GameObject::GetRootParent() {
	shared_ptr<GameObject> pParent = wpParent.lock();
	if (pParent)	// 부모가 있을 경우
		return pParent->GetRootParent();
	return shared_from_this();
}

void GameObject::SetLayer(WORLD_OBJ_LAYER _layer) {
	layer = _layer;
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
	// 입양할 아이가, 부모가 있을 경우 부모로 부터 독립시킨다.
	if (auto pPreParent = _pChild->wpParent.lock()) {
		pPreParent->pChildren.erase(ranges::find(pPreParent->pChildren, _pChild));
	}

	// 나의 자식으로 입양
	pChildren.push_back(_pChild);

	// 자식의 부모를 나로 지정
	_pChild->wpParent = shared_from_this();
}
void GameObject::SetMesh(const shared_ptr<Mesh>& _pMesh, OOBB_TYPE _OOBBType) {
	pMesh = _pMesh;
	OOBBType = _OOBBType;
}
void GameObject::SetPrivateOOBB(const shared_ptr<BoundingOrientedBox>& _pPrivateOOBB, OOBB_TYPE _OOBBType) {
	pPrivateOOBB = _pPrivateOOBB;
	OOBBType = _OOBBType;
}
void GameObject::SetOOBBType(OOBB_TYPE _OOBBType) {
	OOBBType = _OOBBType;
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
	if (auto pParentLock = wpParent.lock()) {	// 부모가 있을 경우
		worldTransform = Matrix4x4::Multiply(localTransform, pParentLock->worldTransform);
	}
	else {	// 부모가 없을 경우
		worldTransform = localTransform;
	}
}
void GameObject::UpdateWorldTransformAll() {
	UpdateWorldTransform();

	// 자식들도 worldTransform을 업데이트 시킨다.
	for (auto& pChild : pChildren) {
		pChild->UpdateWorldTransformAll();
	}
}
void GameObject::UpdateOOBB() {
	if (OOBBType == MESHOOBB || OOBBType == MESHOOBB_FINAL) {
		if (pMesh) {
			// Mesh의 OOBB를 현재 오브젝트의 transform값으로 변환
			pMesh->GetOOBB().Transform(boundingBox, XMLoadFloat4x4(&worldTransform));
			// OOBB의 회전값을 정규화
			//XMStoreFloat4(&boundingBox.Orientation, XMQuaternionNormalize(XMLoadFloat4(&boundingBox.Orientation)));
		}
		else {
			cout << "에러 : 메쉬가 없어서 OOBB를 만들 수 없습니다.\n";
		}
	}
	else if (OOBBType == PRIVATEOOBB || OOBBType == PRIVATEOOBB_FINAL || OOBBType == PRIVATEOOBB_COVER) {
		if (pPrivateOOBB) {
			pPrivateOOBB->Transform(boundingBox, XMLoadFloat4x4(&worldTransform));
		}
		else {
			cout << "에러 : 전용OOBB가 없는데 OOBB타입이 PRIVATEOOBB류 입니다..\n";
		}
	}

}
void GameObject::UpdateOOBBAll() {
	UpdateOOBB();
	if (OOBBType != DISABLED_FINAL && OOBBType != MESHOOBB_FINAL && OOBBType != PRIVATEOOBB_FINAL) {	// FINAL이 아닐경우
		for (const auto& pChild : pChildren) {
			pChild->UpdateOOBBAll();
		}
	}
}
void GameObject::UpdateObject() {
	UpdateLocalTransform();
	UpdateWorldTransformAll();
	UpdateOOBBAll();
}

void GameObject::ProcessCollision(float _timeElapsed) {

}
bool GameObject::CheckCollision(const shared_ptr<GameObject>& _other) const {
	OOBB_TYPE otherOOBBType = _other->GetOOBBType();
	if ( !IsOOBBTypeDisabled(OOBBType) && !IsOOBBTypeDisabled(otherOOBBType) ) {
		if (boundingBox.Intersects(_other->boundingBox)) {	// 충돌체크를 한다.
			if (OOBBType != PRIVATEOOBB_COVER && otherOOBBType != PRIVATEOOBB_COVER) {
				return true;
			}
		}
		else {	// 충돌하지 않았을 경우
			if (OOBBType == PRIVATEOOBB_COVER || otherOOBBType == PRIVATEOOBB_COVER) {
				return false;
			}
		}
	}

	if (!IsOOBBTypeFinal(otherOOBBType)) {	// 상대쪽이 Final이 아닐경우, 상대방 자식과 충돌확인해 본다.
		for (const auto& pOtherChild : _other->pChildren) {
			if (CheckCollision(pOtherChild))
				return true;
		}
	}

	if (!IsOOBBTypeFinal(OOBBType)) {	// 내쪽이 Final이 아닐경우, 자식들과 상대방쪽 충동을 확인해 본다.
		for (const auto& pChild : pChildren) {
			if (pChild->CheckCollision(_other))
				return true;
		}
	}

	return false;
}

void GameObject::Animate(float _timeElapsed) {

	for (const auto& pChild : pChildren) {
		pChild->Animate(_timeElapsed);
	}
}

void GameObject::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	if (pMesh) {	// 메쉬가 있을 경우에만 렌더링을 한다.
		UpdateShaderVariable(_pCommandList);
		// 사용할 쉐이더의 그래픽스 파이프라인을 설정한다 [수정요망]
		
		pMesh->Render(_pCommandList);
	}
	for (const auto& pChild : pChildren) {
		pChild->Render(_pCommandList);
	}

}
void GameObject::RenderHitBox(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, HitBoxMesh& _hitBox) {
	
	if (!IsOOBBTypeDisabled(OOBBType)) {	// 메쉬가 있을 경우에만 렌더링을 한다.
		UpdateHitboxShaderVariable(_pCommandList);
		// 사용할 쉐이더의 그래픽스 파이프라인을 설정한다 [수정요망]
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
	XMFLOAT4X4 world = Matrix4x4::ScaleTransform(Vector3::ScalarProduct(boundingBox.Extents, 2.0f));
	world = Matrix4x4::Multiply(world, Matrix4x4::RotateQuaternion(boundingBox.Orientation));
	world = Matrix4x4::Multiply(world, Matrix4x4::MoveTransform(boundingBox.Center));
	XMStoreFloat4x4(&world, XMMatrixTranspose(XMLoadFloat4x4(&world)));
	_pCommandList->SetGraphicsRoot32BitConstants(1, 16, &world, 0);
}

void GameObject::CopyObject(const GameObject& _other) {
	name = _other.name;

	worldTransform = _other.worldTransform;

	localTransform = _other.localTransform;
	localPosition = _other.localPosition;
	localScale = _other.localScale;
	localRotation = _other.localRotation;

	// 빛도 복사해야 한다.

	boundingBox = _other.boundingBox;
	OOBBType = _other.OOBBType;
	pPrivateOOBB = _other.pPrivateOOBB;

	pMesh = _other.pMesh;

	for (int i = 0; i < _other.pChildren.size(); ++i) {
		shared_ptr<GameObject> child = make_shared<GameObject>();
		child->CopyObject(*_other.pChildren[i]);
		SetChild(child);
	}
}
void GameObject::DeleteMe() {
	if (layer < NUM) {
		static_pointer_cast<PlayScene>(GameFramework::Instance().GetCurrentScene())->DeleteObject(shared_from_this(), layer);
	}
}

/////////////////////////// GameObjectManager /////////////////////
unordered_map<string, shared_ptr<GameObject>> GameObjectManager::storage;

bool GameObjectManager::AddObject(shared_ptr<GameObject> _newObject) {
	if (!_newObject)
		return false;

	if (!storage.contains(_newObject->GetName())) {
		storage[_newObject->GetName()] = _newObject;
		return true;
	}
	return false;
}
bool GameObjectManager::LoadFromFile(const string& _name, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	
	if (!storage.contains(_name)) {	// 메모리에 오브젝트가 없을 경우
		
		ifstream file("GameObject/" + _name, ios::binary);	// 파일을 연다
		shared_ptr<GameObject> newObject = GameObject::LoadFromFile(file, _pDevice, _pCommandList);	// 오브젝트 및 하위 데이터(매쉬-메테리얼) 정보를 읽어온다.
		newObject->UpdateObject();	// eachTransfrom에 맞게 각 계층의 오브젝트들의 worldTransform을 갱신, oobb갱신
		storage[_name] = newObject;

		return true;
	}
	
	return false;
}
shared_ptr<GameObject> GameObjectManager::GetGameObject(const string& _name) {
	// 스토리지 내 오브젝트 정보와 같은 오브젝트를 복사하여 생성한다.
	shared_ptr<GameObject> Object = make_shared<GameObject>();
	Object->CopyObject(*storage[_name]);
	return Object;
}
shared_ptr<GameObject> GameObjectManager::GetGameObject(const string& _name, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {

	LoadFromFile(_name, _pDevice, _pCommandList);	// 이미 메모리가 올라와 있다면 알아서 불러오지 않음

	return GetGameObject(_name);
}



void GameObjectManager::ReleaseObject(const string& _name) {
	storage.erase(_name);
}
void GameObjectManager::ReleaseAllObject() {
	storage.clear();
}




