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
	static shared_ptr<Texture> LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

public:
	Texture();
	virtual ~Texture();

private:
	UINT textureType;

	string name;
	ComPtr<ID3D12Resource> textureBuffer;
	ComPtr<ID3D12Resource> textureUploadBuffer;
	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc;

	//UINT* m_pnResourceTypes = NULL;

	DXGI_FORMAT bufferFormat;
	int bufferElement;

	//int m_nRootParameters = 0;
	//int m_pnRootParameterIndices = NULL;
	D3D12_GPU_DESCRIPTOR_HANDLE srvGpuDescriptorHandles;

	//int								m_nSamplers = 0;
	//D3D12_GPU_DESCRIPTOR_HANDLE* m_pd3dSamplerGpuDescriptorHandles = NULL;

	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dCbvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dCbvGPUDescriptorStartHandle;
	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dSrvCPUDescriptorStartHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dSrvGPUDescriptorStartHandle;

	D3D12_CPU_DESCRIPTOR_HANDLE			m_d3dSrvCPUDescriptorNextHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE			m_d3dSrvGPUDescriptorNextHandle;

public:

	void LoadTextureFromDDSFile(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, const wstring _fileName);
	void SetShaderResourceViewDesc();
};

///////////////////////////////////////////////////////////////////////////////
/// CTexture

class CTexture {
public:
	CTexture();
	CTexture(UINT nResourceType, int nSamplers);
	virtual ~CTexture();

private:

	// int m_nTextures	// 텍스쳐종류의 개수

	UINT textureType;
	UINT texturesMapType;

	array<wstring, TEXTURETYPENUM> texturesName;
	array<ComPtr<ID3D12Resource>, TEXTURETYPENUM> pTextureBuffers;
	array<ComPtr<ID3D12Resource>, TEXTURETYPENUM> pTextureUploadBuffers;

	//_TCHAR(*m_ppstrTextureNames)[64] = NULL;
	//ID3D12Resource** m_ppd3dTextures = NULL;
	//ID3D12Resource** m_ppd3dTextureUploadBuffers;

	//UINT* m_pnResourceTypes = NULL;
	array<UINT, TEXTURETYPENUM> m_pnResourceTypes;
	
	//DXGI_FORMAT* m_pdxgiBufferFormats = NULL;
	array<DXGI_FORMAT, TEXTURETYPENUM> m_pdxgiBufferFormats;

	//int* m_pnBufferElements = NULL;
	array<int, TEXTURETYPENUM> m_pnBufferElements;

	//int	m_nRootParameters = 0;	// 루트 파라미터 개수 == TEXTURETYPENUM 로 설정
	//int* m_pnRootParameterIndices = NULL;
	array<int, TEXTURETYPENUM> m_pnRootParameterIndices;
	array<D3D12_GPU_DESCRIPTOR_HANDLE, TEXTURETYPENUM> m_pd3dSrvGpuDescriptorHandles;

	int	m_nSamplers = 0;
	//D3D12_GPU_DESCRIPTOR_HANDLE* m_pd3dSamplerGpuDescriptorHandles = NULL;
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

	const wstring& GetTextureName(int nIndex) { return(texturesName[nIndex]); }
	ComPtr<ID3D12Resource> GetResource(int nIndex) { return(pTextureBuffers[nIndex]); }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGpuDescriptorHandle(int nIndex) { return(m_pd3dSrvGpuDescriptorHandles[nIndex]); }
	int GetRootParameter(int nIndex) { return(m_pnRootParameterIndices[nIndex]); }

	UINT GetTextureType() { return(textureType); }
	UINT GetTextureType(int nIndex) { return(m_pnResourceTypes[nIndex]); }
	UINT GetTexturesMapType() { return texturesMapType; }
	DXGI_FORMAT GetBufferFormat(int nIndex) { return(m_pdxgiBufferFormats[nIndex]); }
	int GetBufferElements(int nIndex) { return(m_pnBufferElements[nIndex]); }

	D3D12_SHADER_RESOURCE_VIEW_DESC GetShaderResourceViewDesc(int nIndex);

	// 리소스뷰, 서술자 힙, 테이블을 만들기 위한 함수
	void CreateCbvSrvDescriptorHeaps(const ComPtr<ID3D12Device>& _pDevice);
	void CreateShaderResourceView(const ComPtr<ID3D12Device>& _pDevice, int nIndex);

};