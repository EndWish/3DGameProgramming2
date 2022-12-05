#include "stdafx.h"
#include "Texture.h"
#include "Mesh.h"

shared_ptr<Texture> Texture::Load(const wstring& _fileName, UINT _resourceType, UINT _rootParameterIndex, UINT _textureMapType, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	shared_ptr<Texture> newTexture = make_shared<Texture>();
	newTexture->SetName(_fileName);
	newTexture->LoadTextureFromDDSFile(_pDevice, _pCommandList, _fileName, _resourceType);
	newTexture->SetRootParameterIndex(_rootParameterIndex);
	newTexture->SetTextureMapType(_textureMapType);
	return newTexture;
}

Texture::Texture() {
	resourceType = RESOURCE_TEXTURE2D;
	textureMapType = 0;

	name = L"UNKNOWN";
	shaderResourceViewDesc = D3D12_SHADER_RESOURCE_VIEW_DESC();

	bufferFormat = DXGI_FORMAT();
	bufferElement = 0;

	rootParameterIndex = -1;
	srvGpuDescriptorHandle = D3D12_GPU_DESCRIPTOR_HANDLE();
	srvGpuDescriptorHandle.ptr = NULL;
}
Texture::~Texture() {

}

// get, set함수
const wstring& Texture::GetName() const {
	return name;
}
ComPtr<ID3D12Resource> Texture::GetTextureBuffer() {
	return pTextureBuffer;
}
D3D12_GPU_DESCRIPTOR_HANDLE Texture::GetSrvGpuDescriptorHandle() { 
	return srvGpuDescriptorHandle;
}
int Texture::GetRootParameterIndex() { 
	return rootParameterIndex; 
}
UINT Texture::GetResourceType() { 
	return resourceType;
}
UINT Texture::GetTextureMapType() { 
	return textureMapType; 
}
DXGI_FORMAT Texture::GetBufferFormat() { 
	return bufferFormat; 
}
int Texture::GetBufferElements() { 
	return bufferElement; 
}
D3D12_SHADER_RESOURCE_VIEW_DESC Texture::GetShaderResourceViewDesc() {
	D3D12_RESOURCE_DESC d3dResourceDesc = pTextureBuffer->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc;
	d3dShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	switch (resourceType) {
	case RESOURCE_TEXTURE2D: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 1)
	case RESOURCE_TEXTURE2D_ARRAY: //[]
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		d3dShaderResourceViewDesc.Texture2D.MipLevels = -1;
		d3dShaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.Texture2D.PlaneSlice = 0;
		d3dShaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_TEXTURE2DARRAY: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize != 1)
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		d3dShaderResourceViewDesc.Texture2DArray.MipLevels = -1;
		d3dShaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.Texture2DArray.PlaneSlice = 0;
		d3dShaderResourceViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		d3dShaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
		d3dShaderResourceViewDesc.Texture2DArray.ArraySize = d3dResourceDesc.DepthOrArraySize;
		break;
	case RESOURCE_TEXTURE_CUBE: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 6)
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		d3dShaderResourceViewDesc.TextureCube.MipLevels = 1;
		d3dShaderResourceViewDesc.TextureCube.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_BUFFER: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		d3dShaderResourceViewDesc.Format = bufferFormat;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		d3dShaderResourceViewDesc.Buffer.FirstElement = 0;
		d3dShaderResourceViewDesc.Buffer.NumElements = bufferElement;
		d3dShaderResourceViewDesc.Buffer.StructureByteStride = 0;
		d3dShaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		break;
	}
	return(d3dShaderResourceViewDesc);
}

void Texture::SetName(const wstring& _name) {
	name = _name;
}
void Texture::SetName(const string& _name) {
	name.assign(_name.begin(), _name.end());
}
void Texture::SetTextureMapType(UINT _textureMapType) {
	textureMapType = _textureMapType;
}
void Texture::SetRootParameterIndex(UINT _nRootParameterIndex) {
	rootParameterIndex = _nRootParameterIndex;
}
void Texture::SetGpuDescriptorHandle(D3D12_GPU_DESCRIPTOR_HANDLE _srvGpuDescriptorHandle) {
	srvGpuDescriptorHandle = _srvGpuDescriptorHandle;
}

void Texture::LoadTextureFromDDSFile(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, const wstring& _fileName, UINT _resourceType) {
	resourceType = _resourceType;
	pTextureBuffer = ::CreateTextureResourceFromDDSFile(_pDevice, _pCommandList, L"Textures/" + _fileName, pTextureUploadBuffer, D3D12_RESOURCE_STATE_GENERIC_READ);
	cout << string(_fileName.begin(), _fileName.end())  << "\n";
}

void Texture::CreateTexture2DResource(const ComPtr<ID3D12Device>& _pDevice, UINT _rootParameterIndex,  UINT _width, UINT _height, UINT _elements, UINT _mipLevels, DXGI_FORMAT _dxgiFormat, D3D12_RESOURCE_FLAGS _resourceFlags, D3D12_RESOURCE_STATES _resourceStates, D3D12_CLEAR_VALUE pClearValue) {
	rootParameterIndex = _rootParameterIndex;
	bufferFormat = _dxgiFormat;
	bufferElement = _elements;
	resourceType = RESOURCE_TEXTURE2D;
	pTextureBuffer = ::CreateTexture2DResource(_pDevice, _width, _height, _elements, _mipLevels, _dxgiFormat, _resourceFlags, _resourceStates, &pClearValue);
}

/////////////////////////////////////////////////////////////////////////////////////
///	TextureBundle

TextureBundle::TextureBundle() {
	texturesMapType = 0;

	m_d3dSrvCPUDescriptorStartHandle = D3D12_CPU_DESCRIPTOR_HANDLE();
	m_d3dSrvGPUDescriptorStartHandle = D3D12_GPU_DESCRIPTOR_HANDLE();

	m_d3dSrvCPUDescriptorNextHandle = D3D12_CPU_DESCRIPTOR_HANDLE();
	m_d3dSrvGPUDescriptorNextHandle = D3D12_GPU_DESCRIPTOR_HANDLE();

	m_nSamplers = 0;
}

TextureBundle::TextureBundle(UINT _textureType, int nSamplers) {
	texturesMapType = 0;

	m_d3dSrvCPUDescriptorStartHandle = D3D12_CPU_DESCRIPTOR_HANDLE();
	m_d3dSrvGPUDescriptorStartHandle = D3D12_GPU_DESCRIPTOR_HANDLE();

	m_d3dSrvCPUDescriptorNextHandle = D3D12_CPU_DESCRIPTOR_HANDLE();
	m_d3dSrvGPUDescriptorNextHandle = D3D12_GPU_DESCRIPTOR_HANDLE();

	m_nSamplers = nSamplers;
	if (m_nSamplers > 0)
		m_pd3dSamplerGpuDescriptorHandles.assign(m_nSamplers, {});
}

TextureBundle::~TextureBundle() {
	
}

void TextureBundle::SetRootParameterIndex(int nIndex, UINT nRootParameterIndex) {
	pTextures[nIndex]->SetRootParameterIndex(nRootParameterIndex);
}
void TextureBundle::SetGpuDescriptorHandle(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle) {
	pTextures[nIndex]->SetGpuDescriptorHandle(d3dSrvGpuDescriptorHandle);
}
void TextureBundle::SetSampler(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSamplerGpuDescriptorHandle) {
	m_pd3dSamplerGpuDescriptorHandles[nIndex] = d3dSamplerGpuDescriptorHandle;
}

void TextureBundle::UpdateShaderVariables(const ComPtr<ID3D12GraphicsCommandList>& pd3dCommandList) {
	//[주의] : 쉐이더에 있는 것을 텍스쳐에 옮겨놨다.
	if (m_pd3dCbvSrvDescriptorHeap) {
		pd3dCommandList->SetDescriptorHeaps(1, m_pd3dCbvSrvDescriptorHeap.GetAddressOf());
	}
	
	for (shared_ptr<Texture>& texture : pTextures) {
		if (texture && texture->GetSrvGpuDescriptorHandle().ptr) {
			// 
			pd3dCommandList->SetGraphicsRootDescriptorTable(texture->GetRootParameterIndex(), texture->GetSrvGpuDescriptorHandle());
		}
	}
}
void TextureBundle::LoadTextureFromFile(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, ifstream& _file) {
	
	for (int i = 0; i < TEXTURETYPENUM; ++i) {
		string testureNameBuffer;
		ReadStringBinary(testureNameBuffer, _file);	// 텍스쳐 파일의 이름을 읽는다.
		if (!testureNameBuffer.empty()) {	// 텍스쳐가 있을 경우
			wstring testureWNameBuffer(testureNameBuffer.begin(), testureNameBuffer.end());
			pTextures[i] = TexturePicker::GetTexture(testureWNameBuffer, RESOURCE_TEXTURE2D, PARAMETER_STANDARD_TEXTURE + i, 1 << i, _pDevice, _pCommandList);// Texture::Load(testureWNameBuffer, RESOURCE_TEXTURE2D, PARAMETER_STANDARD_TEXTURE + i, 1 << i, _pDevice, _pCommandList);
			CreateShaderResourceView(_pDevice, i);

			texturesMapType = texturesMapType | pTextures[i]->GetTextureMapType();
		}
	}
}
void TextureBundle::LoadTextureFromDirect(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, int textureIndex, const string& _texturefileName) {
	wstring texturefileWName(_texturefileName.begin(), _texturefileName.end());
	pTextures[textureIndex] = TexturePicker::GetTexture(texturefileWName, RESOURCE_TEXTURE2D, PARAMETER_STANDARD_TEXTURE + textureIndex, 1 << textureIndex, _pDevice, _pCommandList); //Texture::Load(texturefileWName, RESOURCE_TEXTURE2D, PARAMETER_STANDARD_TEXTURE + textureIndex, 1 << textureIndex, _pDevice, _pCommandList);
	CreateShaderResourceView(_pDevice, textureIndex);
	texturesMapType = texturesMapType | pTextures[textureIndex]->GetTextureMapType();
}

void TextureBundle::CreateCbvSrvDescriptorHeaps(const ComPtr<ID3D12Device>& _pDevice) {
	D3D12_DESCRIPTOR_HEAP_DESC d3dDescriptorHeapDesc;
	d3dDescriptorHeapDesc.NumDescriptors = 7; // SRVs 
	d3dDescriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	d3dDescriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	d3dDescriptorHeapDesc.NodeMask = 0;
	_pDevice->CreateDescriptorHeap(&d3dDescriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_pd3dCbvSrvDescriptorHeap);

	m_d3dSrvCPUDescriptorStartHandle = m_pd3dCbvSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_d3dSrvGPUDescriptorStartHandle = m_pd3dCbvSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart();

	m_d3dSrvCPUDescriptorNextHandle = m_d3dSrvCPUDescriptorStartHandle;
	m_d3dSrvGPUDescriptorNextHandle = m_d3dSrvGPUDescriptorStartHandle;
}
void TextureBundle::CreateShaderResourceView(const ComPtr<ID3D12Device>& _pDevice, int nIndex) {

	if (pTextures[nIndex] && pTextures[nIndex]->GetTextureBuffer() && !pTextures[nIndex]->GetSrvGpuDescriptorHandle().ptr) {
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = GetShaderResourceViewDesc(nIndex);
		_pDevice->CreateShaderResourceView(pTextures[nIndex]->GetTextureBuffer().Get(), &d3dShaderResourceViewDesc, m_d3dSrvCPUDescriptorNextHandle);
		m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		SetGpuDescriptorHandle(nIndex, m_d3dSrvGPUDescriptorNextHandle);
		m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	}
}

///////////////////////////////////////////////////////////////////////////////
/// TexturePicker
unordered_map<wstring, shared_ptr<Texture>> TexturePicker::storage;
int TexturePicker::useCount;

shared_ptr<Texture> TexturePicker::GetTexture(const wstring& _fileName, UINT _resourceType, UINT _rootParameterIndex, UINT _textureMapType, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {

	// 없을 경우 추가
	if (!storage.contains(_fileName)) {
		// 텍스쳐를 불러오도록 하자
		shared_ptr<Texture> newTexture = Texture::Load(_fileName, _resourceType, _rootParameterIndex, _textureMapType, _pDevice, _pCommandList);
		if (newTexture) {
			storage[_fileName] = newTexture;
		}
	}

	// 스토리지 내 오브젝트 정보와 같은 오브젝트를 복사하여 생성한다.
	return storage[_fileName];
}
void TexturePicker::UseCountUp() {
	++useCount;
}
void TexturePicker::UseCountDown() {
	if (--useCount == 0) {
		storage.clear();
	}
}
