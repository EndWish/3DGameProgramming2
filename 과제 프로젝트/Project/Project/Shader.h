#pragma once

class Texture;
class TextureBundle;
class Mesh;
class GameObject;

class BasicShader;
class AlphaBlendingShader;
class HitBoxShader;
class TerrainShader;
class BillBoardShader;

#define PARTICLE_TYPE_WRECK		0
#define PARTICLE_TYPE_SPARK		1

struct VS_ParticleMappedFormat {
	XMFLOAT3 position;
	XMFLOAT3 velocity;
	XMFLOAT2 boardSize;
	float lifetime;
	UINT type;
};

struct ParticleResource {
	static const UINT nMaxParticle = 10000;
	UINT nDefaultStreamInputParticle = 0;	// defaultStreamInputBuffer�� ��ƼŬ�� ����
	UINT nUploadStreamInputParticle = 0;	// uploadStreamInputBuffer�� ��ƼŬ�� ����
	UINT nDefaultStreamOutputParticle = 0;
	ComPtr<ID3D12Resource> uploadStreamInputBuffer, defaultStreamInputBuffer, defaultDrawBuffer;	// ���� ��ƼŬ �߰��� ���� upload_heap����, ��Ʈ������� �Է��� ����� ����, ��Ʈ�� ����� ����� �� ���� 
	shared_ptr<VS_ParticleMappedFormat[nMaxParticle]> mappedUploadStreamInputBuffer;	// upload���ۿ� ���� �������� �������� ������
	D3D12_VERTEX_BUFFER_VIEW uploadStreamInputBufferView, defaultStreamInputBufferView;	// ��Ʈ�� ����� �Է����� �� ���ҽ��� ���� ��
	D3D12_STREAM_OUTPUT_BUFFER_VIEW defaultStreamOutputBufferView;	// ��Ʈ���� ����� �� ���ҽ��� ���� ��
	D3D12_VERTEX_BUFFER_VIEW defaultDrawBufferView;	// ��µ� ���ҽ����� �׸��� ���� ����� ��
	ComPtr<ID3D12Resource> defaultBufferFilledSize, uploadBufferFilledSize, readBackBufferFilledSize;	// ���� SO���� write�� ũ�Ⱑ �Էµ� ����, default���ۿ� 0�� �������� ����, defualt�� ���� ���� �о���� ���� ����
	shared_ptr<UINT> mappedReadBackBufferFilledSize;

	// �ؽ�ó ���� ����
	shared_ptr<Texture> texture;
	ComPtr<ID3D12DescriptorHeap> textureDescriptorHeap;
	shared_ptr<TextureBundle> textures;

	void Init(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

	void ReadFilledSize(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

};

class Shader {
protected:
	static vector<shared_ptr<Shader>> shaders;

	// �׷����ϴ� �������� ������Ʈ�� ���� �����͵�
	static vector<weak_ptr<GameObject>> wpAlphaObjects;

	// StreamOutput�� ���õ� ������
	static ParticleResource particleResource;

public:
	static void MakeShaders(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	static shared_ptr<Shader> GetShader(SHADER_TYPE _shaderIndex);

	static void AddAlphaObjects(const vector<weak_ptr<GameObject>>& _addObjects);
	static void RenderAlphaObjects(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, const XMFLOAT3& cameraPos);

	//  StreamOutput�� ���õ� �Լ���
	static void AddParticle(const VS_ParticleMappedFormat& _particle);
	static void RenderParticle(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

protected:
	ComPtr<ID3DBlob> pVSBlob, pPSBlob;
	ComPtr<ID3D12PipelineState> pPipelineState;
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc;
	vector<D3D12_INPUT_ELEMENT_DESC> inputElementDescs;
	vector<weak_ptr<Mesh>> wpMeshes;

public:
	// ���� ���� �Լ���
	Shader();
	virtual ~Shader();
	void Init(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);

	D3D12_SHADER_BYTECODE CompileShaderFromFile(const wstring& _fileName, const string& _shaderName, const string& _shaderProfile, ComPtr<ID3DBlob>& _pBlob);

	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() = 0;
	virtual D3D12_STREAM_OUTPUT_DESC CreateStreamOuputState();

	// ���������� Set
	virtual void PrepareRender(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
	// �Ž� �߰�
	void AddMesh(const shared_ptr<Mesh>& _pMesh);
	// �׸���
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) = 0;

};

class BasicShader : public Shader {

public:
	BasicShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~BasicShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

class AlphaBlendingShader : public Shader {

public:
	AlphaBlendingShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~AlphaBlendingShader();

	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;
	virtual D3D12_BLEND_DESC CreateBlendState() final;
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState() final;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

class TerrainShader : public Shader {

public:
	TerrainShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~TerrainShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

class BillBoardShader : public Shader {
protected:
	ComPtr<ID3DBlob> pGSBlob;
public:
	BillBoardShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~BillBoardShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);
};

class ParticleStreamOutShader : public Shader {
protected:
	ComPtr<ID3DBlob> pGSBlob;
public:
	ParticleStreamOutShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~ParticleStreamOutShader();

	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() final;
	virtual D3D12_STREAM_OUTPUT_DESC CreateStreamOuputState() final;
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) { cout << "������ �ʴ´�.\n"; };	// Render ���������Լ�?
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, bool isUploadInput);	// Render ���������Լ�?
};
class ParticleDrawShader : public Shader {
protected:
	ComPtr<ID3DBlob> pGSBlob;
public:
	ParticleDrawShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	virtual ~ParticleDrawShader();

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

