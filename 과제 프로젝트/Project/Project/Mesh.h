#pragma once

#include "Shader.h"
#include "Material.h"

///////////////////////////////////////////////////////////////////////////////
/// �޽�
class Mesh {
public:
	static shared_ptr<Mesh> LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

protected:
	string name;

	D3D12_PRIMITIVE_TOPOLOGY primitiveTopology;
	UINT nVertex;	// ���ؽ�(������ ��ֺ���)�� ����

	ComPtr<ID3D12Resource> pPositionBuffer;	// ���ؽ��� ��ġ ����
	ComPtr<ID3D12Resource> pPositionUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW positionBufferView;

	vector<UINT> nSubMeshIndex;	// subMesh���� �ε��� ����
	vector<ComPtr<ID3D12Resource>> pSubMeshIndexBuffers;	// subMesh���� �ε��� ����
	vector<ComPtr<ID3D12Resource>> pSubMeshIndexUploadBuffers;
	vector<D3D12_INDEX_BUFFER_VIEW> subMeshIndexBufferViews;

	vector<shared_ptr<Material>> materials;
	BoundingOrientedBox oobb;

public:		// �������� ��� �Լ���
	// ������ �� �Ҹ���
	Mesh();
	virtual ~Mesh();

public:		// ��� �Լ���

	// get, set�Լ�
	const string& GetName() const;
	const BoundingOrientedBox& GetOOBB() const;
	//void LoadFromFile(const string& _fileName, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

///////////////////////////////////////////////////////////////////////////////
/// �⺻ �޽�
class BasicMesh : public Mesh {
public:
	static shared_ptr<BasicMesh> LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

protected:

	ComPtr<ID3D12Resource> pNormalBuffer;		// ��ֺ����� ����
	ComPtr<ID3D12Resource> pNormalUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW normalBufferView;

	ComPtr<ID3D12Resource> pTextureCoordBuffer;		// UV������ ����
	ComPtr<ID3D12Resource> pTextureCoordUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW textureCoordBufferView;

public:		// �������� ��� �Լ���
	// ������ �� �Ҹ���
	BasicMesh();
	virtual ~BasicMesh();

public:		// ��� �Լ���

	// get, set�Լ�
	const string& GetName() const;
	const BoundingOrientedBox& GetOOBB() const;
	//void LoadFromFile(const string& _fileName, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

///////////////////////////////////////////////////////////////////////////////
/// boundingBox �޽�
class HitBoxMesh {
private:
	static shared_ptr<HitBoxMesh> hitBoxMesh;
public:
	static void MakeHitBoxMesh(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	static shared_ptr<HitBoxMesh> GetHitBoxMesh();
private:
	D3D12_PRIMITIVE_TOPOLOGY primitiveTopology;
	ComPtr<ID3D12Resource> pPositionBuffer;	// ���ؽ��� ��ġ ����
	ComPtr<ID3D12Resource> pPositionUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW positionBufferView;
	
	ComPtr<ID3D12Resource> pIndexBuffers;	// subMesh���� �ε��� ����
	ComPtr<ID3D12Resource> pIndexUploadBuffers;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
public:
	HitBoxMesh();
	~HitBoxMesh();
	void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

///////////////////////////////////////////////////////////////////////////////
///  ���̸�
class HeightMapImage {
public:
	static const int height = 255;
private:
	//���� �� �̹��� �ȼ�(8-��Ʈ)���� ������ �迭�̴�. �� �ȼ��� 0~255�� ���� ���´�. 
	 vector<vector<BYTE>> heightMapPixels;	// [x][z]
	//���� �� �̹����� ���ο� ����,���� ũ���̴�. 
	int width;
	int length;
	
	XMFLOAT3 scale;

	//���� �� �̹����� ������ �� �� Ȯ���Ͽ� ����� ���ΰ��� ��Ÿ���� ������ �����̴�. 
	//XMFLOAT3 m_xmf3Scale;	// ��� ���͸� ����ϱ����� �ʿ��ϴ�.
public:
	HeightMapImage();
	~HeightMapImage(void);
	bool LoadHeightMapFile(const string& _fileName, int _nWidth, int _nLength, const XMFLOAT3 _scale);
	//���� �� �̹������� (x, z) ��ġ�� �ȼ� ���� ����� ������ ���̸� ��ȯ�Ѵ�. 
	float GetHeight(float _terrainX, float _terrainZ) const;

	//���� �� �̹������� (x, z) ��ġ�� ���� ���͸� ��ȯ�Ѵ�. 
	XMFLOAT3 GetNormal(float _terrainX, float _terrainZ) const;
	int GetWidth() const;
	int GetLength() const;
};

///////////////////////////////////////////////////////////////////////////////
/// �ͷ��� �޽�
class TerrainMesh : public BasicMesh {
public:
	static shared_ptr<TerrainMesh> LoadFromFile(const string& _heightMapfileName, XMINT2 _imageSize, XMFLOAT3 _terrainSize, float _gridWidth, const VS_MaterialMappedFormat& _materialColors,
		float _detailWidth, const array<string, TEXTURETYPENUM>& _textureFileNames,
		const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

protected:
	ComPtr<ID3D12Resource> pTextureCoord2Buffer;		// UV������ ����
	ComPtr<ID3D12Resource> pTextureCoord2UploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW textureCoord2BufferView;

	HeightMapImage heightMapImage;
	XMFLOAT3 size;

public:
	TerrainMesh();
	virtual ~TerrainMesh();

	virtual void CreateVertex(const XMFLOAT3& terrainSize, float _gridWidth, const VS_MaterialMappedFormat& _materialColors, float _detailWidth, const array<string, TEXTURETYPENUM>& _textureFileNames, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	const HeightMapImage& GetHeightMapImage() const;
};

///////////////////////////////////////////////////////////////////////////////
/// ������ �޽�
class BillBoardMesh : public Mesh {
public:
	static shared_ptr<BillBoardMesh> LoadFromFile(const string& _meshName, const string& _textureFileName, const XMFLOAT2& _center, const XMFLOAT2& _size , const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

protected:

	ComPtr<ID3D12Resource> pSizeBuffer;	// �������� ��ġ ����
	ComPtr<ID3D12Resource> pSizeUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW sizeBufferView;

public:		// �������� ��� �Լ���
	// ������ �� �Ҹ���
	BillBoardMesh();
	virtual ~BillBoardMesh();

public:		// ��� �Լ���
	//void LoadFromFile(const string& _fileName, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};
