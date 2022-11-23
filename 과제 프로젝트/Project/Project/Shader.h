#pragma once

class Mesh;
class GameObject;

class BasicShader;
class AlphaBlendingShader;
class HitBoxShader;
class TerrainShader;
class BillBoardShader;

class Shader {
private:
	static vector<shared_ptr<Shader>> shaders;

	static vector<weak_ptr<GameObject>> wpAlphaObjects;
public:
	static void MakeShaders(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	static shared_ptr<Shader> GetShader(SHADER_TYPE _shaderIndex);

	static void AddAlphaObjects(const vector<weak_ptr<GameObject>>& _addObjects);
	static void RenderAlphaObjects(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, const XMFLOAT3& cameraPos);
protected:
	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	ComPtr<ID3D12PipelineState> pPipelineState;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc;
	vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
	vector<weak_ptr<Mesh>> wpMeshes;

public:
	// 생성 관련 함수들
	Shader();
	virtual ~Shader();
	void Init(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);

	D3D12_SHADER_BYTECODE CompileShaderFromFile(const wstring& _fileName, const string& _shaderName, const string& _shaderProfile, ComPtr<ID3DBlob>& _pBlob);

	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_RASTERIZER_DESC CreateRasterizerState() = 0;
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() = 0;

	// 파이프라인 Set
	virtual void PrepareRender(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	// 매쉬 추가
	void AddMesh(const shared_ptr<Mesh>& _pMesh);
	// 그리기
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) = 0;

};

class BasicShader : public Shader {

public:
	BasicShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~BasicShader();

	virtual D3D12_RASTERIZER_DESC CreateRasterizerState() final;
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

class AlphaBlendingShader : public Shader {

public:
	AlphaBlendingShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~AlphaBlendingShader();

	virtual D3D12_RASTERIZER_DESC CreateRasterizerState() final;
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;
	virtual D3D12_BLEND_DESC CreateBlendState() final;
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState() final;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

class TerrainShader : public Shader {

public:
	TerrainShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~TerrainShader();

	virtual D3D12_RASTERIZER_DESC CreateRasterizerState() final;
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

class BillBoardShader : public Shader {
protected:
	ComPtr<ID3DBlob> pGSBlob;
public:
	BillBoardShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~BillBoardShader();

	virtual D3D12_RASTERIZER_DESC CreateRasterizerState() final;
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

class HitBoxShader : public Shader  {

public:
	HitBoxShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~HitBoxShader();

	D3D12_RASTERIZER_DESC CreateRasterizerState() final;
	D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

