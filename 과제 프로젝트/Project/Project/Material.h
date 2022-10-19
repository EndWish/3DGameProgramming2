#pragma once
#include "Texture.h"

struct VS_MaterialMappedFormat {
	XMFLOAT4 ambient;
	XMFLOAT4 diffuse;
	XMFLOAT4 specular;
	XMFLOAT4 emissive;
	UINT textureType;
};

class Material {
public:
	static shared_ptr<Material> LoadFromFile(ifstream& _file, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	static shared_ptr<Material> LoadFromDirect(const string& name, const VS_MaterialMappedFormat& _materialColors, const array<string, TEXTURETYPENUM>& _textureFileNames, const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
private:
	string name;
	ComPtr<ID3D12Resource> pMaterialBuffer;
	CTexture texture;
	
public:
	Material();
	~Material();

	void UpdateShaderVariable(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};