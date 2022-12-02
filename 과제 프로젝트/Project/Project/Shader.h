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
	UINT nDefaultStreamInputParticle = 0;	// defaultStreamInputBuffer의 파티클의 개수
	UINT nUploadStreamInputParticle = 0;	// uploadStreamInputBuffer의 파티클의 개수
	UINT nDefaultStreamOutputParticle = 0;
	ComPtr<ID3D12Resource> uploadStreamInputBuffer, defaultStreamInputBuffer, defaultDrawBuffer;	// 각각 파티클 추가를 위한 upload_heap버퍼, 스트림출력의 입력을 담당할 버퍼, 스트림 출력의 출력이 될 버퍼 
	shared_ptr<VS_ParticleMappedFormat[nMaxParticle]> mappedUploadStreamInputBuffer;	// upload버퍼에 값을 쓰기위해 맵핑을할 포인터
	D3D12_VERTEX_BUFFER_VIEW uploadStreamInputBufferView, defaultStreamInputBufferView;	// 스트림 출력의 입력으로 쓸 리소스에 대한 뷰
	D3D12_STREAM_OUTPUT_BUFFER_VIEW defaultStreamOutputBufferView;	// 스트림의 출력이 될 리소스에 대한 뷰
	D3D12_VERTEX_BUFFER_VIEW defaultDrawBufferView;	// 출력된 리소스들을 그릴기 위해 사용할 뷰
	ComPtr<ID3D12Resource> defaultBufferFilledSize, uploadBufferFilledSize, readBackBufferFilledSize;	// 각각 SO에서 write한 크기가 입력될 버퍼, default버퍼에 0을 쓰기위한 버퍼, defualt에 써진 값을 읽어오기 위한 버퍼
	shared_ptr<UINT> mappedReadBackBufferFilledSize;

	// 텍스처 관련 변수
	shared_ptr<Texture> texture;
	ComPtr<ID3D12DescriptorHeap> textureDescriptorHeap;
	shared_ptr<TextureBundle> textures;

	void Init(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

	void ReadFilledSize(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

};

class Shader {
protected:
	static vector<shared_ptr<Shader>> shaders;

	// 그려야하는 반투명한 오브젝트에 대한 포인터들
	static vector<weak_ptr<GameObject>> wpAlphaObjects;

	// StreamOutput과 관련된 변수들
	static ParticleResource particleResource;

public:
	static void MakeShaders(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, const ComPtr<ID3D12RootSignature>& _pRootSignature);
	static shared_ptr<Shader> GetShader(SHADER_TYPE _shaderIndex);

	static void AddAlphaObjects(const vector<weak_ptr<GameObject>>& _addObjects);
	static void RenderAlphaObjects(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, const XMFLOAT3& cameraPos);

	//  StreamOutput과 관련된 함수들
	static void AddParticle(const VS_ParticleMappedFormat& _particle);
	static void RenderParticle(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList);

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

	virtual D3D12_RASTERIZER_DESC CreateRasterizerState();
	virtual D3D12_DEPTH_STENCIL_DESC CreateDepthStencilState();
	virtual D3D12_BLEND_DESC CreateBlendState();
	virtual D3D12_INPUT_LAYOUT_DESC CreateInputLayout() = 0;
	virtual D3D12_STREAM_OUTPUT_DESC CreateStreamOuputState();

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

	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) { cout << "사용되지 않는다.\n"; };	// Render 순수가상함수?
	virtual void Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, bool isUploadInput);	// Render 순수가상함수?
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

