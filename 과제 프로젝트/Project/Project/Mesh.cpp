#include "stdafx.h"
#include "Mesh.h"
#include "GameFramework.h"

///////////////////////////////////////////////////////////////////////////////
/// �޽�
shared_ptr<Mesh> Mesh::LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	
	// meshNameSize(UINT) / meshName(string) : �޽��� �̸��� �о�´�.
	string meshNameBuffer;
	ReadStringBinary(meshNameBuffer, _file);

	if (meshNameBuffer.length() == 0) {	// �޽��� ���� ��� NULL�� �����Ѵ�.
		return NULL;
	}

	// �޽��� �����Ѵٸ� ������ �Ҵ��Ͽ� �����͸� ä����.
	shared_ptr<Mesh> pNewMesh = make_shared<Mesh>();

	// ���ؽ��� ���� �б�
	_file.read((char*)&pNewMesh->nVertex, sizeof(UINT));
	// �� �̸� �ϱ�
	ReadStringBinary(pNewMesh->name, _file);

	// OOBB���� �б�
	XMFLOAT3 oobbCenter, oobbExtents;
	_file.read((char*)&oobbCenter, sizeof(XMFLOAT3));
	_file.read((char*)&oobbExtents, sizeof(XMFLOAT3));
	pNewMesh->oobb = BoundingOrientedBox(oobbCenter, oobbExtents, XMFLOAT4A(0.0f, 0.0f, 0.0f, 1.0f));

	// �����ǰ� �б�
	vector<float> positions(3 * pNewMesh->nVertex);
	_file.read((char*)positions.data(), sizeof(float) * 3 * pNewMesh->nVertex);
	// positions�� ���ҽ��� ����� ����
	pNewMesh->pPositionBuffer = CreateBufferResource(_pDevice, _pCommandList, positions.data(), sizeof(float) * 3 * pNewMesh->nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNewMesh->pPositionUploadBuffer);
	pNewMesh->positionBufferView.BufferLocation = pNewMesh->pPositionBuffer->GetGPUVirtualAddress();
	pNewMesh->positionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	pNewMesh->positionBufferView.SizeInBytes = sizeof(XMFLOAT3) * pNewMesh->nVertex;
	
	// ����޽� ���� �б�
	UINT nSubMesh;
	_file.read((char*)&nSubMesh, sizeof(UINT));

	// ����޽��� ������ŭ ���͸� �̸� �Ҵ��� ���´�.
	pNewMesh->nSubMeshIndex.assign(nSubMesh, 0);
	pNewMesh->pSubMeshIndexBuffers.assign(nSubMesh, {});
	pNewMesh->pSubMeshIndexUploadBuffers.assign(nSubMesh, {});
	pNewMesh->subMeshIndexBufferViews.assign(nSubMesh, {});
	pNewMesh->materials.assign(nSubMesh, {});

	for (UINT i = 0; i < nSubMesh; ++i) {
		_file.read((char*)&pNewMesh->nSubMeshIndex[i], sizeof(UINT));
		vector<UINT> indices(pNewMesh->nSubMeshIndex[i]);
		_file.read((char*)indices.data(), sizeof(UINT) * pNewMesh->nSubMeshIndex[i]);
		// subMeshIndices�� ���ҽ��� ����� ����
		pNewMesh->pSubMeshIndexBuffers[i] = CreateBufferResource(_pDevice, _pCommandList, indices.data(), sizeof(UINT) * pNewMesh->nSubMeshIndex[i], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, pNewMesh->pSubMeshIndexUploadBuffers[i]);
		pNewMesh->subMeshIndexBufferViews[i].BufferLocation = pNewMesh->pSubMeshIndexBuffers[i]->GetGPUVirtualAddress();
		pNewMesh->subMeshIndexBufferViews[i].Format = DXGI_FORMAT_R32_UINT;
		pNewMesh->subMeshIndexBufferViews[i].SizeInBytes = sizeof(UINT) * pNewMesh->nSubMeshIndex[i];

		// ���׸��� �������� �б�. (Ȯ���� ����)
		pNewMesh->materials[i] = Material::LoadFromFile(_file, _pDevice, _pCommandList);
	}

	return pNewMesh;
}

// ������, �Ҹ���
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
		// �ش� ����Ž��� ��Ī�Ǵ� ���׸����� Set ���ش�.
		materials[i]->UpdateShaderVariable(_pCommandList);
		_pCommandList->IASetIndexBuffer(&subMeshIndexBufferViews[i]);
		_pCommandList->DrawIndexedInstanced(nSubMeshIndex[i], 1, 0, 0, 0);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// �⺻ �޽�

// ���� ���� �� �Լ�
shared_ptr<BasicMesh> BasicMesh::LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	
	// meshNameSize(UINT) / meshName(string) : �޽��� �̸��� �о�´�.
	string meshNameBuffer;
	ReadStringBinary(meshNameBuffer, _file);

	if (meshNameBuffer.length() == 0) {	// �޽��� ���� ��� NULL�� �����Ѵ�.
		return NULL;
	}

	// �޽��� �����Ѵٸ� ������ �Ҵ��Ͽ� �����͸� ä����.
	shared_ptr<BasicMesh> pNewMesh = make_shared<BasicMesh>();

	// ���ؽ��� ���� �б�
	_file.read((char*)&pNewMesh->nVertex, sizeof(UINT));
	// �� �̸� �ϱ�
	ReadStringBinary(pNewMesh->name, _file);

	// OOBB���� �б�
	XMFLOAT3 oobbCenter, oobbExtents;
	_file.read((char*)&oobbCenter, sizeof(XMFLOAT3));
	_file.read((char*)&oobbExtents, sizeof(XMFLOAT3));
	pNewMesh->oobb = BoundingOrientedBox(oobbCenter, oobbExtents, XMFLOAT4A(0.0f, 0.0f, 0.0f, 1.0f));

	// �����ǰ� �б�
	vector<float> positions(3 * pNewMesh->nVertex);
	_file.read((char*)positions.data(), sizeof(float) * 3 * pNewMesh->nVertex);
	// positions�� ���ҽ��� ����� ����
	pNewMesh->pPositionBuffer = CreateBufferResource(_pDevice, _pCommandList, positions.data(), sizeof(float) * 3 * pNewMesh->nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNewMesh->pPositionUploadBuffer);
	pNewMesh->positionBufferView.BufferLocation = pNewMesh->pPositionBuffer->GetGPUVirtualAddress();
	pNewMesh->positionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	pNewMesh->positionBufferView.SizeInBytes = sizeof(XMFLOAT3) * pNewMesh->nVertex;

	// ��ְ� �б�
	vector<float> normals(3 * pNewMesh->nVertex);
	_file.read((char*)normals.data(), sizeof(float) * 3 * pNewMesh->nVertex);
	// normals�� ���ҽ��� ����� ����
	pNewMesh->pNormalBuffer = CreateBufferResource(_pDevice, _pCommandList, normals.data(), sizeof(float) * 3 * pNewMesh->nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNewMesh->pNormalUploadBuffer);
	pNewMesh->normalBufferView.BufferLocation = pNewMesh->pNormalBuffer->GetGPUVirtualAddress();
	pNewMesh->normalBufferView.StrideInBytes = sizeof(XMFLOAT3);
	pNewMesh->normalBufferView.SizeInBytes = sizeof(XMFLOAT3) * pNewMesh->nVertex;

	// UV�� �б�
	UINT nUV;
	_file.read((char*)&nUV, sizeof(UINT));	// uv�� ���� �б�

	vector<float> textureCoords(2 * nUV);
	_file.read((char*)textureCoords.data(), sizeof(float) * 2 * nUV);
	// UV�� ���ҽ��� ����� ����
	pNewMesh->pTextureCoordBuffer = CreateBufferResource(_pDevice, _pCommandList, textureCoords.data(), sizeof(float) * 2 * nUV, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNewMesh->pTextureCoordUploadBuffer);
	pNewMesh->textureCoordBufferView.BufferLocation = pNewMesh->pTextureCoordBuffer->GetGPUVirtualAddress();
	pNewMesh->textureCoordBufferView.StrideInBytes = sizeof(XMFLOAT2);
	pNewMesh->textureCoordBufferView.SizeInBytes = sizeof(XMFLOAT2) * nUV;

	// ����޽� ���� �б�
	UINT nSubMesh;
	_file.read((char*)&nSubMesh, sizeof(UINT));

	// ����޽��� ������ŭ ���͸� �̸� �Ҵ��� ���´�.
	pNewMesh->nSubMeshIndex.assign(nSubMesh, 0);
	pNewMesh->pSubMeshIndexBuffers.assign(nSubMesh, {});
	pNewMesh->pSubMeshIndexUploadBuffers.assign(nSubMesh, {});
	pNewMesh->subMeshIndexBufferViews.assign(nSubMesh, {});
	pNewMesh->materials.assign(nSubMesh, {});

	for (UINT i = 0; i < nSubMesh; ++i) {
		_file.read((char*)&pNewMesh->nSubMeshIndex[i], sizeof(UINT));
		vector<UINT> indices(pNewMesh->nSubMeshIndex[i]);
		_file.read((char*)indices.data(), sizeof(UINT) * pNewMesh->nSubMeshIndex[i]);
		// subMeshIndices�� ���ҽ��� ����� ����
		pNewMesh->pSubMeshIndexBuffers[i] = CreateBufferResource(_pDevice, _pCommandList, indices.data(), sizeof(UINT) * pNewMesh->nSubMeshIndex[i], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, pNewMesh->pSubMeshIndexUploadBuffers[i]);
		pNewMesh->subMeshIndexBufferViews[i].BufferLocation = pNewMesh->pSubMeshIndexBuffers[i]->GetGPUVirtualAddress();
		pNewMesh->subMeshIndexBufferViews[i].Format = DXGI_FORMAT_R32_UINT;
		pNewMesh->subMeshIndexBufferViews[i].SizeInBytes = sizeof(UINT) * pNewMesh->nSubMeshIndex[i];

		// ���׸��� �������� �б�. (Ȯ���� ����)
		pNewMesh->materials[i] = Material::LoadFromFile(_file, _pDevice, _pCommandList);
	}
	
	return pNewMesh;
}

// ������, �Ҹ���
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
		// �ش� ����Ž��� ��Ī�Ǵ� ���׸����� Set ���ش�.
		materials[i]->UpdateShaderVariable(_pCommandList);
		_pCommandList->IASetIndexBuffer(&subMeshIndexBufferViews[i]);
		_pCommandList->DrawIndexedInstanced(nSubMeshIndex[i], 1, 0, 0, 0);
	}
}

///////////////////////////////////////////////////////////////////////////////
/// boundingBox �޽�

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
///  ���̸�

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
		cout << "������ �����ϴ�.\n";
		return false;
	}

	width = nWidth;
	length = nLength;
	scale = _scale;
	heightMapPixels.assign(width, vector<BYTE>(length));
	vector<BYTE> heightMapPixelsTemp(width * length);

	file.read((char*)heightMapPixelsTemp.data(), width * length);	// ������ �о�´�. DMA
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

	//z-��ǥ�� 1, 3, 5, ...�� ��� �ε����� �����ʿ��� �������� �����ȴ�. 
	bool bRightToLeft = ((z % 2) != 0);
	if (bRightToLeft) {
		/*������ �ﰢ������ �����ʿ��� ���� �������� �����Ǵ� ����̴�. ���� �׸��� �������� (fzPercent < fxPercent)
		�� ����̴�. �� ��� TopLeft�� �ȼ� ���� (fTopLeft = fTopRight + (fBottomLeft - fBottomRight))�� �ٻ��Ѵ�. ���� �׸��� ������ (fzPercent �� fxPercent)�� ����̴�. �� ��� BottomRight�� �ȼ� ���� (fBottomRight = fBottomLeft + (fTopRight - fTopLeft))�� �ٻ��Ѵ�.*/
		if (zPercent >= xPercent)
			bottomRight = bottomLeft + (topRight - topLeft);
		else
			topLeft = topRight + (bottomLeft - bottomRight);
	}
	else {
		/*������ �ﰢ������ ���ʿ��� ������ �������� �����Ǵ� ����̴�. ���� �׸��� ������ (fzPercent < (1.0f - fxPercent))�� ����̴�. �� ��� TopRight�� �ȼ� ���� (fTopRight = fTopLeft + (fBottomRight - fBottomLeft))��
		�ٻ��Ѵ�. ���� �׸��� �������� (fzPercent �� (1.0f - fxPercent))�� ����̴�. �� ��� BottomLeft�� �ȼ� ����
		(fBottomLeft = fTopLeft + (fBottomRight - fTopRight))�� �ٻ��Ѵ�.*/
		if (zPercent < (1.0f - xPercent))
			topRight = topLeft + (bottomRight - bottomLeft);
		else
			bottomLeft = topLeft + (bottomRight - topRight);
	}

	//�簢���� �� ���� �����Ͽ� ����(�ȼ� ��)�� ����Ѵ�. 
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

	//x-��ǥ�� z-��ǥ�� ���� ���� ������ ����� ������ ���� ���ʹ� y-�� ���� �����̴�. 
	if ((imageX < 0.0f) || (imageZ < 0.0f) || (imageX >= width - 1) || (imageZ >= length - 1))
		return(XMFLOAT3(0.0f, 1.0f, 0.0f));

	/*���� �ʿ��� (x, z) ��ǥ�� �ȼ� ���� ������ �� ���� �� (x+1, z), (x, z+1)�� ���� �ȼ� ���� ����Ͽ� ���� ���͸�
	����Ѵ�.*/
	float y1 = (float)heightMapPixels[(int)imageX][(int)imageZ] * scale.y;
	float y2 = (float)heightMapPixels[(int)imageX][(int)imageZ + 1] * scale.y;
	float y3 = (float)heightMapPixels[(int)imageX + 1][(int)imageZ] * scale.y;

	//xmf3Edge1�� (0, y3, m_xmf3Scale.z) - (0, y1, 0) �����̴�. 
	XMFLOAT3 xmf3Edge1 = XMFLOAT3(0.0f, y3 - y1, scale.z);
	//xmf3Edge2�� (m_xmf3Scale.x, y2, 0) - (0, y1, 0) �����̴�. 
	XMFLOAT3 xmf3Edge2 = XMFLOAT3(scale.x, y2 - y1, 0.0f);
	//���� ���ʹ� xmf3Edge1�� xmf3Edge2�� ������ ����ȭ�ϸ� �ȴ�. 
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
/// �ͷ��� �޽�

// ���� ���� �� �Լ�


shared_ptr<TerrainMesh> TerrainMesh::LoadFromFile(const string& _heightMapfileName, XMINT2 _imageSize, XMFLOAT3 _terrainSize, float _gridWidth, const VS_MaterialMappedFormat& _materialColors,
	float _detailWidth, const array<string, TEXTURETYPENUM>& _textureFileNames,
	const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {

	shared_ptr<TerrainMesh> pNewTerrainMesh = make_shared<TerrainMesh>();	// TerrainMesh ����

	pNewTerrainMesh->size = _terrainSize;	// �ͷ����� ����,����,���� ����

	XMFLOAT3 heightMapScale = { _terrainSize.x / _imageSize.x, _terrainSize.y / HeightMapImage::height , _terrainSize.z / _imageSize.y };
	bool result = pNewTerrainMesh->heightMapImage.LoadHeightMapFile("Terrain/" + _heightMapfileName, _imageSize.x, _imageSize.y, heightMapScale);	// ���̸� �̹����� �о�´�.
	if (!result)
		return nullptr;
	
	// ���ؽ� ����
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
	int nWidth = (int)(terrainSize.x / _gridWidth) + 2;	// ���� ���ؽ��� ��
	int nLength = (int)(terrainSize.z / _gridWidth) + 2;	// ���� ���ؽ��� ��
	int nVertex = nWidth * nLength;

	// ���ؽ� ������ ����
	vector<XMFLOAT3> positions(nVertex);
	vector<XMFLOAT3> normals(nVertex);
	vector<XMFLOAT2> textureCoords(nVertex);
	vector<XMFLOAT2> textureCoords2(nVertex);

	for (int i = 0, z = 0; z < nLength; z++) {
		for (int x = 0; x < nWidth; x++, i++) {
			// x, z�� ���� ��ġ
			XMFLOAT3 position = { x * terrainSize.x / (nWidth - 1), 0 , z * terrainSize.z / (nLength - 1) };
			//������ ���̿� ������ ���� �����κ��� ���Ѵ�. 
			positions[i] = XMFLOAT3(position.x, heightMapImage.GetHeight(position.x, position.z), position.z);
			normals[i] = heightMapImage.GetNormal(position.x, position.z);
			textureCoords[i] = XMFLOAT2((float)x / nWidth, (float)z / nLength);
			textureCoords2[i] = XMFLOAT2(position.x / _detailWidth, position.z / _detailWidth);
		}
	}

	// positions�� ���ҽ��� ����� ����
	pPositionBuffer = CreateBufferResource(_pDevice, _pCommandList, positions.data(), sizeof(XMFLOAT3) * nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pPositionUploadBuffer);
	positionBufferView.BufferLocation = pPositionBuffer->GetGPUVirtualAddress();
	positionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	positionBufferView.SizeInBytes = sizeof(XMFLOAT3) * nVertex;

	// normals�� ���ҽ��� ����� ����
	pNormalBuffer = CreateBufferResource(_pDevice, _pCommandList, normals.data(), sizeof(XMFLOAT3) * nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNormalUploadBuffer);
	normalBufferView.BufferLocation = pNormalBuffer->GetGPUVirtualAddress();
	normalBufferView.StrideInBytes = sizeof(XMFLOAT3);
	normalBufferView.SizeInBytes = sizeof(XMFLOAT3) * nVertex;

	// UV�� ���ҽ��� ����� ����
	pTextureCoordBuffer = CreateBufferResource(_pDevice, _pCommandList, textureCoords.data(), sizeof(XMFLOAT2) * nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pTextureCoordUploadBuffer);
	textureCoordBufferView.BufferLocation = pTextureCoordBuffer->GetGPUVirtualAddress();
	textureCoordBufferView.StrideInBytes = sizeof(XMFLOAT2);
	textureCoordBufferView.SizeInBytes = sizeof(XMFLOAT2) * nVertex;

	// UV2�� ���ҽ��� ����� ����
	pTextureCoord2Buffer = CreateBufferResource(_pDevice, _pCommandList, textureCoords2.data(), sizeof(XMFLOAT2) * nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pTextureCoord2UploadBuffer);
	textureCoord2BufferView.BufferLocation = pTextureCoord2Buffer->GetGPUVirtualAddress();
	textureCoord2BufferView.StrideInBytes = sizeof(XMFLOAT2);
	textureCoord2BufferView.SizeInBytes = sizeof(XMFLOAT2) * nVertex;

	// ����޽� ���� �����
	UINT nSubMesh = 1;

	// ����޽��� ������ŭ ���͸� �̸� �Ҵ��� ���´�.
	nSubMeshIndex.assign(nSubMesh, 0);
	pSubMeshIndexBuffers.assign(nSubMesh, {});
	pSubMeshIndexUploadBuffers.assign(nSubMesh, {});
	subMeshIndexBufferViews.assign(nSubMesh, {});
	materials.assign(nSubMesh, {});

	nSubMeshIndex[0] = ((nWidth * 2) * (nLength - 1)) + ((nLength - 1) - 1);// �ε����� ����
	vector<UINT> indices(nSubMeshIndex[0]);

	// �ε��� ä���
	for (int j = 0, z = 0; z < nLength - 1; z++) {
		if ((z % 2) == 0) {
			//Ȧ�� ��° ���̹Ƿ�(z = 0, 2, 4, ...) �ε����� ���� ������ ���ʿ��� ������ �����̴�. 
			for (int x = 0; x < nWidth; x++) {
				//ù ��° ���� �����ϰ� ���� �ٲ� ������(x == 0) ù ��° �ε����� �߰��Ѵ�. 
				if ((x == 0) && (z > 0)) indices[j++] = (UINT)(x + (z * nWidth));
				//�Ʒ�(x, z), ��(x, z+1)�� ������ �ε����� �߰��Ѵ�. 
				indices[j++] = (UINT)(x + (z * nWidth));
				indices[j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
		else {
			//¦�� ��° ���̹Ƿ�(z = 1, 3, 5, ...) �ε����� ���� ������ �����ʿ��� ���� �����̴�. 
			for (int x = nWidth - 1; x >= 0; x--) {
				//���� �ٲ� ������(x == (nWidth-1)) ù ��° �ε����� �߰��Ѵ�. 
				if (x == (nWidth - 1)) indices[j++] = (UINT)(x + (z * nWidth));
				//�Ʒ�(x, z), ��(x, z+1)�� ������ �ε����� �߰��Ѵ�. 
				indices[j++] = (UINT)(x + (z * nWidth));
				indices[j++] = (UINT)((x + (z * nWidth)) + nWidth);
			}
		}
	}

	// subMeshIndices�� ���ҽ��� ����� ����
	pSubMeshIndexBuffers[0] = CreateBufferResource(_pDevice, _pCommandList, indices.data(), sizeof(UINT) * nSubMeshIndex[0], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, pSubMeshIndexUploadBuffers[0]);
	subMeshIndexBufferViews[0].BufferLocation = pSubMeshIndexBuffers[0]->GetGPUVirtualAddress();
	subMeshIndexBufferViews[0].Format = DXGI_FORMAT_R32_UINT;
	subMeshIndexBufferViews[0].SizeInBytes = sizeof(UINT) * nSubMeshIndex[0];

	// ���׸��� �������� �����
	materials[0] = Material::LoadFromDirect("terrain_material", _materialColors, _textureFileNames, _pDevice, _pCommandList);


}

void TerrainMesh::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	_pCommandList->IASetPrimitiveTopology(primitiveTopology);
	D3D12_VERTEX_BUFFER_VIEW vertexBuffersViews[4] = { positionBufferView , normalBufferView, textureCoordBufferView, textureCoord2BufferView };
	_pCommandList->IASetVertexBuffers(0, 4, vertexBuffersViews);
	for (int i = 0; i < nSubMeshIndex.size(); ++i) {
		// �ش� ����Ž��� ��Ī�Ǵ� ���׸����� Set ���ش�.
		materials[i]->UpdateShaderVariable(_pCommandList);
		_pCommandList->IASetIndexBuffer(&subMeshIndexBufferViews[i]);
		_pCommandList->DrawIndexedInstanced(nSubMeshIndex[i], 1, 0, 0, 0);
	}
}

const HeightMapImage& TerrainMesh::GetHeightMapImage() const {
	return heightMapImage;
}

///////////////////////////////////////////////////////////////////////////////
/// ������ �޽�

shared_ptr<BillBoardMesh> BillBoardMesh::LoadFromFile(const string& _meshName, const string& _textureFileName, const XMFLOAT2& _center, const XMFLOAT2& _size, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {

	// �޽��� �����Ѵٸ� ������ �Ҵ��Ͽ� �����͸� ä����.
	shared_ptr<BillBoardMesh> pNewMesh = make_shared<BillBoardMesh>();
	pNewMesh->name = _meshName;
	pNewMesh->nVertex = 1;

	pNewMesh->oobb = BoundingOrientedBox(XMFLOAT3(0,0,0), XMFLOAT3(_size.x / 2.f, _size.y / 2.f, 0), XMFLOAT4A(0.0f, 0.0f, 0.0f, 1.0f));

	// positions�� ���ҽ��� ����� ����
	XMFLOAT3 position = XMFLOAT3(_center.x, _center.y, 0);
	pNewMesh->pPositionBuffer = CreateBufferResource(_pDevice, _pCommandList, &position, sizeof(XMFLOAT3) * pNewMesh->nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNewMesh->pPositionUploadBuffer);
	pNewMesh->positionBufferView.BufferLocation = pNewMesh->pPositionBuffer->GetGPUVirtualAddress();
	pNewMesh->positionBufferView.StrideInBytes = sizeof(XMFLOAT3);
	pNewMesh->positionBufferView.SizeInBytes = sizeof(XMFLOAT3) * pNewMesh->nVertex;

	// positions�� ���ҽ��� ����� ����
	XMFLOAT2 boardSize = _size;
	pNewMesh->pSizeBuffer = CreateBufferResource(_pDevice, _pCommandList, &boardSize, sizeof(XMFLOAT2) * pNewMesh->nVertex, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, pNewMesh->pSizeUploadBuffer);
	pNewMesh->sizeBufferView.BufferLocation = pNewMesh->pSizeBuffer->GetGPUVirtualAddress();
	pNewMesh->sizeBufferView.StrideInBytes = sizeof(XMFLOAT2);
	pNewMesh->sizeBufferView.SizeInBytes = sizeof(XMFLOAT2) * pNewMesh->nVertex;

	// ����޽� ���� �б�
	UINT nSubMesh = 1;
	// ����޽��� ������ŭ ���͸� �̸� �Ҵ��� ���´�.
	pNewMesh->nSubMeshIndex.assign(nSubMesh, 0);
	pNewMesh->pSubMeshIndexBuffers.assign(nSubMesh, {});
	pNewMesh->pSubMeshIndexUploadBuffers.assign(nSubMesh, {});
	pNewMesh->subMeshIndexBufferViews.assign(nSubMesh, {});
	pNewMesh->materials.assign(nSubMesh, {});

	pNewMesh->nSubMeshIndex[0] = 1;
	UINT indexInfo = 0;

	// subMeshIndices�� ���ҽ��� ����� ����
	pNewMesh->pSubMeshIndexBuffers[0] = CreateBufferResource(_pDevice, _pCommandList, &indexInfo, sizeof(UINT) * pNewMesh->nSubMeshIndex[0], D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_INDEX_BUFFER, pNewMesh->pSubMeshIndexUploadBuffers[0]);
	pNewMesh->subMeshIndexBufferViews[0].BufferLocation = pNewMesh->pSubMeshIndexBuffers[0]->GetGPUVirtualAddress();
	pNewMesh->subMeshIndexBufferViews[0].Format = DXGI_FORMAT_R32_UINT;
	pNewMesh->subMeshIndexBufferViews[0].SizeInBytes = sizeof(UINT) * pNewMesh->nSubMeshIndex[0];

	// ���׸��� �������� �����
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

// ������, �Ҹ���
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
		// �ش� ����Ž��� ��Ī�Ǵ� ���׸����� Set ���ش�.
		materials[i]->UpdateShaderVariable(_pCommandList);
		_pCommandList->IASetIndexBuffer(&subMeshIndexBufferViews[i]);
		_pCommandList->DrawIndexedInstanced(nSubMeshIndex[i], 1, 0, 0, 0);
	}
}


