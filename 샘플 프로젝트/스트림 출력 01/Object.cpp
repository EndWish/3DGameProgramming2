//-----------------------------------------------------------------------------
// File: CGameObject.cpp
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include "Object.h"
#include "Shader.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CTexture::CTexture(int nTextures, UINT nTextureType, int nSamplers, int nRootParameters)
{
	m_nTextureType = nTextureType;

	m_nTextures = nTextures;
	if (m_nTextures > 0)
	{
		m_ppd3dTextures = new ID3D12Resource * [m_nTextures];
		for (int i = 0; i < m_nTextures; i++) m_ppd3dTextures[i] = NULL;
		m_ppd3dTextureUploadBuffers = new ID3D12Resource * [m_nTextures];
		for (int i = 0; i < m_nTextures; i++) m_ppd3dTextureUploadBuffers[i] = NULL;
		m_pd3dSrvGpuDescriptorHandles = new D3D12_GPU_DESCRIPTOR_HANDLE[m_nTextures];
		for (int i = 0; i < m_nTextures; i++) m_pd3dSrvGpuDescriptorHandles[i].ptr = NULL;

		m_pnResourceTypes = new UINT[m_nTextures];
		for (int i = 0; i < m_nTextures; i++) m_pnResourceTypes[i] = 0;
		m_pdxgiBufferFormats = new DXGI_FORMAT[m_nTextures];
		for (int i = 0; i < m_nTextures; i++) m_pdxgiBufferFormats[i] = DXGI_FORMAT_UNKNOWN;
		m_pnBufferElements = new int[m_nTextures];
		for (int i = 0; i < m_nTextures; i++) m_pnBufferElements[i] = 0;
		m_pnBufferStrides = new int[m_nTextures];
		for (int i = 0; i < m_nTextures; i++) m_pnBufferStrides[i] = 0;
	}
	m_nRootParameters = nRootParameters;
	if (nRootParameters > 0) m_pnRootParameterIndices = new int[nRootParameters];
	for (int i = 0; i < m_nRootParameters; i++) m_pnRootParameterIndices[i] = -1;

	m_nSamplers = nSamplers;
	if (m_nSamplers > 0) m_pd3dSamplerGpuDescriptorHandles = new D3D12_GPU_DESCRIPTOR_HANDLE[m_nSamplers];
}

CTexture::~CTexture()
{
	if (m_ppd3dTextures)
	{
		for (int i = 0; i < m_nTextures; i++) if (m_ppd3dTextures[i]) m_ppd3dTextures[i]->Release();
		delete[] m_ppd3dTextures;
	}
	if (m_pnResourceTypes) delete[] m_pnResourceTypes;
	if (m_pdxgiBufferFormats) delete[] m_pdxgiBufferFormats;
	if (m_pnBufferElements) delete[] m_pnBufferElements;
	if (m_pnBufferStrides) delete[] m_pnBufferStrides;

	if (m_pnRootParameterIndices) delete[] m_pnRootParameterIndices;
	if (m_pd3dSrvGpuDescriptorHandles) delete[] m_pd3dSrvGpuDescriptorHandles;

	if (m_pd3dSamplerGpuDescriptorHandles) delete[] m_pd3dSamplerGpuDescriptorHandles;
}

void CTexture::SetRootParameterIndex(int nIndex, UINT nRootParameterIndex)
{
	m_pnRootParameterIndices[nIndex] = nRootParameterIndex;
}

void CTexture::SetGpuDescriptorHandle(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSrvGpuDescriptorHandle)
{
	m_pd3dSrvGpuDescriptorHandles[nIndex] = d3dSrvGpuDescriptorHandle;
}

void CTexture::SetSampler(int nIndex, D3D12_GPU_DESCRIPTOR_HANDLE d3dSamplerGpuDescriptorHandle)
{
	m_pd3dSamplerGpuDescriptorHandles[nIndex] = d3dSamplerGpuDescriptorHandle;
}

void CTexture::UpdateShaderVariables(ID3D12GraphicsCommandList* pd3dCommandList)
{
	if (m_nRootParameters == m_nTextures)
	{
		for (int i = 0; i < m_nRootParameters; i++)
		{
			pd3dCommandList->SetGraphicsRootDescriptorTable(m_pnRootParameterIndices[i], m_pd3dSrvGpuDescriptorHandles[i]);
		}
	}
	else
	{
		pd3dCommandList->SetGraphicsRootDescriptorTable(m_pnRootParameterIndices[0], m_pd3dSrvGpuDescriptorHandles[0]);
	}
}

void CTexture::UpdateShaderVariable(ID3D12GraphicsCommandList* pd3dCommandList, int nParameterIndex, int nTextureIndex)
{
	pd3dCommandList->SetGraphicsRootDescriptorTable(m_pnRootParameterIndices[nParameterIndex], m_pd3dSrvGpuDescriptorHandles[nTextureIndex]);
}

void CTexture::ReleaseShaderVariables()
{
}

void CTexture::ReleaseUploadBuffers()
{
	if (m_ppd3dTextureUploadBuffers)
	{
		for (int i = 0; i < m_nTextures; i++) if (m_ppd3dTextureUploadBuffers[i]) m_ppd3dTextureUploadBuffers[i]->Release();
		delete[] m_ppd3dTextureUploadBuffers;
		m_ppd3dTextureUploadBuffers = NULL;
	}
}

void CTexture::LoadTextureFromFile(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, wchar_t* pszFileName, UINT nResourceType, UINT nIndex)
{
	m_pnResourceTypes[nIndex] = nResourceType;
	m_ppd3dTextures[nIndex] = ::CreateTextureResourceFromDDSFile(pd3dDevice, pd3dCommandList, pszFileName, &m_ppd3dTextureUploadBuffers[nIndex], D3D12_RESOURCE_STATE_GENERIC_READ/*D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE*/);
}

void CTexture::CreateBuffer(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nElements, UINT nStride, DXGI_FORMAT ndxgiFormat, D3D12_HEAP_TYPE d3dHeapType, D3D12_RESOURCE_STATES d3dResourceStates, UINT nIndex)
{
	m_pnResourceTypes[nIndex] = RESOURCE_BUFFER;
	m_pdxgiBufferFormats[nIndex] = ndxgiFormat;
	m_pnBufferElements[nIndex] = nElements;
	m_pnBufferStrides[nIndex] = nStride;
	m_ppd3dTextures[nIndex] = ::CreateBufferResource(pd3dDevice, pd3dCommandList, pData, nElements * nStride, d3dHeapType, d3dResourceStates, &m_ppd3dTextureUploadBuffers[nIndex]);
}

void CTexture::CreateTexture(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nBytes, UINT nResourceType, UINT nWidth, UINT nHeight, UINT nDepthOrArraySize, UINT nMipLevels, D3D12_RESOURCE_FLAGS d3dResourceFlags, DXGI_FORMAT dxgiFormat, UINT nIndex)
{
	m_pnResourceTypes[nIndex] = nResourceType;
	m_pdxgiBufferFormats[nIndex] = dxgiFormat;
	D3D12_RESOURCE_DIMENSION d3dResourceDimension = (nResourceType == RESOURCE_TEXTURE2D) ? D3D12_RESOURCE_DIMENSION_TEXTURE2D : D3D12_RESOURCE_DIMENSION_TEXTURE1D;
	m_ppd3dTextures[nIndex] = ::CreateTextureResource(pd3dDevice, pd3dCommandList, pData, nBytes, d3dResourceDimension, nWidth, nHeight, nDepthOrArraySize, nMipLevels, d3dResourceFlags, dxgiFormat, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_GENERIC_READ, &m_ppd3dTextureUploadBuffers[nIndex]);
}

D3D12_SHADER_RESOURCE_VIEW_DESC CTexture::GetShaderResourceViewDesc(int nIndex)
{
	ID3D12Resource* pShaderResource = GetResource(nIndex);
	D3D12_RESOURCE_DESC d3dResourceDesc = pShaderResource->GetDesc();

	D3D12_SHADER_RESOURCE_VIEW_DESC d3dShaderResourceViewDesc;
	d3dShaderResourceViewDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	int nResourceType = GetResourceType(nIndex);
	switch (nResourceType)
	{
	case RESOURCE_TEXTURE1D: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_TEXTURE2D)(d3dResourceDesc.DepthOrArraySize == 1)
		d3dShaderResourceViewDesc.Format = d3dResourceDesc.Format;
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
		d3dShaderResourceViewDesc.Texture1D.MipLevels = -1;
		d3dShaderResourceViewDesc.Texture1D.MostDetailedMip = 0;
		d3dShaderResourceViewDesc.Texture2D.ResourceMinLODClamp = 0.0f;
		break;
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
		d3dShaderResourceViewDesc.TextureCube.MipLevels = -1;
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
	case RESOURCE_STRUCTURED_BUFFER: //(d3dResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
		d3dShaderResourceViewDesc.Format = m_pdxgiBufferFormats[nIndex];
		d3dShaderResourceViewDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		d3dShaderResourceViewDesc.Buffer.FirstElement = 0;
		d3dShaderResourceViewDesc.Buffer.NumElements = m_pnBufferElements[nIndex];
		d3dShaderResourceViewDesc.Buffer.StructureByteStride = m_pnBufferStrides[nIndex];
		d3dShaderResourceViewDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;
		break;
	}
	return(d3dShaderResourceViewDesc);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CMaterial::CMaterial()
{
}

CMaterial::~CMaterial()
{
	if (m_pTexture) m_pTexture->Release();
	if (m_pShader) m_pShader->Release();
}

void CMaterial::SetReflection(MATERIAL *m_pReflection)
{
	m_pReflection = m_pReflection;
}

void CMaterial::SetTexture(CTexture *pTexture)
{
	if (m_pTexture) m_pTexture->Release();
	m_pTexture = pTexture;
	if (m_pTexture) m_pTexture->AddRef();
}

void CMaterial::SetShader(CShader *pShader)
{
	if (m_pShader) m_pShader->Release();
	m_pShader = pShader;
	if (m_pShader) m_pShader->AddRef();
}

void CMaterial::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
	if (m_pTexture) m_pTexture->UpdateShaderVariables(pd3dCommandList);
}

void CMaterial::ReleaseShaderVariables()
{
	if (m_pShader) m_pShader->ReleaseShaderVariables();
	if (m_pTexture) m_pTexture->ReleaseShaderVariables();
}

void CMaterial::ReleaseUploadBuffers()
{
	if (m_pTexture) m_pTexture->ReleaseUploadBuffers();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
CGameObject::CGameObject(int nMeshes)
{
	m_xmf4x4World = Matrix4x4::Identity();

	m_nMeshes = nMeshes;
	m_ppMeshes = NULL;
	if (m_nMeshes > 0)
	{
		m_ppMeshes = new CMesh*[m_nMeshes];
		for (int i = 0; i < m_nMeshes; i++)	m_ppMeshes[i] = NULL;
	}
}

CGameObject::~CGameObject()
{
	ReleaseShaderVariables();

	if (m_ppMeshes)
	{
		for (int i = 0; i < m_nMeshes; i++)
		{
			if (m_ppMeshes[i]) m_ppMeshes[i]->Release();
			m_ppMeshes[i] = NULL;
		}
		delete[] m_ppMeshes;
	}
	if (m_pMaterial) m_pMaterial->Release();
}

void CGameObject::SetMesh(int nIndex, CMesh *pMesh)
{
	if (m_ppMeshes)
	{
		if (m_ppMeshes[nIndex]) m_ppMeshes[nIndex]->Release();
		m_ppMeshes[nIndex] = pMesh;
		if (pMesh) pMesh->AddRef();
	}
}

void CGameObject::SetShader(CShader *pShader)
{
	if (!m_pMaterial)
	{
		CMaterial *pMaterial = new CMaterial();
		SetMaterial(pMaterial);
	}
	if (m_pMaterial) m_pMaterial->SetShader(pShader);
}

void CGameObject::SetMaterial(CMaterial *pMaterial)
{
	if (m_pMaterial) m_pMaterial->Release();
	m_pMaterial = pMaterial;
	if (m_pMaterial) m_pMaterial->AddRef();
}

void CGameObject::SetTexture(CTexture* pTexture)
{
	CMaterial* pMaterial = new CMaterial();
	pMaterial->SetTexture(pTexture);
	SetMaterial(pMaterial);
}

void CGameObject::CreateShaderVariables(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList)
{
	UINT ncbElementBytes = ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255); //256ÀÇ ¹è¼ö
	m_pd3dcbGameObject = ::CreateBufferResource(pd3dDevice, pd3dCommandList, NULL, ncbElementBytes, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_GENERIC_READ, NULL);

	m_pd3dcbGameObject->Map(0, NULL, (void **)&m_pcbMappedGameObject);
}

void CGameObject::ReleaseShaderVariables()
{
	if (m_pd3dcbGameObject)
	{
		m_pd3dcbGameObject->Unmap(0, NULL);
		m_pd3dcbGameObject->Release();
	}
	if (m_pMaterial) m_pMaterial->ReleaseShaderVariables();
}

void CGameObject::UpdateShaderVariables(ID3D12GraphicsCommandList *pd3dCommandList)
{
	if (m_pcbMappedGameObject) XMStoreFloat4x4(&m_pcbMappedGameObject->m_xmf4x4World, XMMatrixTranspose(XMLoadFloat4x4(&m_xmf4x4World)));
	pd3dCommandList->SetGraphicsRootDescriptorTable(2, m_d3dCbvGPUDescriptorHandle);
}

void CGameObject::Animate(CCamera* pCamera, float fDeltaTime)
{
}

void CGameObject::Render(ID3D12GraphicsCommandList *pd3dCommandList, CCamera *pCamera)
{
	OnPrepareRender();

	if (m_pMaterial)
	{
		if (m_pMaterial->m_pShader)
		{
			m_pMaterial->m_pShader->Render(pd3dCommandList, pCamera, 0);
			m_pMaterial->m_pShader->UpdateShaderVariables(pd3dCommandList);
		}
		if (m_pMaterial->m_pTexture)
		{
			m_pMaterial->m_pTexture->UpdateShaderVariables(pd3dCommandList);
		}
	}

	UpdateShaderVariables(pd3dCommandList);

	if (m_ppMeshes)
	{
		for (int i = 0; i < m_nMeshes; i++)
		{
			if (m_ppMeshes[i]) m_ppMeshes[i]->Render(pd3dCommandList);
		}
	}
}

void CGameObject::ReleaseUploadBuffers()
{
	if (m_ppMeshes)
	{
		for (int i = 0; i < m_nMeshes; i++)
		{
			if (m_ppMeshes[i]) m_ppMeshes[i]->ReleaseUploadBuffers();
		}
	}

	if (m_pMaterial) m_pMaterial->ReleaseUploadBuffers();
}

void CGameObject::SetPosition(float x, float y, float z)
{
	m_xmf4x4World._41 = x;
	m_xmf4x4World._42 = y;
	m_xmf4x4World._43 = z;
}

void CGameObject::SetPosition(XMFLOAT3 xmf3Position)
{
	SetPosition(xmf3Position.x, xmf3Position.y, xmf3Position.z);
}

XMFLOAT3 CGameObject::GetPosition()
{
	return(XMFLOAT3(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43));
}

XMFLOAT3 CGameObject::GetLook()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._31, m_xmf4x4World._32, m_xmf4x4World._33)));
}

XMFLOAT3 CGameObject::GetUp()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._21, m_xmf4x4World._22, m_xmf4x4World._23)));
}

XMFLOAT3 CGameObject::GetRight()
{
	return(Vector3::Normalize(XMFLOAT3(m_xmf4x4World._11, m_xmf4x4World._12, m_xmf4x4World._13)));
}

void CGameObject::MoveStrafe(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Right = GetRight();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Right, fDistance);
	CGameObject::SetPosition(xmf3Position);
}

void CGameObject::MoveUp(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Up = GetUp();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Up, fDistance);
	CGameObject::SetPosition(xmf3Position);
}

void CGameObject::MoveForward(float fDistance)
{
	XMFLOAT3 xmf3Position = GetPosition();
	XMFLOAT3 xmf3Look = GetLook();
	xmf3Position = Vector3::Add(xmf3Position, xmf3Look, fDistance);
	CGameObject::SetPosition(xmf3Position);
}

void CGameObject::Rotate(float fPitch, float fYaw, float fRoll)
{
	XMMATRIX mtxRotate = XMMatrixRotationRollPitchYaw(XMConvertToRadians(fPitch), XMConvertToRadians(fYaw), XMConvertToRadians(fRoll));
	m_xmf4x4World = Matrix4x4::Multiply(mtxRotate, m_xmf4x4World);
}

void CGameObject::Rotate(XMFLOAT3 *pxmf3Axis, float fAngle)
{
	XMMATRIX mtxRotate = XMMatrixRotationAxis(XMLoadFloat3(pxmf3Axis), XMConvertToRadians(fAngle));
	m_xmf4x4World = Matrix4x4::Multiply(mtxRotate, m_xmf4x4World);
}

void CGameObject::SetLookAt(XMFLOAT3& xmf3Target, XMFLOAT3& xmf3Up)
{
	XMFLOAT3 xmf3Position(m_xmf4x4World._41, m_xmf4x4World._42, m_xmf4x4World._43);
	XMFLOAT4X4 mtxLookAt = Matrix4x4::LookAtLH(xmf3Position, xmf3Target, xmf3Up);
	m_xmf4x4World._11 = mtxLookAt._11; m_xmf4x4World._12 = mtxLookAt._21; m_xmf4x4World._13 = mtxLookAt._31;
	m_xmf4x4World._21 = mtxLookAt._12; m_xmf4x4World._22 = mtxLookAt._22; m_xmf4x4World._23 = mtxLookAt._32;
	m_xmf4x4World._31 = mtxLookAt._13; m_xmf4x4World._32 = mtxLookAt._23; m_xmf4x4World._33 = mtxLookAt._33;
/*	
	XMFLOAT3 xmf3Look = Vector3::Normalize(Vector3::Subtract(xmf3Target, xmf3Position));
	XMFLOAT3 xmf3Right = Vector3::CrossProduct(xmf3Up, xmf3Look, true);
	xmf3Up = Vector3::CrossProduct(xmf3Look, xmf3Right, true);
	m_xmf4x4World._11 = xmf3Right.x; m_xmf4x4World._12 = xmf3Right.y; m_xmf4x4World._13 = xmf3Right.z;
	m_xmf4x4World._21 = xmf3Up.x; m_xmf4x4World._22 = xmf3Up.y; m_xmf4x4World._23 = xmf3Up.z;
	m_xmf4x4World._31 = xmf3Look.x; m_xmf4x4World._32 = xmf3Look.y; m_xmf4x4World._33 = xmf3Look.z;
	*/
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
CBillboardObject::CBillboardObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, float fWidth, float fHeight) : CGameObject(1)
{
	CTexturedRectMesh* pMesh = new CTexturedRectMesh(pd3dDevice, pd3dCommandList, fWidth, fHeight, 0.0f, 0.0f, 0.0f, 0.5f);
	SetMesh(0, pMesh);
}

CBillboardObject::~CBillboardObject()
{
}

void CBillboardObject::Animate(CCamera* pCamera, float fDeltaTime)
{
	XMFLOAT3 xmf3CameraPosition = pCamera->GetPosition();
	SetLookAt(xmf3CameraPosition, XMFLOAT3(0.0f, 1.0f, 0.0f));
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// 
CParticleObject::CParticleObject(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12RootSignature* pd3dGraphicsRootSignature, XMFLOAT3 xmf3Position, XMFLOAT3 xmf3Velocity, float fLifetime, XMFLOAT3 xmf3Acceleration, XMFLOAT3 xmf3Color, XMFLOAT2 xmf2Size, UINT nMaxParticles) : CGameObject()
{
	CParticleMesh* pMesh = new CParticleMesh(pd3dDevice, pd3dCommandList, xmf3Position, xmf3Velocity, fLifetime, xmf3Acceleration, xmf3Color, xmf2Size, nMaxParticles);
	SetMesh(0, pMesh);

	CTexture* pParticleTexture = new CTexture(1, RESOURCE_TEXTURE2D, 0, 1);
	pParticleTexture->LoadTextureFromFile(pd3dDevice, pd3dCommandList, L"Image/RoundSoftParticle.dds", RESOURCE_TEXTURE2D, 0);

	CMaterial* pMaterial = new CMaterial();
	pMaterial->SetTexture(pParticleTexture);

/*
	XMFLOAT4 *pxmf4RandomValues = new XMFLOAT4[1024];
	random_device rd;   // non-deterministic generator
	mt19937 gen(rd());  // to seed mersenne twister.
	uniform_real_distribution<> vdist(-1.0, +1.0);
	uniform_real_distribution<> cdist(0.0, +1.0);
	for (int i = 0; i < 1024; i++) pxmf4RandomValues[i] = XMFLOAT4(float(vdist(gen)), float(vdist(gen)), float(vdist(gen)), float(cdist(gen)));
*/
	srand((unsigned)time(NULL));

	XMFLOAT4 *pxmf4RandomValues = new XMFLOAT4[1024];
	for (int i = 0; i < 1024; i++) { pxmf4RandomValues[i].x = float((rand() % 10000) - 5000) / 5000.0f; pxmf4RandomValues[i].y = float((rand() % 10000) - 5000) / 5000.0f; pxmf4RandomValues[i].z = float((rand() % 10000) - 5000) / 5000.0f; pxmf4RandomValues[i].w = float((rand() % 10000) - 5000) / 5000.0f; }

//	m_pRandowmValueTexture = new CTexture(1, RESOURCE_TEXTURE1D, 0, 1);
	m_pRandowmValueTexture = new CTexture(1, RESOURCE_BUFFER, 0, 1);
	m_pRandowmValueTexture->CreateBuffer(pd3dDevice, pd3dCommandList, pxmf4RandomValues, 1024, sizeof(XMFLOAT4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_GENERIC_READ, 0);

	m_pRandowmValueOnSphereTexture = new CTexture(1, RESOURCE_TEXTURE1D, 0, 1);
	m_pRandowmValueOnSphereTexture->CreateBuffer(pd3dDevice, pd3dCommandList, pxmf4RandomValues, 256, sizeof(XMFLOAT4), DXGI_FORMAT_R32G32B32A32_FLOAT, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_GENERIC_READ, 0);
	
	CreateShaderVariables(pd3dDevice, pd3dCommandList);

	CParticleShader* pShader = new CParticleShader();
	pShader->CreateGraphicsPipelineState(pd3dDevice, pd3dGraphicsRootSignature, 0);
	pShader->CreateShaderVariables(pd3dDevice, pd3dCommandList);
	pShader->CreateCbvSrvDescriptorHeaps(pd3dDevice, 1, 3);
	pShader->CreateConstantBufferViews(pd3dDevice, 1, m_pd3dcbGameObject, ((sizeof(CB_GAMEOBJECT_INFO) + 255) & ~255));

	pShader->CreateShaderResourceViews(pd3dDevice, pParticleTexture, 0, 5);
	pShader->CreateShaderResourceViews(pd3dDevice, m_pRandowmValueTexture, 0, 6);
	pShader->CreateShaderResourceViews(pd3dDevice, m_pRandowmValueOnSphereTexture, 0, 7);

	SetCbvGPUDescriptorHandle(pShader->GetGPUCbvDescriptorStartHandle());

	pMaterial->SetShader(pShader);
	SetMaterial(pMaterial);
}

CParticleObject::~CParticleObject()
{
	if (m_pRandowmValueTexture) m_pRandowmValueTexture->Release();
	if (m_pRandowmValueOnSphereTexture) m_pRandowmValueOnSphereTexture->Release();
}

void CParticleObject::ReleaseUploadBuffers()
{
	if (m_pRandowmValueTexture) m_pRandowmValueTexture->ReleaseUploadBuffers();
	if (m_pRandowmValueOnSphereTexture) m_pRandowmValueOnSphereTexture->ReleaseUploadBuffers();

	CGameObject::ReleaseUploadBuffers();
}

void CParticleObject::Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera)
{
	OnPrepareRender();

	if (m_pMaterial)
	{
		if (m_pMaterial->m_pShader) m_pMaterial->m_pShader->OnPrepareRender(pd3dCommandList, 0);
		if (m_pMaterial->m_pTexture) m_pMaterial->m_pTexture->UpdateShaderVariables(pd3dCommandList);

		if (m_pRandowmValueTexture) m_pRandowmValueTexture->UpdateShaderVariables(pd3dCommandList);
		if (m_pRandowmValueOnSphereTexture) m_pRandowmValueOnSphereTexture->UpdateShaderVariables(pd3dCommandList);
	}

	UpdateShaderVariables(pd3dCommandList);

	for (int i = 0; i < m_nMeshes; i++) if (m_ppMeshes[i]) m_ppMeshes[i]->PreRender(pd3dCommandList, 0); //Stream Output
	for (int i = 0; i < m_nMeshes; i++) if (m_ppMeshes[i]) m_ppMeshes[i]->Render(pd3dCommandList, 0); //Stream Output
	for (int i = 0; i < m_nMeshes; i++) if (m_ppMeshes[i]) m_ppMeshes[i]->PostRender(pd3dCommandList, 0); //Stream Output

	if (m_pMaterial && m_pMaterial->m_pShader) m_pMaterial->m_pShader->OnPrepareRender(pd3dCommandList, 1);

	for (int i = 0; i < m_nMeshes; i++) if (m_ppMeshes[i]) m_ppMeshes[i]->PreRender(pd3dCommandList, 1); //Draw
	for (int i = 0; i < m_nMeshes; i++) if (m_ppMeshes[i]) m_ppMeshes[i]->Render(pd3dCommandList, 1); //Draw
}

void CParticleObject::OnPostRender()
{
	for (int i = 0; i < m_nMeshes; i++) if (m_ppMeshes[i]) m_ppMeshes[i]->OnPostRender(0); //Read Stream Output Buffer Filled Size
}