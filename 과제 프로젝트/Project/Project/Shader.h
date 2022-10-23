#pragma once

class BasicShader;
class HitBoxShader;
class TerrainShader;
class BillBoardShader;

class Shader {
private:
	static shared_ptr<BasicShader> basicShader;
	static shared_ptr<HitBoxShader> hitBoxShader;
	static shared_ptr<TerrainShader> terrainShader;
	static shared_ptr<BillBoardShader> billBoardShader;
public:
	static void MakeBasicShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	static shared_ptr<BasicShader> GetBasicShader();
	static void MakeHitBoxShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	static shared_ptr<HitBoxShader> GetHitBoxShader();
	static void MakeTerrainShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	static shared_ptr<TerrainShader> GetTerrainShader();
	static void MakeBillBoardShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	static shared_ptr<BillBoardShader> GetBillBoardShader();
protected:
	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	ComPtr<ID3D12PipelineState> pPipelineState;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc;
	vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;

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

	//
	virtual void PrepareRender(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

};

class BasicShader : public Shader {

public:
	BasicShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~BasicShader();

	virtual D3D12_RASTERIZER_DESC CreateRasterizerState() final;
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;

};

class TerrainShader : public Shader {

public:
	TerrainShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~TerrainShader();

	virtual D3D12_RASTERIZER_DESC CreateRasterizerState() final;
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;

};

class BillBoardShader : public Shader {
protected:
	ComPtr<ID3DBlob> pGSBlob;
public:
	BillBoardShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~BillBoardShader();

	virtual D3D12_RASTERIZER_DESC CreateRasterizerState() final;
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
};

class HitBoxShader : public Shader  {

public:
	HitBoxShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~HitBoxShader();

	D3D12_RASTERIZER_DESC CreateRasterizerState() final;
	D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;
};

