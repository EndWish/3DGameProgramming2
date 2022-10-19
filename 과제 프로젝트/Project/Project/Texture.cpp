#include "stdafx.h"
#include "Texture.h"
#include "Mesh.h"

shared_ptr<Texture> Texture::LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	shared_ptr<Texture> pNewTexture = make_shared<Texture>();

	// 텍스쳐 파일 이름 읽기
	ReadStringBinary(pNewTexture->name, _file);
	
	// DDS파일로 부터 텍스쳐 리소스 가져오기
	pNewTexture->LoadTextureFromDDSFile(_pDevice, _pCommandList, wstring(pNewTexture->name.begin(), pNewTexture->name.end()) );
	//Mesh::GetShader()->

	return pNewTexture;
}

Texture::Texture() {
	textureType = RESOURCE_TEXTURE2DARRAY;
}

Texture::~Texture() {

}

// get, set함수

void Texture::LoadTextureFromDDSFile(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, const wstring _fileName) {
	textureBuffer = CreateTextureResourceFromDDSFile(_pDevice, _pCommandList,L"Textures/" + _fileName, textureUploadBuffer, D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
	SetShaderResourceViewDesc();

	//if (textureBuffer && !srvGpuDescriptorHandles.ptr) {
	//	_pDevice->CreateShaderResourceView(textureBuffer.Get(), &shaderResourceViewDesc, m_d3dSrvCPUDescriptorNextHandle);
	//	m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

	//	pTexture->SetGpuDescriptorHandle(nIndex, m_d3dSrvGPUDescriptorNextHandle);
	//	m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	//}
}

void Texture::SetShaderResourceViewDesc() {
	D3D12_RESOURCE_DESC d3dResourceDesc = textureBuffer->GetDesc();

	shaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	switch (textureType) {
	case RESOURCE_TEXTURE2D: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 1)
	case RESOURCE_TEXTURE2D_ARRAY: //[]
		shaderResourceViewDesc.Format = d3dResourceDesc.Format;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		shaderResourceViewDesc.Texture2D.MipLevels = -1;
		shaderResourceViewDesc.Texture2D.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2D.PlaneSlice = 0;
		shaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_TEXTURE2DARRAY: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize != 1)
		shaderResourceViewDesc.Format = d3dResourceDesc.Format;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
		shaderResourceViewDesc.Texture2DArray.MipLevels = -1;
		shaderResourceViewDesc.Texture2DArray.MostDetailedMip = 0;
		shaderResourceViewDesc.Texture2DArray.PlaneSlice = 0;
		shaderResourceViewDesc.Texture2DArray.ResourceMinLODClamp = 0.0f;
		shaderResourceViewDesc.Texture2DArray.FirstArraySlice = 0;
		shaderResourceViewDesc.Texture2DArray.ArraySize = d3dResourceDesc.DepthOrArraySize;
		break;
	case RESOURCE_TEXTURE_CUBE: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 6)
		shaderResourceViewDesc.Format = d3dResourceDesc.Format;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		shaderResourceViewDesc.TextureCube.MipLevels = 1;
		shaderResourceViewDesc.TextureCube.MostDetailedMip = 0;
		shaderResourceViewDesc.TextureCube.ResourceMinLODClamp = 0.0f;
		break;
	case RESOURCE_BUFFER: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		shaderResourceViewDesc.Format = bufferFormat;
		shaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		shaderResourceViewDesc.Buffer.FirstElement = 0;
		shaderResourceViewDesc.Buffer.NumElements = bufferElement;
		shaderResourceViewDesc.Buffer.StructureByteStride = 0;
		shaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		break;
	}
}

/////////////////////////////////////////////////////////////////////////////////////
///	CTexture

CTexture::CTexture() {
	textureType = RESOURCE_TEXTURE2D;
	texturesMapType = 0;

	for (auto& srvGpuDescriptorHandle : m_pd3dSrvGpuDescriptorHandles)
		srvGpuDescriptorHandle.ptr = NULL;

	for (int& rootParameterIndice : m_pnRootParameterIndices)
		rootParameterIndice = -1;

	m_nSamplers = 0;
}

CTexture::CTexture(UINT _textureType, int nSamplers) {
	textureType = _textureType;

	for (auto& srvGpuDescriptorHandle : m_pd3dSrvGpuDescriptorHandles)
		srvGpuDescriptorHandle.ptr = NULL;

	for (int& rootParameterIndice : m_pnRootParameterIndices)
		rootParameterIndice = -1;

	m_nSamplers = nSamplers;
	if (m_nSamplers > 0)
		m_pd3dSamplerGpuDescriptorHandles.assign(m_nSamplers, {});
}

CTexture::~CTexture() {
	
}

void CTexture::SetRootParameterIndex(int nIndex, UINT nRootParameterIndex) {
	m_pnRootParameterIndices[nIndex] = nRootParameterIndex;
}

void CTexture::SetGpuDescriptorHandle(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle) {
	m_pd3dSrvGpuDescriptorHandles[nIndex] = d3dSrvGpuDescriptorHandle;
}

void CTexture::SetSampler(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSamplerGpuDescriptorHandle) {
	m_pd3dSamplerGpuDescriptorHandles[nIndex] = d3dSamplerGpuDescriptorHandle;
}

void CTexture::UpdateShaderVariables(const ComPtr<ID3D12GraphicsCommandList>& pd3dCommandList) {
	//[주의] : 쉐이더에 있는 것을 텍스쳐에 옮겨놨다.
	if (m_pd3dCbvSrvDescriptorHeap) {
		pd3dCommandList->SetDescriptorHeaps(1, m_pd3dCbvSrvDescriptorHeap.GetAddressOf());	// ???
	}
	
	for (int i = 0; i < TEXTURETYPENUM; i++) {
		if (m_pd3dSrvGpuDescriptorHandles[i].ptr && (m_pnRootParameterIndices[i] != -1)) {
			pd3dCommandList->SetGraphicsRootDescriptorTable(m_pnRootParameterIndices[i], m_pd3dSrvGpuDescriptorHandles[i]);
		}
	}
}

void CTexture::LoadTextureFromDDSFile(const ComPtr<ID3D12Device>& pd3dDevice, const ComPtr<ID3D12GraphicsCommandList>& pd3dCommandList, const wstring& _fileName, UINT nResourceType, UINT nIndex) {
	m_pnResourceTypes[nIndex] = nResourceType;
	pTextureBuffers[nIndex] = ::CreateTextureResourceFromDDSFile(pd3dDevice, pd3dCommandList, L"Textures/" + _fileName, pTextureUploadBuffers[nIndex], D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
	// [주의] : &pTextureUploadBuffers[nIndex] 를 pTextureUploadBuffers[nIndex]로 수정
	// TextureManager에서 가져오는 걸로 바꾸자.
}

void CTexture::LoadTextureFromFile(const ComPtr<ID3D12Device>& pd3dDevice, const ComPtr<ID3D12GraphicsCommandList>& pd3dCommandList, ifstream& _file) {
	for (int i = 0; i < TEXTURETYPENUM; ++i) {
		string testureNameBuffer;
		ReadStringBinary(testureNameBuffer, _file);	// 텍스쳐 파일의 이름을 읽는다.
		if (!testureNameBuffer.empty()) {	// 텍스쳐가 있을 경우
			texturesName[i].assign(testureNameBuffer.begin(), testureNameBuffer.end());	// 이름 저장

			// [수정요구] 메쉬에서 텍스쳐를 미리 만들어 같은 텍스쳐를 공유하도록 만들자.
			LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, texturesName[i].c_str(), RESOURCE_TEXTURE2D, i);
			CreateShaderResourceView(pd3dDevice, i);
			m_pnRootParameterIndices[i] = PARAMETER_STANDARD_TEXTURE + i;

			texturesMapType = texturesMapType | (1 << i);
		}
	}
}

void CTexture::LoadTextureFromDirect(const ComPtr<ID3D12Device>& pd3dDevice, const ComPtr<ID3D12GraphicsCommandList>& pd3dCommandList, int textureIndex, const string& _texturefileName) {
	texturesName[textureIndex].assign(_texturefileName.begin(), _texturefileName.end());	// 이름 저장

	// [수정요구] 메쉬에서 텍스쳐를 미리 만들어 같은 텍스쳐를 공유하도록 만들자.
	LoadTextureFromDDSFile(pd3dDevice, pd3dCommandList, texturesName[textureIndex].c_str(), RESOURCE_TEXTURE2D, textureIndex);
	CreateShaderResourceView(pd3dDevice, textureIndex);
	m_pnRootParameterIndices[textureIndex] = PARAMETER_STANDARD_TEXTURE + textureIndex;

	texturesMapType = texturesMapType | (1 << textureIndex);
}

void CTexture::CreateCbvSrvDescriptorHeaps(const ComPtr<ID3D12Device>& _pDevice) {
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

void CTexture::CreateShaderResourceView(const ComPtr<ID3D12Device>& _pDevice, int nIndex) {
	if (pTextureBuffers[nIndex] && !m_pd3dSrvGpuDescriptorHandles[nIndex].ptr) {
		D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc = GetShaderResourceViewDesc(nIndex);
		_pDevice->CreateShaderResourceView(pTextureBuffers[nIndex].Get(), &d3dShaderResourceViewDesc, m_d3dSrvCPUDescriptorNextHandle);
		m_d3dSrvCPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;

		SetGpuDescriptorHandle(nIndex, m_d3dSrvGPUDescriptorNextHandle);
		m_d3dSrvGPUDescriptorNextHandle.ptr += ::gnCbvSrvDescriptorIncrementSize;
	}
}

D3D12_SHADER_RESOURCE_VIEW_DESC CTexture::GetShaderResourceViewDesc(int nIndex) {
	D3D12_RESOURCE_DESC d3dResourceDesc = pTextureBuffers[nIndex]->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc;
	d3dShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	int nTextureType = GetTextureType(nIndex);
	switch (nTextureType) {
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
		d3dShaderResourceViewDesc.Format = m_pdxgiBufferFormats[nIndex];
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		d3dShaderResourceViewDesc.Buffer.FirstElement = 0;
		d3dShaderResourceViewDesc.Buffer.NumElements = m_pnBufferElements[nIndex];
		d3dShaderResourceViewDesc.Buffer.StructureByteStride = 0;
		d3dShaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		break;
	}
	return(d3dShaderResourceViewDesc);
}

