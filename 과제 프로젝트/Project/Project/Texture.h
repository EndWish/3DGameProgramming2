#pragma once

#define RESOURCE_TEXTURE2D			0x01
#define RESOURCE_TEXTURE2D_ARRAY	0x02	//[]
#define RESOURCE_TEXTURE2DARRAY		0x03
#define RESOURCE_TEXTURE_CUBE		0x04
#define RESOURCE_BUFFER				0x05

#define TEXTURETYPENUM				7
#define MATERIAL_ALBEDO_MAP			0x01
#define MATERIAL_SPECULAR_MAP		0x02
#define MATERIAL_NORMAL_MAP			0x04
#define MATERIAL_METALLIC_MAP		0x08
#define MATERIAL_EMISSION_MAP		0x10
#define MATERIAL_DETAIL_ALBEDO_MAP	0x20
#define MATERIAL_DETAIL_NORMAL_MAP	0x40

#define MATERIAL_ALBEDO_MAP_INDEX			0
#define MATERIAL_SPECULAR_MAP_INDEX			1
#define MATERIAL_NORMAL_MAP_INDEX			2
#define MATERIAL_METALLIC_MAP_INDEX			3
#define MATERIAL_EMISSION_MAP_INDEX			4
#define MATERIAL_DETAIL_ALBEDO_MAP_INDEX	5
#define MATERIAL_DETAIL_NORMAL_MAP_INDEX	6

class Shader;

class Texture {
public:
	static shared_ptr<Texture> Load(const wstring& _fileName, UINT _resourceType, UINT _rootParameterIndex, UINT _textureMapType, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
public:
	Texture();
	virtual ~Texture();

private:
	UINT resourceType;		// 예) RESOURCE_TEXTURE2D
	UINT textureMapType;	// 예) MATERIAL_ALBEDO_MAP

	wstring name;
	ComPtr<ID3D12Resource> pTextureBuffer;
	ComPtr<ID3D12Resource> pTextureUploadBuffer;
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

	DXGI_FORMAT bufferFormat;
	int bufferElement;

	int rootParameterIndex;
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuDescriptorHandle;

public:
	// get set 함수
	const wstring& GetName() const;
	ComPtr<ID3D12Resource> GetTextureBuffer();
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvGpuDescriptorHandle();
	int GetRootParameterIndex();
	UINT GetResourceType();
	UINT GetTextureMapType();
	DXGI_FORMAT GetBufferFormat();
	int GetBufferElements();
	D3D12_SHADER_RESOURCE_VIEW_DESC GetShaderResourceViewDesc();

	void SetName(const wstring& _name);
	void SetName(const string& _name);
	void SetTextureMapType(UINT _textureMapType);
	void SetRootParameterIndex(UINT _nRootParameterIndex);
	void SetGpuDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE _srvGpuDescriptorHandle);

	void LoadTextureFromDDSFile(const ComPtr<ID3D12Device>& pDevice, const ComPtr<ID3D12GraphicsCommandList>& pCommandList, const wstring& _fileName, UINT nResourceType);
};

///////////////////////////////////////////////////////////////////////////////
/// TextureBundle TextureBundle

class TextureBundle {
public:
	TextureBundle();
	TextureBundle(UINT nResourceType, int nSamplers);
	virtual ~TextureBundle();

private:

	// int m_nTextures	// 텍스쳐종류의 개수

	UINT texturesMapType;	// 예) MATERIAL_ALBEDO_MAP | MATERIAL_SPECULAR_MAP

	array<shared_ptr<Texture>, TEXTURETYPENUM> pTextures;

	int	m_nSamplers = 0;
	vector<D3D12_GPU_DESCRIPTOR_HANDLE> m_pd3dSamplerGpuDescriptorHandles;

	// 서술자 힙, 테이블을 만들기 위한 변수들
	ComPtr<ID3D12DescriptorHeap> m_pd3dCbvSrvDescriptorHeap;

	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dSrvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dSrvGPUDescriptorStartHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dSrvCPUDescriptorNextHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dSrvGPUDescriptorNextHandle;
public:

	void SetSampler(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSamplerGpuDescriptorHandle);

	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>&, int nParameterIndex, int nTextureIndex);
	void UpdateShaderVariables(const ComPtr<ID3D12GraphicsCommandList>& pd3dCommandList);

	void LoadTextureFromDDSFile(const ComPtr<ID3D12Device>& pd3dDevice, const ComPtr<ID3D12GraphicsCommandList>&, const wstring& _fileName, UINT nResourceType, UINT nIndex);

	void LoadTextureFromFile(const ComPtr<ID3D12Device>& pd3dDevice, const ComPtr<ID3D12GraphicsCommandList>& pd3dCommandList, ifstream& _file);
	void LoadTextureFromDirect(const ComPtr<ID3D12Device>& pd3dDevice, const ComPtr<ID3D12GraphicsCommandList>& pd3dCommandList, int textureIndex, const string& _texturefileName);

	void SetRootParameterIndex(int nIndex, UINT nRootParameterIndex);
	void SetGpuDescriptorHandle(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle);

	const wstring& GetTextureName(int nIndex) { return pTextures[nIndex]->GetName(); }
	ComPtr<ID3D12Resource> GetTextureBuffer(int nIndex) { return pTextures[nIndex]->GetTextureBuffer(); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(int nIndex) { return pTextures[nIndex]->GetSrvGpuDescriptorHandle(); }
	int GetRootParameter(int nIndex) { return pTextures[nIndex]->GetRootParameterIndex(); }

	UINT GetResourceType(int nIndex) { return pTextures[nIndex]->GetResourceType(); }
	UINT GetTexturesMapType() { return texturesMapType; }
	DXGI_FORMAT GetBufferFormat(int nIndex) { return pTextures[nIndex]->GetBufferFormat(); }
	int GetBufferElements(int nIndex) { return pTextures[nIndex]->GetBufferElements(); }

	D3D12_SHADER_RESOURCE_VIEW_DESC GetShaderResourceViewDesc(int nIndex) { return pTextures[nIndex]->GetShaderResourceViewDesc(); }

	// 리소스뷰, 서술자 힙, 테이블을 만들기 위한 함수
	void CreateCbvSrvDescriptorHeaps(const ComPtr<ID3D12Device>& _pDevice);
	void CreateShaderResourceView(const ComPtr<ID3D12Device>& _pDevice, int nIndex);

};

///////////////////////////////////////////////////////////////////////////////
/// TexturePicker

class TexturePicker {
private:
	static unordered_map<wstring, shared_ptr<Texture>> storage;
	static int useCount;

public:
	static shared_ptr<Texture> GetTexture(const wstring& _fileName, UINT _resourceType, UINT _rootParameterIndex, UINT _textureMapType, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	static void UseCountUp();	// 사용하기 위해 카운드를 올린다.
	static void UseCountDown();	// 사용을 다하면 카운드를 내린다. 카운드카 0이 될때 포인터를 모두 없앤다.
};
