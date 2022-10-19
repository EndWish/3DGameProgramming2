#include "stdafx.h"
#include "Material.h"
#include "GameFramework.h"

shared_ptr<Material> Material::LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	
	// 새로운 메테리얼 정보가 들어갈 공간을 할당한다.
	shared_ptr<Material> pNewMaterial = make_shared<Material>();

	// materialNameSize(UINT) / materialName(string) : 이름 읽어오기
	ReadStringBinary(pNewMaterial->name, _file);

	// 마테리얼 내에 리소스 생성후 map
	shared_ptr<VS_MaterialMappedFormat> pMappedMaterial;

	UINT cbElementSize = (sizeof(VS_MaterialMappedFormat) + 255) & (~255);
	ComPtr<ID3D12Resource> temp;
	pNewMaterial->pMaterialBuffer = CreateBufferResource(_pDevice, _pCommandList, NULL, cbElementSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, temp);
	pNewMaterial->pMaterialBuffer->Map(0, NULL, (void**)&pMappedMaterial);

	// format에 맞는값을 파일에서 읽어 리소스에 복사
	_file.read((char*)&pMappedMaterial->ambient, sizeof(XMFLOAT4));
	_file.read((char*)&pMappedMaterial->diffuse, sizeof(XMFLOAT4));
	_file.read((char*)&pMappedMaterial->specular, sizeof(XMFLOAT4));
	_file.read((char*)&pMappedMaterial->emissive, sizeof(XMFLOAT4));
	pNewMaterial->texture.CreateCbvSrvDescriptorHeaps(_pDevice);
	pNewMaterial->texture.LoadTextureFromFile(_pDevice, _pCommandList, _file);
	pMappedMaterial->textureType = pNewMaterial->texture.GetTexturesMapType();

	return pNewMaterial;
}

shared_ptr<Material> Material::LoadFromDirect(const string& name, const VS_MaterialMappedFormat& _materialColors, const array<string, TEXTURETYPENUM>& _textureFileNames, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {

	// 새로운 메테리얼 정보가 들어갈 공간을 할당한다.
	shared_ptr<Material> pNewMaterial = make_shared<Material>();

	// 이름 설정
	pNewMaterial->name = name;
	// 마테리얼 내에 리소스 생성후 map
	shared_ptr<VS_MaterialMappedFormat> pMappedMaterial;

	UINT cbElementSize = (sizeof(VS_MaterialMappedFormat) + 255) & (~255);
	ComPtr<ID3D12Resource> temp;
	pNewMaterial->pMaterialBuffer = CreateBufferResource(_pDevice, _pCommandList, NULL, cbElementSize, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, temp);
	pNewMaterial->pMaterialBuffer->Map(0, NULL, (void**)&pMappedMaterial);

	// format에 맞는값을 파일에서 읽어 리소스에 복사
	pMappedMaterial->ambient = _materialColors.ambient;
	pMappedMaterial->diffuse = _materialColors.diffuse;
	pMappedMaterial->specular = _materialColors.specular;
	pMappedMaterial->emissive = _materialColors.emissive;
	
	pNewMaterial->texture.CreateCbvSrvDescriptorHeaps(_pDevice);
	for (int i = 0; i < TEXTURETYPENUM; ++i) {
		if(!_textureFileNames[i].empty())
			pNewMaterial->texture.LoadTextureFromDirect(_pDevice, _pCommandList, i, _textureFileNames[i]);
	}
	pMappedMaterial->textureType = pNewMaterial->texture.GetTexturesMapType();

	return pNewMaterial;
}

Material::Material() {
	
}

Material::~Material() {

}

void Material::UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	texture.UpdateShaderVariables(_pCommandList);
	D3D12_GPU_VIRTUAL_ADDRESS gpuVirtualAddress = pMaterialBuffer->GetGPUVirtualAddress();
	_pCommandList->SetGraphicsRootConstantBufferView(3, gpuVirtualAddress);
}
