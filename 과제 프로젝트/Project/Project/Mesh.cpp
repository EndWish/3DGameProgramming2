#include "stdafx.h"
#include "Mesh.h"
#include "GameFramework.h"

///////////////////////////////////////////////////////////////////////////////
/// 메쉬
shared_ptr<Mesh> Mesh::LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	
	// meshNameSize(UINT) / meshName(string) : 메쉬의 이름을 읽어온다.
	string meshNameBuffer;
	ReadStringBinary(meshNameBuffer, _file);

	if (meshNameBuffer.length() == 0) {	// 메쉬가 없는 경우 NULL을 리턴한다.
		return NULL;
	}

	// 메쉬가 존재한다면 공간을 할당하여 데이터를 채우자.
	shared_ptr<Mesh> pNewMesh = make_shared<Mesh>();

	// 버텍스의 개수 읽기
	_file.read((char*)&pNewMesh->nVertex, sizeof(UINT));
	// 모델 이름 일기
	ReadStringBinary(pNewMesh->name, _file);

	// OOBB정보 읽기
	XMFLOAT3 oobbCenter, oobbExtents;
	_file.read((char*)&oobbCenter, sizeof(XMFLOAT3));
	_file.read((char*)&oobbExtents, sizeof(XMFLOAT3));
	pNewMesh->oobb = BoundingOrientedBox(oobbCenter, oobbExtents, XMFLOAT4A(0.0f, 0.0f, 0.0f, 1.0f));

	// 포지션값 읽기
	vector<float> positions(3 * pNewMesh->nVertex);
	_file.read((char*)positions.data(), sizeof(float) * 3 * pNewMesh->nVertex);
	// positions를 리소스로 만드는 과정
	pNewMesh->pPositionBuffer = CreateBufferResource(_pDevice, _pCommandList, positions.data(), sizeof(float) * 3 * pNewMesh->nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNewMesh->pPositionUploadBuffer);
	pNewMesh->positionBufferView.BufferLocation = pNewMesh->pPositionBuffer->GetGPUVirtualAddress();
	pNewMesh->positionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	pNewMesh->positionBufferView.SizeInBytes = sizeof(XMFLOAT3) * pNewMesh->nVertex;
	
	// 서브메쉬 정보 읽기
	UINT nSubMesh;
	_file.read((char*)&nSubMesh, sizeof(UINT));

	// 서브메쉬의 개수만큼 벡터를 미리 할당해 놓는다.
	pNewMesh->nSubMeshIndex.assign(nSubMesh, 0);
	pNewMesh->pSubMeshIndexBuffers.assign(nSubMesh, {});
	pNewMesh->pSubMeshIndexUploadBuffers.assign(nSubMesh, {});
	pNewMesh->subMeshIndexBufferViews.assign(nSubMesh, {});
	pNewMesh->materials.assign(nSubMesh, {});

	for (UINT i = 0; i < nSubMesh; ++i) {
		_file.read((char*)&pNewMesh->nSubMeshIndex[i], sizeof(UINT));
		vector<UINT> indices(pNewMesh->nSubMeshIndex[i]);
		_file.read((char*)indices.data(), sizeof(UINT) * pNewMesh->nSubMeshIndex[i]);
		// subMeshIndices를 리소스로 만드는 과정
		pNewMesh->pSubMeshIndexBuffers[i] = CreateBufferResource(_pDevice, _pCommandList, indices.data(), sizeof(UINT) * pNewMesh->nSubMeshIndex[i], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, pNewMesh->pSubMeshIndexUploadBuffers[i]);
		pNewMesh->subMeshIndexBufferViews[i].BufferLocation = pNewMesh->pSubMeshIndexBuffers[i]->GetGPUVirtualAddress();
		pNewMesh->subMeshIndexBufferViews[i].Format = DXGI_FORMAT_R32_UINT;
		pNewMesh->subMeshIndexBufferViews[i].SizeInBytes = sizeof(UINT) * pNewMesh->nSubMeshIndex[i];

		// 마테리얼 파일정보 읽기. (확장자 없음)
		pNewMesh->materials[i] = Material::LoadFromFile(_file, _pDevice, _pCommandList);
	}

	return pNewMesh;
}

// 생성자, 소멸자
Mesh::Mesh() {
	primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	nVertex = 0;
	positionBufferView = D3D12_VERTEX_BUFFER_VIEW();
}
Mesh::~Mesh() {

}

const string& Mesh::GetName() const {
	return name;
}

const BoundingOrientedBox& Mesh::GetOOBB() const {
	return oobb;
}

void Mesh::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	_pCommandList->IASetPrimitiveTopology(primitiveTopology);
	D3D12_VERTEX_BUFFER_VIEW vertexBuffersViews[1] = { positionBufferView };
	_pCommandList->IASetVertexBuffers(0, 1, vertexBuffersViews);
	for (int i = 0; i < nSubMeshIndex.size(); ++i) {
		// 해당 서브매쉬와 매칭되는 메테리얼을 Set 해준다.
		materials[i]->UpdateShaderVariable(_pCommandList);
		_pCommandList->IASetIndexBuffer(&subMeshIndexBufferViews[i]);
		_pCommandList->DrawIndexedInstanced(nSubMeshIndex[i], 1, 0, 0, 0);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// 기본 메쉬

// 정적 변수 및 함수
shared_ptr<BasicMesh> BasicMesh::LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	
	// meshNameSize(UINT) / meshName(string) : 메쉬의 이름을 읽어온다.
	string meshNameBuffer;
	ReadStringBinary(meshNameBuffer, _file);

	if (meshNameBuffer.length() == 0) {	// 메쉬가 없는 경우 NULL을 리턴한다.
		return NULL;
	}

	// 메쉬가 존재한다면 공간을 할당하여 데이터를 채우자.
	shared_ptr<BasicMesh> pNewMesh = make_shared<BasicMesh>();

	// 버텍스의 개수 읽기
	_file.read((char*)&pNewMesh->nVertex, sizeof(UINT));
	// 모델 이름 일기
	ReadStringBinary(pNewMesh->name, _file);

	// OOBB정보 읽기
	XMFLOAT3 oobbCenter, oobbExtents;
	_file.read((char*)&oobbCenter, sizeof(XMFLOAT3));
	_file.read((char*)&oobbExtents, sizeof(XMFLOAT3));
	pNewMesh->oobb = BoundingOrientedBox(oobbCenter, oobbExtents, XMFLOAT4A(0.0f, 0.0f, 0.0f, 1.0f));

	// 포지션값 읽기
	vector<float> positions(3 * pNewMesh->nVertex);
	_file.read((char*)positions.data(), sizeof(float) * 3 * pNewMesh->nVertex);
	// positions를 리소스로 만드는 과정
	pNewMesh->pPositionBuffer = CreateBufferResource(_pDevice, _pCommandList, positions.data(), sizeof(float) * 3 * pNewMesh->nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNewMesh->pPositionUploadBuffer);
	pNewMesh->positionBufferView.BufferLocation = pNewMesh->pPositionBuffer->GetGPUVirtualAddress();
	pNewMesh->positionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	pNewMesh->positionBufferView.SizeInBytes = sizeof(XMFLOAT3) * pNewMesh->nVertex;

	// 노멀값 읽기
	vector<float> normals(3 * pNewMesh->nVertex);
	_file.read((char*)normals.data(), sizeof(float) * 3 * pNewMesh->nVertex);
	// normals를 리소스로 만드는 과정
	pNewMesh->pNormalBuffer = CreateBufferResource(_pDevice, _pCommandList, normals.data(), sizeof(float) * 3 * pNewMesh->nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNewMesh->pNormalUploadBuffer);
	pNewMesh->normalBufferView.BufferLocation = pNewMesh->pNormalBuffer->GetGPUVirtualAddress();
	pNewMesh->normalBufferView.StrideInBytes = sizeof(XMFLOAT3);
	pNewMesh->normalBufferView.SizeInBytes = sizeof(XMFLOAT3) * pNewMesh->nVertex;

	// UV값 읽기
	UINT nUV;
	_file.read((char*)&nUV, sizeof(UINT));	// uv의 개수 읽기

	vector<float> textureCoords(2 * nUV);
	_file.read((char*)textureCoords.data(), sizeof(float) * 2 * nUV);
	// UV를 리소스로 만드는 과정
	pNewMesh->pTextureCoordBuffer = CreateBufferResource(_pDevice, _pCommandList, textureCoords.data(), sizeof(float) * 2 * nUV, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNewMesh->pTextureCoordUploadBuffer);
	pNewMesh->textureCoordBufferView.BufferLocation = pNewMesh->pTextureCoordBuffer->GetGPUVirtualAddress();
	pNewMesh->textureCoordBufferView.StrideInBytes = sizeof(XMFLOAT2);
	pNewMesh->textureCoordBufferView.SizeInBytes = sizeof(XMFLOAT2) * nUV;

	// 서브메쉬 정보 읽기
	UINT nSubMesh;
	_file.read((char*)&nSubMesh, sizeof(UINT));

	// 서브메쉬의 개수만큼 벡터를 미리 할당해 놓는다.
	pNewMesh->nSubMeshIndex.assign(nSubMesh, 0);
	pNewMesh->pSubMeshIndexBuffers.assign(nSubMesh, {});
	pNewMesh->pSubMeshIndexUploadBuffers.assign(nSubMesh, {});
	pNewMesh->subMeshIndexBufferViews.assign(nSubMesh, {});
	pNewMesh->materials.assign(nSubMesh, {});

	for (UINT i = 0; i < nSubMesh; ++i) {
		_file.read((char*)&pNewMesh->nSubMeshIndex[i], sizeof(UINT));
		vector<UINT> indices(pNewMesh->nSubMeshIndex[i]);
		_file.read((char*)indices.data(), sizeof(UINT) * pNewMesh->nSubMeshIndex[i]);
		// subMeshIndices를 리소스로 만드는 과정
		pNewMesh->pSubMeshIndexBuffers[i] = CreateBufferResource(_pDevice, _pCommandList, indices.data(), sizeof(UINT) * pNewMesh->nSubMeshIndex[i], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, pNewMesh->pSubMeshIndexUploadBuffers[i]);
		pNewMesh->subMeshIndexBufferViews[i].BufferLocation = pNewMesh->pSubMeshIndexBuffers[i]->GetGPUVirtualAddress();
		pNewMesh->subMeshIndexBufferViews[i].Format = DXGI_FORMAT_R32_UINT;
		pNewMesh->subMeshIndexBufferViews[i].SizeInBytes = sizeof(UINT) * pNewMesh->nSubMeshIndex[i];

		// 마테리얼 파일정보 읽기. (확장자 없음)
		pNewMesh->materials[i] = Material::LoadFromFile(_file, _pDevice, _pCommandList);
	}
	
	return pNewMesh;
}

// 생성자, 소멸자
BasicMesh::BasicMesh() {
	primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	nVertex = 0;
	normalBufferView = D3D12_VERTEX_BUFFER_VIEW();
	positionBufferView = D3D12_VERTEX_BUFFER_VIEW();
	textureCoordBufferView = D3D12_VERTEX_BUFFER_VIEW();
}
BasicMesh::~BasicMesh() {

}

const string& BasicMesh::GetName() const {
	return name;
}

const BoundingOrientedBox& BasicMesh::GetOOBB() const {
	return oobb;
}

void BasicMesh::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	_pCommandList->IASetPrimitiveTopology(primitiveTopology);
	D3D12_VERTEX_BUFFER_VIEW vertexBuffersViews[3] = { positionBufferView , normalBufferView, textureCoordBufferView };
	_pCommandList->IASetVertexBuffers(0, 3, vertexBuffersViews);
	for (int i = 0; i < nSubMeshIndex.size(); ++i) {
		// 해당 서브매쉬와 매칭되는 메테리얼을 Set 해준다.
		materials[i]->UpdateShaderVariable(_pCommandList);
		_pCommandList->IASetIndexBuffer(&subMeshIndexBufferViews[i]);
		_pCommandList->DrawIndexedInstanced(nSubMeshIndex[i], 1, 0, 0, 0);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// boundingBox 메쉬

shared_ptr<HitBoxMesh> HitBoxMesh::hitBoxMesh;

void HitBoxMesh::MakeHitBoxMesh(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	hitBoxMesh = make_shared<HitBoxMesh>();

	XMFLOAT3 c(0, 0, 0);
	XMFLOAT3 e(0.5, 0.5, 0.5);
	hitBoxMesh->primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	vector<XMFLOAT3> positions{
		XMFLOAT3(c.x - e.x, c.y - e.y, c.z - e.z),
		XMFLOAT3(c.x - e.x, c.y - e.y, c.z + e.z),
		XMFLOAT3(c.x + e.x, c.y - e.y, c.z + e.z),
		XMFLOAT3(c.x + e.x, c.y - e.y, c.z - e.z),
		XMFLOAT3(c.x - e.x, c.y + e.y, c.z - e.z),
		XMFLOAT3(c.x - e.x, c.y + e.y, c.z + e.z),
		XMFLOAT3(c.x + e.x, c.y + e.y, c.z + e.z),
		XMFLOAT3(c.x + e.x, c.y + e.y, c.z - e.z),
	};

	hitBoxMesh->pPositionBuffer = CreateBufferResource(_pDevice, _pCommandList, positions.data(), sizeof(XMFLOAT3) * 8, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, hitBoxMesh->pPositionUploadBuffer);
	hitBoxMesh->positionBufferView.BufferLocation = hitBoxMesh->pPositionBuffer->GetGPUVirtualAddress();
	hitBoxMesh->positionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	hitBoxMesh->positionBufferView.SizeInBytes = sizeof(XMFLOAT3) * 8;

	vector<UINT> indices{
		0,1,1,2,
		2,3,3,0,
		4,5,5,6,
		6,7,7,4,
		0,4,1,5,
		2,6,3,7
	};

	hitBoxMesh->pIndexBuffers = CreateBufferResource(_pDevice, _pCommandList, indices.data(), sizeof(UINT) * 24, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, hitBoxMesh->pIndexUploadBuffers);
	hitBoxMesh->indexBufferView.BufferLocation = hitBoxMesh->pIndexBuffers->GetGPUVirtualAddress();
	hitBoxMesh->indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	hitBoxMesh->indexBufferView.SizeInBytes = sizeof(UINT) * 24;
}

shared_ptr<HitBoxMesh> HitBoxMesh::GetHitBoxMesh() {
	return hitBoxMesh;
}

HitBoxMesh::HitBoxMesh() {
	primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;
	positionBufferView = D3D12_VERTEX_BUFFER_VIEW();
	indexBufferView = D3D12_INDEX_BUFFER_VIEW();
}

HitBoxMesh::~HitBoxMesh() {

}

void HitBoxMesh::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	_pCommandList->IASetPrimitiveTopology(primitiveTopology);
	D3D12_VERTEX_BUFFER_VIEW vertexBuffersViews[1] = { positionBufferView };
	_pCommandList->IASetVertexBuffers(0, 1, vertexBuffersViews);
	_pCommandList->IASetIndexBuffer(&indexBufferView);
	_pCommandList->DrawIndexedInstanced(24, 1, 0, 0, 0);
}

///////////////////////////////////////////////////////////////////////////////
///  높이맵

HeightMapImage::HeightMapImage() {
	width = 0;
	length = 0;
	scale = XMFLOAT3(1, 1, 1);
}
HeightMapImage::~HeightMapImage() {

}

bool HeightMapImage::LoadHeightMapFile(const string& _fileName, int nWidth, int nLength, const XMFLOAT3 _scale) {
	ifstream file(_fileName, ios::binary);
	if (!file) {
		cout << "파일이 없습니다.\n";
		return false;
	}

	width = nWidth;
	length = nLength;
	scale = _scale;
	heightMapPixels.assign(width, vector<BYTE>(length));
	vector<BYTE> heightMapPixelsTemp(width * length);

	file.read((char*)heightMapPixelsTemp.data(), width * length);	// 파일을 읽어온다. DMA
	int count = 0;
	for (int y = 0; y < length; y++) {
		for (int x = 0; x < width; x++) {
			heightMapPixels[x][length - 1 - y] = heightMapPixelsTemp[x + (y * width)];
		}
	}
	return true;
}

float HeightMapImage::GetHeight(float _terrainX, float _terrainZ) const {
	float imageX = _terrainX / scale.x;
	float imageZ = _terrainZ / scale.z;
	imageX = clamp(imageX, 0.f, (float)(width) - 1.01f);
	imageZ = clamp(imageZ, 0.f, (float)(length) - 1.01f);

	int x = (int)imageX;
	int z = (int)imageZ;
	float xPercent = imageX - x;
	float zPercent = imageZ - z;
	float bottomLeft = (float)heightMapPixels[x][z];
	float bottomRight = (float)heightMapPixels[x + 1][z];
	float topLeft = (float)heightMapPixels[x][z + 1];
	float topRight = (float)heightMapPixels[x + 1][z + 1];

	//z-좌표가 1, 3, 5, ...인 경우 인덱스가 오른쪽에서 왼쪽으로 나열된다. 
	bool bRightToLeft = ((z % 2) != 0);
	if (bRightToLeft) {
		/*지형의 삼각형들이 오른쪽에서 왼쪽 방향으로 나열되는 경우이다. 다음 그림의 오른쪽은 (fzPercent < fxPercent)
		인 경우이다. 이 경우 TopLeft의 픽셀 값은 (fTopLeft = fTopRight + (fBottomLeft - fBottomRight))로 근사한다. 다음 그림의 왼쪽은 (fzPercent ≥ fxPercent)인 경우이다. 이 경우 BottomRight의 픽셀 값은 (fBottomRight = fBottomLeft + (fTopRight - fTopLeft))로 근사한다.*/
		if (zPercent >= xPercent)
			bottomRight = bottomLeft + (topRight - topLeft);
		else
			topLeft = topRight + (bottomLeft - bottomRight);
	}
	else {
		/*지형의 삼각형들이 왼쪽에서 오른쪽 방향으로 나열되는 경우이다. 다음 그림의 왼쪽은 (fzPercent < (1.0f - fxPercent))인 경우이다. 이 경우 TopRight의 픽셀 값은 (fTopRight = fTopLeft + (fBottomRight - fBottomLeft))로
		근사한다. 다음 그림의 오른쪽은 (fzPercent ≥ (1.0f - fxPercent))인 경우이다. 이 경우 BottomLeft의 픽셀 값은
		(fBottomLeft = fTopLeft + (fBottomRight - fTopRight))로 근사한다.*/
		if (zPercent < (1.0f - xPercent))
			topRight = topLeft + (bottomRight - bottomLeft);
		else
			bottomLeft = topLeft + (bottomRight - topRight);
	}

	//사각형의 네 점을 보간하여 높이(픽셀 값)를 계산한다. 
	float fTopHeight = topLeft * (1 - xPercent) + topRight * xPercent;
	float fBottomHeight = bottomLeft * (1 - xPercent) + bottomRight * xPercent;
	float fHeight = fBottomHeight * (1 - zPercent) + fTopHeight * zPercent;
	return fHeight * scale.y;

}

XMFLOAT3 HeightMapImage::GetNormal(float _terrainX, float _terrainZ) const {
	float imageX = _terrainX / scale.x;
	float imageZ = _terrainZ / scale.z;
	imageX = clamp(imageX, 0.f, (float)(width)-1.01f);
	imageZ = clamp(imageZ, 0.f, (float)(length)-1.01f);

	//x-좌표와 z-좌표가 높이 맵의 범위를 벗어나면 지형의 법선 벡터는 y-축 방향 벡터이다. 
	if ((imageX < 0.0f) || (imageZ < 0.0f) || (imageX >= width - 1) || (imageZ >= length - 1))
		return(XMFLOAT3(0.0f, 1.0f, 0.0f));

	/*높이 맵에서 (x, z) 좌표의 픽셀 값과 인접한 두 개의 점 (x+1, z), (x, z+1)에 대한 픽셀 값을 사용하여 법선 벡터를
	계산한다.*/
	float y1 = (float)heightMapPixels[(int)imageX][(int)imageZ] * scale.y;
	float y2 = (float)heightMapPixels[(int)imageX][(int)imageZ + 1] * scale.y;
	float y3 = (float)heightMapPixels[(int)imageX + 1][(int)imageZ] * scale.y;

	//xmf3Edge1은 (0, y3, m_xmf3Scale.z) - (0, y1, 0) 벡터이다. 
	XMFLOAT3 xmf3Edge1 = XMFLOAT3(0.0f, y3 - y1, scale.z);
	//xmf3Edge2는 (m_xmf3Scale.x, y2, 0) - (0, y1, 0) 벡터이다. 
	XMFLOAT3 xmf3Edge2 = XMFLOAT3(scale.x, y2 - y1, 0.0f);
	//법선 벡터는 xmf3Edge1과 xmf3Edge2의 외적을 정규화하면 된다. 
	XMFLOAT3 normal = Vector3::Normalize(Vector3::Cross(xmf3Edge1, xmf3Edge2));
	return normal;
}

int HeightMapImage::GetWidth() const {
	return width;
}
int HeightMapImage::GetLength() const {
	return length;
}

///////////////////////////////////////////////////////////////////////////////
/// 터레인 메쉬

// 정적 변수 및 함수


shared_ptr<TerrainMesh> TerrainMesh::LoadFromFile(const string& _heightMapfileName, XMINT2 _imageSize, XMFLOAT3 _terrainSize, float _gridWidth, const VS_MaterialMappedFormat& _materialColors,
	float _detailWidth, const array<string, TEXTURETYPENUM>& _textureFileNames,
	const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {

	shared_ptr<TerrainMesh> pNewTerrainMesh = make_shared<TerrainMesh>();	// TerrainMesh 생성

	pNewTerrainMesh->size = _terrainSize;	// 터레인의 가로,세로,높이 대입

	XMFLOAT3 heightMapScale = { _terrainSize.x / _imageSize.x, _terrainSize.y / HeightMapImage::height , _terrainSize.z / _imageSize.y };
	bool result = pNewTerrainMesh->heightMapImage.LoadHeightMapFile("Terrain/" + _heightMapfileName, _imageSize.x, _imageSize.y, heightMapScale);	// 높이맵 이미지를 읽어온다.
	if (!result)
		return nullptr;
	
	// 버텍스 생성
	pNewTerrainMesh->CreateVertex(_terrainSize, _gridWidth, _materialColors, _detailWidth, _textureFileNames, _pDevice, _pCommandList);

	return pNewTerrainMesh;
}



TerrainMesh::TerrainMesh() {
	textureCoord2BufferView = D3D12_VERTEX_BUFFER_VIEW();

	size = XMFLOAT3(100, 10, 100);
	//primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
}
TerrainMesh::~TerrainMesh() {
	
}

void TerrainMesh::CreateVertex(const XMFLOAT3& terrainSize, float _gridWidth, const VS_MaterialMappedFormat& _materialColors , float _detailWidth, const array<string, TEXTURETYPENUM>& _textureFileNames, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	int nWidth = (int)(terrainSize.x / _gridWidth) + 2;	// 가로 버텍스의 수
	int nLength = (int)(terrainSize.z / _gridWidth) + 2;	// 세로 버텍스의 수
	int nVertex = nWidth * nLength;

	// 버텍스 데이터 버퍼
	vector<XMFLOAT3> positions(nVertex);
	vector<XMFLOAT3> normals(nVertex);
	vector<XMFLOAT2> textureCoords(nVertex);
	vector<XMFLOAT2> textureCoords2(nVertex);

	for (int i = 0, z = 0; z < nLength; z++) {
		for (int x = 0; x < nWidth; x++, i++) {
			// x, z의 실제 위치
			XMFLOAT3 position = { x * terrainSize.x / (nWidth - 1), 0 , z * terrainSize.z / (nLength - 1) };
			//정점의 높이와 색상을 높이 맵으로부터 구한다. 
			positions[i] = XMFLOAT3(position.x, heightMapImage.GetHeight(position.x, position.z), position.z);
			normals[i] = heightMapImage.GetNormal(position.x, position.z);
			textureCoords[i] = XMFLOAT2((float)x / nWidth, (float)z / nLength);
			textureCoords2[i] = XMFLOAT2(position.x / _detailWidth, position.z / _detailWidth);
		}
	}

	// positions를 리소스로 만드는 과정
	pPositionBuffer = CreateBufferResource(_pDevice, _pCommandList, positions.data(), sizeof(XMFLOAT3) * nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pPositionUploadBuffer);
	positionBufferView.BufferLocation = pPositionBuffer->GetGPUVirtualAddress();
	positionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	positionBufferView.SizeInBytes = sizeof(XMFLOAT3) * nVertex;

	// normals를 리소스로 만드는 과정
	pNormalBuffer = CreateBufferResource(_pDevice, _pCommandList, normals.data(), sizeof(XMFLOAT3) * nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNormalUploadBuffer);
	normalBufferView.BufferLocation = pNormalBuffer->GetGPUVirtualAddress();
	normalBufferView.StrideInBytes = sizeof(XMFLOAT3);
	normalBufferView.SizeInBytes = sizeof(XMFLOAT3) * nVertex;

	// UV를 리소스로 만드는 과정
	pTextureCoordBuffer = CreateBufferResource(_pDevice, _pCommandList, textureCoords.data(), sizeof(XMFLOAT2) * nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pTextureCoordUploadBuffer);
	textureCoordBufferView.BufferLocation = pTextureCoordBuffer->GetGPUVirtualAddress();
	textureCoordBufferView.StrideInBytes = sizeof(XMFLOAT2);
	textureCoordBufferView.SizeInBytes = sizeof(XMFLOAT2) * nVertex;

	// UV2를 리소스로 만드는 과정
	pTextureCoord2Buffer = CreateBufferResource(_pDevice, _pCommandList, textureCoords2.data(), sizeof(XMFLOAT2) * nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pTextureCoord2UploadBuffer);
	textureCoord2BufferView.BufferLocation = pTextureCoord2Buffer->GetGPUVirtualAddress();
	textureCoord2BufferView.StrideInBytes = sizeof(XMFLOAT2);
	textureCoord2BufferView.SizeInBytes = sizeof(XMFLOAT2) * nVertex;

	// 서브메쉬 정보 만들기
	UINT nSubMesh = 1;

	// 서브메쉬의 개수만큼 벡터를 미리 할당해 놓는다.
	nSubMeshIndex.assign(nSubMesh, 0);
	pSubMeshIndexBuffers.assign(nSubMesh, {});
	pSubMeshIndexUploadBuffers.assign(nSubMesh, {});
	subMeshIndexBufferViews.assign(nSubMesh, {});
	materials.assign(nSubMesh, {});

	nSubMeshIndex[0] = ((nWidth * 2) * (nLength - 1)) + ((nLength - 1) - 1);// 인덱스의 개수
	vector<UINT> indices(nSubMeshIndex[0]);

	// 인덱스 채우기
	for (int j = 0, z = 0; z < nLength - 1; z++) {
		if ((z % 2) == 0) {
			//홀수 번째 줄이므로(z = 0, 2, 4, ...) 인덱스의 나열 순서는 왼쪽에서 오른쪽 방향이다. 
			for (int x = 0; x < nWidth; x++) {
				//첫 번째 줄을 제외하고 줄이 바뀔 때마다(x == 0) 첫 번째 인덱스를 추가한다. 
				if ((x == 0) && (z > 0)) indices[j++] = (UINT)(x + (z * nWidth));
				//아래(x, z), 위(x, z+1)의 순서로 인덱스를 추가한다. 
				indices[j++] = (UINT)(x + (z * nWidth));
				indices[j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
		else {
			//짝수 번째 줄이므로(z = 1, 3, 5, ...) 인덱스의 나열 순서는 오른쪽에서 왼쪽 방향이다. 
			for (int x = nWidth - 1; x >= 0; x--) {
				//줄이 바뀔 때마다(x == (nWidth-1)) 첫 번째 인덱스를 추가한다. 
				if (x == (nWidth - 1)) indices[j++] = (UINT)(x + (z * nWidth));
				//아래(x, z), 위(x, z+1)의 순서로 인덱스를 추가한다. 
				indices[j++] = (UINT)(x + (z * nWidth));
				indices[j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
	}

	// subMeshIndices를 리소스로 만드는 과정
	pSubMeshIndexBuffers[0] = CreateBufferResource(_pDevice, _pCommandList, indices.data(), sizeof(UINT) * nSubMeshIndex[0], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, pSubMeshIndexUploadBuffers[0]);
	subMeshIndexBufferViews[0].BufferLocation = pSubMeshIndexBuffers[0]->GetGPUVirtualAddress();
	subMeshIndexBufferViews[0].Format = DXGI_FORMAT_R32_UINT;
	subMeshIndexBufferViews[0].SizeInBytes = sizeof(UINT) * nSubMeshIndex[0];

	// 마테리얼 파일정보 만들기
	materials[0] = Material::LoadFromDirect("terrain_material", _materialColors, _textureFileNames, _pDevice, _pCommandList);


}

void TerrainMesh::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	_pCommandList->IASetPrimitiveTopology(primitiveTopology);
	D3D12_VERTEX_BUFFER_VIEW vertexBuffersViews[4] = { positionBufferView , normalBufferView, textureCoordBufferView, textureCoord2BufferView };
	_pCommandList->IASetVertexBuffers(0, 4, vertexBuffersViews);
	for (int i = 0; i < nSubMeshIndex.size(); ++i) {
		// 해당 서브매쉬와 매칭되는 메테리얼을 Set 해준다.
		materials[i]->UpdateShaderVariable(_pCommandList);
		_pCommandList->IASetIndexBuffer(&subMeshIndexBufferViews[i]);
		_pCommandList->DrawIndexedInstanced(nSubMeshIndex[i], 1, 0, 0, 0);
	}
}

const HeightMapImage& TerrainMesh::GetHeightMapImage() const {
	return heightMapImage;
}

///////////////////////////////////////////////////////////////////////////////
/// 빌보드 메쉬

shared_ptr<BillBoardMesh> BillBoardMesh::LoadFromFile(const string& _meshName, const string& _textureFileName, const XMFLOAT2& _center, const XMFLOAT2& _size, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {

	// 메쉬가 존재한다면 공간을 할당하여 데이터를 채우자.
	shared_ptr<BillBoardMesh> pNewMesh = make_shared<BillBoardMesh>();
	pNewMesh->name = _meshName;
	pNewMesh->nVertex = 1;

	pNewMesh->oobb = BoundingOrientedBox(XMFLOAT3(0,0,0), XMFLOAT3(_size.x / 2.f, _size.y / 2.f, 0), XMFLOAT4A(0.0f, 0.0f, 0.0f, 1.0f));

	// positions를 리소스로 만드는 과정
	XMFLOAT3 position = XMFLOAT3(_center.x, _center.y, 0);
	pNewMesh->pPositionBuffer = CreateBufferResource(_pDevice, _pCommandList, &position, sizeof(XMFLOAT3) * pNewMesh->nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNewMesh->pPositionUploadBuffer);
	pNewMesh->positionBufferView.BufferLocation = pNewMesh->pPositionBuffer->GetGPUVirtualAddress();
	pNewMesh->positionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	pNewMesh->positionBufferView.SizeInBytes = sizeof(XMFLOAT3) * pNewMesh->nVertex;

	// positions를 리소스로 만드는 과정
	XMFLOAT2 boardSize = _size;
	pNewMesh->pSizeBuffer = CreateBufferResource(_pDevice, _pCommandList, &boardSize, sizeof(XMFLOAT2) * pNewMesh->nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNewMesh->pSizeUploadBuffer);
	pNewMesh->sizeBufferView.BufferLocation = pNewMesh->pSizeBuffer->GetGPUVirtualAddress();
	pNewMesh->sizeBufferView.StrideInBytes = sizeof(XMFLOAT2);
	pNewMesh->sizeBufferView.SizeInBytes = sizeof(XMFLOAT2) * pNewMesh->nVertex;

	// 서브메쉬 정보 읽기
	UINT nSubMesh = 1;
	// 서브메쉬의 개수만큼 벡터를 미리 할당해 놓는다.
	pNewMesh->nSubMeshIndex.assign(nSubMesh, 0);
	pNewMesh->pSubMeshIndexBuffers.assign(nSubMesh, {});
	pNewMesh->pSubMeshIndexUploadBuffers.assign(nSubMesh, {});
	pNewMesh->subMeshIndexBufferViews.assign(nSubMesh, {});
	pNewMesh->materials.assign(nSubMesh, {});

	pNewMesh->nSubMeshIndex[0] = 1;
	UINT indexInfo = 0;

	// subMeshIndices를 리소스로 만드는 과정
	pNewMesh->pSubMeshIndexBuffers[0] = CreateBufferResource(_pDevice, _pCommandList, &indexInfo, sizeof(UINT) * pNewMesh->nSubMeshIndex[0], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, pNewMesh->pSubMeshIndexUploadBuffers[0]);
	pNewMesh->subMeshIndexBufferViews[0].BufferLocation = pNewMesh->pSubMeshIndexBuffers[0]->GetGPUVirtualAddress();
	pNewMesh->subMeshIndexBufferViews[0].Format = DXGI_FORMAT_R32_UINT;
	pNewMesh->subMeshIndexBufferViews[0].SizeInBytes = sizeof(UINT) * pNewMesh->nSubMeshIndex[0];

	// 마테리얼 파일정보 만들기
	VS_MaterialMappedFormat materialColors;
	materialColors.diffuse = XMFLOAT4(0, 0, 0, 1);
	materialColors.ambient = XMFLOAT4(0, 0, 0, 1);
	materialColors.emissive = XMFLOAT4(0, 0, 0, 1);
	materialColors.specular = XMFLOAT4(0, 0, 0, 1);

	array<string, TEXTURETYPENUM> textureFileNames{
	_textureFileName,		//MATERIAL_ALBEDO_MAP_INDEX
	"",							//MATERIAL_SPECULAR_MAP_INDEX
	"",							//MATERIAL_NORMAL_MAP_INDEX
	"",							//MATERIAL_METALLIC_MAP_INDEX
	"",							//MATERIAL_EMISSION_MAP_INDEX
	"",		//MATERIAL_DETAIL_ALBEDO_MAP_INDEX
	"" };

	pNewMesh->materials[0] = Material::LoadFromDirect("BillBoardMaterial", materialColors, textureFileNames, _pDevice, _pCommandList);

	//Material::LoadFromDirect("BillBoardMaterial", )

	return pNewMesh;
}

// 생성자, 소멸자
BillBoardMesh::BillBoardMesh() {
	sizeBufferView = D3D12_VERTEX_BUFFER_VIEW();
	primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_POINTLIST;
}
BillBoardMesh::~BillBoardMesh() {

}

void BillBoardMesh::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	_pCommandList->IASetPrimitiveTopology(primitiveTopology);
	D3D12_VERTEX_BUFFER_VIEW vertexBuffersViews[2] = { positionBufferView, sizeBufferView };
	_pCommandList->IASetVertexBuffers(0, 2, vertexBuffersViews);
	for (int i = 0; i < nSubMeshIndex.size(); ++i) {
		// 해당 서브매쉬와 매칭되는 메테리얼을 Set 해준다.
		materials[i]->UpdateShaderVariable(_pCommandList);
		_pCommandList->IASetIndexBuffer(&subMeshIndexBufferViews[i]);
		_pCommandList->DrawIndexedInstanced(nSubMeshIndex[i], 1, 0, 0, 0);
	}
}


