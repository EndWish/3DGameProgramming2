#pragma once

#include "Shader.h"
#include "Material.h"

///////////////////////////////////////////////////////////////////////////////
/// 메쉬
class Mesh {
public:
	static shared_ptr<Mesh> LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

protected:
	string name;

	D3D12_PRIMITIVE_TOPOLOGY primitiveTopology;
	UINT nVertex;	// 버텍스(정점과 노멀벡터)의 개수

	ComPtr<ID3D12Resource> pPositionBuffer;	// 버텍스의 위치 정보
	ComPtr<ID3D12Resource> pPositionUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW positionBufferView;

	vector<UINT> nSubMeshIndex;	// subMesh들의 인덱스 개수
	vector<ComPtr<ID3D12Resource>> pSubMeshIndexBuffers;	// subMesh들의 인덱스 정보
	vector<ComPtr<ID3D12Resource>> pSubMeshIndexUploadBuffers;
	vector<D3D12_INDEX_BUFFER_VIEW> subMeshIndexBufferViews;

	vector<shared_ptr<Material>> materials;
	BoundingOrientedBox oobb;

public:		// 생성관련 멤버 함수▼
	// 생성자 및 소멸자
	Mesh();
	virtual ~Mesh();

public:		// 멤버 함수▼

	// get, set함수
	const string& GetName() const;
	const BoundingOrientedBox& GetOOBB() const;
	//void LoadFromFile(const string& _fileName, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

///////////////////////////////////////////////////////////////////////////////
/// 기본 메쉬
class BasicMesh : public Mesh {
public:
	static shared_ptr<BasicMesh> LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

protected:

	ComPtr<ID3D12Resource> pNormalBuffer;		// 노멀벡터의 정보
	ComPtr<ID3D12Resource> pNormalUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW normalBufferView;

	ComPtr<ID3D12Resource> pTextureCoordBuffer;		// UV벡터의 정보
	ComPtr<ID3D12Resource> pTextureCoordUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW textureCoordBufferView;

public:		// 생성관련 멤버 함수▼
	// 생성자 및 소멸자
	BasicMesh();
	virtual ~BasicMesh();

public:		// 멤버 함수▼

	// get, set함수
	const string& GetName() const;
	const BoundingOrientedBox& GetOOBB() const;
	//void LoadFromFile(const string& _fileName, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

///////////////////////////////////////////////////////////////////////////////
/// boundingBox 메쉬
class HitBoxMesh {
private:
	static shared_ptr<HitBoxMesh> hitBoxMesh;
public:
	static void MakeHitBoxMesh(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	static shared_ptr<HitBoxMesh> GetHitBoxMesh();
private:
	D3D12_PRIMITIVE_TOPOLOGY primitiveTopology;
	ComPtr<ID3D12Resource> pPositionBuffer;	// 버텍스의 위치 정보
	ComPtr<ID3D12Resource> pPositionUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW positionBufferView;
	
	ComPtr<ID3D12Resource> pIndexBuffers;	// subMesh들의 인덱스 정보
	ComPtr<ID3D12Resource> pIndexUploadBuffers;
	D3D12_INDEX_BUFFER_VIEW indexBufferView;
public:
	HitBoxMesh();
	~HitBoxMesh();
	void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

///////////////////////////////////////////////////////////////////////////////
///  높이맵
class HeightMapImage {
public:
	static const int height = 255;
private:
	//높이 맵 이미지 픽셀(8-비트)들의 이차원 배열이다. 각 픽셀은 0~255의 값을 갖는다. 
	 vector<vector<BYTE>> heightMapPixels;	// [x][z]
	//높이 맵 이미지의 가로와 세로,높이 크기이다. 
	int width;
	int length;
	
	XMFLOAT3 scale;

	//높이 맵 이미지를 실제로 몇 배 확대하여 사용할 것인가를 나타내는 스케일 벡터이다. 
	//XMFLOAT3 m_xmf3Scale;	// 노멀 벡터를 계산하기위해 필요하다.
public:
	HeightMapImage();
	~HeightMapImage(void);
	bool LoadHeightMapFile(const string& _fileName, int _nWidth, int _nLength, const XMFLOAT3 _scale);
	//높이 맵 이미지에서 (x, z) 위치의 픽셀 값에 기반한 지형의 높이를 반환한다. 
	float GetHeight(float _terrainX, float _terrainZ) const;

	//높이 맵 이미지에서 (x, z) 위치의 법선 벡터를 반환한다. 
	XMFLOAT3 GetNormal(float _terrainX, float _terrainZ) const;
	int GetWidth() const;
	int GetLength() const;
};

///////////////////////////////////////////////////////////////////////////////
/// 터레인 메쉬
class TerrainMesh : public BasicMesh {
public:
	static shared_ptr<TerrainMesh> LoadFromFile(const string& _heightMapfileName, XMINT2 _imageSize, XMFLOAT3 _terrainSize, float _gridWidth, const VS_MaterialMappedFormat& _materialColors,
		float _detailWidth, const array<string, TEXTURETYPENUM>& _textureFileNames,
		const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

protected:
	ComPtr<ID3D12Resource> pTextureCoord2Buffer;		// UV벡터의 정보
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
/// 빌보드 메쉬
class BillBoardMesh : public Mesh {
public:
	static shared_ptr<BillBoardMesh> LoadFromFile(const string& _meshName, const string& _textureFileName, const XMFLOAT2& _center, const XMFLOAT2& _size , const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

protected:

	ComPtr<ID3D12Resource> pSizeBuffer;	// 사이즈의 위치 정보
	ComPtr<ID3D12Resource> pSizeUploadBuffer;
	D3D12_VERTEX_BUFFER_VIEW sizeBufferView;

public:		// 생성관련 멤버 함수▼
	// 생성자 및 소멸자
	BillBoardMesh();
	virtual ~BillBoardMesh();

public:		// 멤버 함수▼
	//void LoadFromFile(const string& _fileName, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};
