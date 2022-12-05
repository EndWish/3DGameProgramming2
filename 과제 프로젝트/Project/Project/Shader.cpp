#include "stdafx.h"
#include "Shader.h"
#include "Texture.h"
#include "Mesh.h"
#include "GameObject.h"
#include "GameFramework.h"

// ���� ���� �� �Լ�
vector<shared_ptr<Shader>> Shader::shaders;

vector<weak_ptr<GameObject>> Shader::wpAlphaObjects;

// StreamOutput�� ���õ� ������
ParticleResource Shader::particleResource = ParticleResource();

void ParticleResource::Init(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	// UINT ncbElementBytes = ((sizeof(CB_FRAMEWORK_INFO) + 255) & ~255); //256�� ���
	// 256�� ����� �ȸ���?
	
	// ��Ʈ�� �ƿ�ǲ ���ҽ� ����
	UINT nStride = (UINT)(sizeof(VS_ParticleMappedFormat));
	nDefaultStreamInputParticle = 0;
	nUploadStreamInputParticle = 0;

	// ��ƼŬ�� ���� ������ �� ���ҽ����� ����
	uploadStreamInputBuffer = CreateBufferResource(_pDevice, _pCommandList, NULL, nStride * nMaxParticle, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
	defaultStreamInputBuffer = CreateBufferResource(_pDevice, _pCommandList, NULL, nStride * nMaxParticle, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);
	defaultDrawBuffer = CreateBufferResource(_pDevice, _pCommandList, NULL, nStride * nMaxParticle, D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, NULL);

	// ��ƼŬ�� ������ ����/����/�б����� ���ҽ� ����
	defaultBufferFilledSize = ::CreateBufferResource(_pDevice, _pCommandList, NULL, sizeof(UINT), D3D12_HEAP_TYPE_DEFAULT, D3D12_RESOURCE_STATE_STREAM_OUT, NULL);
	uploadBufferFilledSize = ::CreateBufferResource(_pDevice, _pCommandList, &nDefaultStreamInputParticle, sizeof(UINT), D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ | D3D12_RESOURCE_STATE_COPY_SOURCE, NULL);
	readBackBufferFilledSize = ::CreateBufferResource(_pDevice, _pCommandList, NULL, sizeof(UINT), D3D12_HEAP_TYPE_READBACK, D3D12_RESOURCE_STATE_COPY_DEST, NULL);

	// ���ε� ��ǲ ���� �� ���� �� ����
	uploadStreamInputBuffer->Map(0, NULL, (void**)&mappedUploadStreamInputBuffer);	// ���ε� ���� ����
	uploadStreamInputBuffer->Unmap(0, NULL);	// ���ε� ���� ����
	uploadStreamInputBufferView.BufferLocation = uploadStreamInputBuffer->GetGPUVirtualAddress();
	uploadStreamInputBufferView.SizeInBytes = nStride * nMaxParticle;
	uploadStreamInputBufferView.StrideInBytes = nStride;

	// ����Ʈ ��ǲ ���� �� ����
	defaultStreamInputBufferView.BufferLocation = defaultStreamInputBuffer->GetGPUVirtualAddress();
	defaultStreamInputBufferView.SizeInBytes = nStride * nMaxParticle;
	defaultStreamInputBufferView.StrideInBytes = nStride;

	// ����Ʈ �ƿ�ǲ ���� �� ����
	defaultStreamOutputBufferView.BufferLocation = defaultDrawBuffer->GetGPUVirtualAddress();
	defaultStreamOutputBufferView.SizeInBytes = nStride * nMaxParticle;
	defaultStreamOutputBufferView.BufferFilledSizeLocation = defaultBufferFilledSize->GetGPUVirtualAddress();

	// �������ϱ����� ���� �� ����
	defaultDrawBufferView.BufferLocation = defaultDrawBuffer->GetGPUVirtualAddress();
	defaultDrawBufferView.SizeInBytes = nStride * nMaxParticle;
	defaultDrawBufferView.StrideInBytes = nStride;

	// ��ƼŬ�� ������ �б����� ���ҽ��� ����
	readBackBufferFilledSize->Map(0, NULL, (void**)&mappedReadBackBufferFilledSize);
	readBackBufferFilledSize->Unmap(0, NULL);

	// �ʱ� ���� ����
	SynchronizeResourceTransition(_pCommandList, defaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST);
	_pCommandList->CopyResource(defaultBufferFilledSize.Get(), uploadBufferFilledSize.Get());
	SynchronizeResourceTransition(_pCommandList, defaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT);

	
	// ������ ��  ����
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc;
	descriptorHeapDesc.NumDescriptors = 1; // SRVs 
	descriptorHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descriptorHeapDesc.NodeMask = 0;
	_pDevice->CreateDescriptorHeap(&descriptorHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)textureDescriptorHeap.GetAddressOf());

	// ���̴��� �ؽ��� ��������
	texture = make_shared<Texture>();
	texture = Texture::Load(L"Particles.dds", RESOURCE_TEXTURE2D, PARAMETER_STANDARD_TEXTURE + MATERIAL_ALBEDO_MAP_INDEX, MATERIAL_ALBEDO_MAP, _pDevice, _pCommandList);

	D3D12_SHADER_RESOURCE_VIEW_DESC shaderResourceViewDesc = texture->GetShaderResourceViewDesc();
	_pDevice->CreateShaderResourceView(texture->GetTextureBuffer().Get(), &shaderResourceViewDesc, textureDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	texture->SetGpuDescriptorHandle(textureDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
	
}
void ParticleResource::ReadFilledSize(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	size_t nStride = sizeof(VS_ParticleMappedFormat);
	
	//4-2. ��Ʈ�� ����� ����� ���� ��ƼŬ�� ������ �����Ѵ�.
	//SynchronizeResourceTransition(_pCommandList, defaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);
	//_pCommandList->CopyResource(readBackBufferFilledSize.Get(), defaultBufferFilledSize.Get());
	//SynchronizeResourceTransition(_pCommandList, defaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT);

	//readBackBufferFilledSize->Map(0, NULL, (void**)&mappedReadBackBufferFilledSize);
	nDefaultStreamOutputParticle += (*mappedReadBackBufferFilledSize) / nStride;
	cout << "��ƼŬ�� ���� ���� : " << nDefaultStreamOutputParticle << "\n";
	//readBackBufferFilledSize->Unmap(0, NULL);
}

void Shader::MakeShaders(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, const ComPtr<ID3D12RootSignature>& _pRootSignature) {
	shaders.assign((int)SHADER_TYPE::NUM, {});
	shaders[0] = make_shared<BasicShader>(_pDevice, _pRootSignature);
	shaders[1] = make_shared<AlphaBlendingShader>(_pDevice, _pRootSignature);
	shaders[2] = make_shared<HitBoxShader>(_pDevice, _pRootSignature);
	shaders[3] = make_shared<TerrainShader>(_pDevice, _pRootSignature);
	shaders[4] = make_shared<BillBoardShader>(_pDevice, _pRootSignature);
	shaders[5] = make_shared<ParticleStreamOutShader>(_pDevice, _pRootSignature);
	shaders[6] = make_shared<ParticleDrawShader>(_pDevice, _pRootSignature);
	shaders[7] = make_shared<MultipleRenderTargetShader>(_pDevice, _pRootSignature);

	particleResource.Init(_pDevice, _pCommandList);

}
shared_ptr<Shader> Shader::GetShader(SHADER_TYPE _shaderIndex) {
	return shaders[(int)_shaderIndex];
}

void  Shader::AddAlphaObjects(const vector<weak_ptr<GameObject>>& _addObjects) {
	wpAlphaObjects.insert(wpAlphaObjects.begin(), _addObjects.begin(), _addObjects.end());
}
void Shader::RenderAlphaObjects(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, const XMFLOAT3& cameraPos) {
	auto func = [cameraPos](const shared_ptr<GameObject>& a, const shared_ptr<GameObject>& b) {
		return Vector3::Distance2(cameraPos, a->GetWorldPosition()) > Vector3::Distance2(cameraPos, b->GetWorldPosition());
	};
	ranges::sort(wpAlphaObjects, func, &weak_ptr<GameObject>::lock);

	for (auto& wpAlphaObject : wpAlphaObjects) {
		shared_ptr<GameObject> pAlphaObject = wpAlphaObject.lock();
		if (pAlphaObject) {
			// ����� ���̴��� �ε����� ��´�.
			SHADER_TYPE shaderType = pAlphaObject->GetMesh()->GetShaderType();
			Shader::GetShader(shaderType)->PrepareRender(_pCommandList);
			pAlphaObject->RenderOnce(_pCommandList);
		}
	}
	wpAlphaObjects.clear();
}

//  StreamOutput�� ���õ� �Լ���
void Shader::AddParticle(const VS_ParticleMappedFormat& _particle) {
	UINT& nParticle = particleResource.nUploadStreamInputParticle;
	if (nParticle < particleResource.nMaxParticle) {
		particleResource.uploadStreamInputBuffer->Map(0, NULL, (void**)&particleResource.mappedUploadStreamInputBuffer);	// ���ε� ���� ����
		memcpy(&particleResource.mappedUploadStreamInputBuffer[nParticle], &_particle, sizeof(VS_ParticleMappedFormat));
		particleResource.uploadStreamInputBuffer->Unmap(0, NULL);	// ���ε� ���� ����
		++nParticle;
		cout << nParticle << "�� ���ο� ��ƼŬ �߰�\n";
	}
}
void Shader::RenderParticle(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {


	size_t nStride = sizeof(VS_ParticleMappedFormat);

	particleResource.readBackBufferFilledSize->Map(0, NULL, (void**)&particleResource.mappedReadBackBufferFilledSize);
	particleResource.nDefaultStreamOutputParticle += (*particleResource.mappedReadBackBufferFilledSize) / nStride;
	//cout << "��ü ����ؾߵ� ���� : " << particleResource.nDefaultStreamOutputParticle << "\n";
	particleResource.readBackBufferFilledSize->Unmap(0, NULL);

	//5. defaultDrawBuffer�� D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER�� �ٲ۴�.  defaultDrawBuffer�� �Է����� �������� �����Ѵ�.
	SynchronizeResourceTransition(_pCommandList, particleResource.defaultDrawBuffer.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
	if (0 < particleResource.nDefaultStreamOutputParticle) {
		Shader::GetShader(SHADER_TYPE::ParticleDraw)->Render(_pCommandList);
	}

	if (1) {
		//6. �ΰ��� �����͸� �ٲ۴�. ��ƼŬ�� ������ �ٲ��ش�.


		swap(particleResource.defaultDrawBuffer, particleResource.defaultStreamInputBuffer);
		//cout << particleResource.nDefaultStreamInputParticle << " = " << particleResource.nDefaultStreamOutputParticle << "\n";
		particleResource.nDefaultStreamInputParticle = particleResource.nDefaultStreamOutputParticle;	// �̹��� ��°����� ������ �Է°����� �ȴ�.
		particleResource.defaultStreamInputBufferView.BufferLocation = particleResource.defaultStreamInputBuffer->GetGPUVirtualAddress();
		particleResource.defaultDrawBufferView.BufferLocation = particleResource.defaultDrawBuffer->GetGPUVirtualAddress();
		particleResource.defaultStreamOutputBufferView.BufferLocation = particleResource.defaultDrawBuffer->GetGPUVirtualAddress();
		particleResource.defaultStreamOutputBufferView.SizeInBytes = nStride * particleResource.nMaxParticle;
	}

	//1. uploadStreamInputBuffer�� ��ƼŬ�� �߰��Ѵ�.nUploadStreamInputParticle�� �߰��� ������ŭ ������Ų��.
	
	//2. defaultBufferFilledSize�� uploadBufferFilledSize�� �����Ͽ� 0���� �����. �׷��� �ϴ� ��ƼŬ ������ 0���� �ʱ�ȭ
	SynchronizeResourceTransition(_pCommandList, particleResource.defaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST);
	_pCommandList->CopyResource(particleResource.defaultBufferFilledSize.Get(), particleResource.uploadBufferFilledSize.Get());
	SynchronizeResourceTransition(_pCommandList, particleResource.defaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT);
	particleResource.nDefaultStreamOutputParticle = 0;


	//3. defaultDrawBuffer�� ���¸� D3D12_RESOURCE_STATE_STREAM_OUT�� �ٲ۴�. uploadStreamInputBuffer�� �Է����� defaultDrawBuffer�� ������� SO�ܰ踦 �����Ѵ�. 4�ܰ��� ����� ��ġ�� �ٲپ��� �� nUploadStreamInputParticle�� 0���� �ʱ�ȭ.
	bool uploadProcess = false;
	SynchronizeResourceTransition(_pCommandList, particleResource.defaultDrawBuffer.Get(), D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, D3D12_RESOURCE_STATE_STREAM_OUT);
	if (0 < particleResource.nUploadStreamInputParticle) {
		static_pointer_cast<ParticleStreamOutShader>(Shader::GetShader(SHADER_TYPE::ParticleStreamOut))->Render(_pCommandList, true);

		//3-2. ��Ʈ�� ����� ����� ���� ��ƼŬ�� ������ �����Ѵ�.
		SynchronizeResourceTransition(_pCommandList, particleResource.defaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);
		_pCommandList->CopyResource(particleResource.readBackBufferFilledSize.Get(), particleResource.defaultBufferFilledSize.Get());
		SynchronizeResourceTransition(_pCommandList, particleResource.defaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT);
		
		//particleResource.readBackBufferFilledSize->Map(0, NULL, (void**)&particleResource.mappedReadBackBufferFilledSize);
		particleResource.nDefaultStreamOutputParticle += particleResource.nUploadStreamInputParticle;
		//cout << "���� �߰��� ��ƼŬ SO ������ ���� : " << (*particleResource.mappedReadBackBufferFilledSize) / nStride << "\n";
		//particleResource.readBackBufferFilledSize->Unmap(0, NULL);

		//SynchronizeResourceTransition(_pCommandList, particleResource.defaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_DEST);
		//_pCommandList->CopyResource(particleResource.defaultBufferFilledSize.Get(), particleResource.uploadBufferFilledSize.Get());
		//SynchronizeResourceTransition(_pCommandList, particleResource.defaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_STREAM_OUT);

		particleResource.defaultStreamOutputBufferView.BufferLocation = particleResource.defaultDrawBuffer->GetGPUVirtualAddress() + nStride * particleResource.nUploadStreamInputParticle;
		particleResource.defaultStreamOutputBufferView.SizeInBytes = nStride * (particleResource.nMaxParticle - particleResource.nUploadStreamInputParticle);
		particleResource.nUploadStreamInputParticle = 0;

		uploadProcess = true;
	}

	//4. �� ���� ��ġ���� defaultStreamInputBuffer�� �̷����� defaultDrawBuffer�� ������� SO�ܰ踦 �����Ѵ�.
	bool defaultProcess = false;
	if (0 < particleResource.nDefaultStreamInputParticle) {
		static_pointer_cast<ParticleStreamOutShader>(Shader::GetShader(SHADER_TYPE::ParticleStreamOut))->Render(_pCommandList, false);

		//4-2. ��Ʈ�� ����� ����� ���� ��ƼŬ�� ������ �����Ѵ�.
		SynchronizeResourceTransition(_pCommandList, particleResource.defaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_STREAM_OUT, D3D12_RESOURCE_STATE_COPY_SOURCE);
		_pCommandList->CopyResource(particleResource.readBackBufferFilledSize.Get(), particleResource.defaultBufferFilledSize.Get());
		SynchronizeResourceTransition(_pCommandList, particleResource.defaultBufferFilledSize.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_STREAM_OUT);


		defaultProcess = true;
	}

	_pCommandList->SOSetTargets(0, 1, NULL);	// SO����� ���� �ʵ��� �����Ѵ�. ȭ�鿡 �׷��� �ϹǷ�



}

//////////////////// Shader

Shader::Shader() {

}
Shader::~Shader() {

}

void Shader::Init(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature) {
	
	ZeroMemory(&pipelineStateDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	pipelineStateDesc.pRootSignature = _pRootSignature.Get();
	pipelineStateDesc.RasterizerState = CreateRasterizerState();
	pipelineStateDesc.BlendState = CreateBlendState();
	pipelineStateDesc.InputLayout = CreateInputLayout();
	pipelineStateDesc.DepthStencilState = CreateDepthStencilState();
	pipelineStateDesc.StreamOutput = CreateStreamOuputState();
	
	pipelineStateDesc.SampleMask = UINT_MAX;
	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateDesc.NumRenderTargets = 1;
	pipelineStateDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	pipelineStateDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	pipelineStateDesc.SampleDesc.Count = 1;
	pipelineStateDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
}

D3D12_RASTERIZER_DESC Shader::CreateRasterizerState() {
	D3D12_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return rasterizerDesc;
}
D3D12_SHADER_BYTECODE Shader::CompileShaderFromFile(const wstring& _fileName, const string& _shaderName, const string& _shaderProfile, ComPtr<ID3DBlob>& _pBlob) {
	UINT nCompileFlag = 0;

	ComPtr<ID3DBlob> pErrorBlob;
	HRESULT hResult = D3DCompileFromFile(_fileName.c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, _shaderName.c_str(), _shaderProfile.c_str(), nCompileFlag, 0, _pBlob.GetAddressOf(), pErrorBlob.GetAddressOf());

	char* pErrorString = NULL;
	if (pErrorBlob) { 
		pErrorString = (char*)pErrorBlob->GetBufferPointer();
		cout << "����Ʈ �ڵ� ���� ���� : " << pErrorString << "\n";
	}

	D3D12_SHADER_BYTECODE shaderByteCode;
	shaderByteCode.BytecodeLength = _pBlob->GetBufferSize();
	shaderByteCode.pShaderBytecode = _pBlob->GetBufferPointer();
	return shaderByteCode;
}
D3D12_DEPTH_STENCIL_DESC Shader::CreateDepthStencilState() {
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = 0x00;
	depthStencilDesc.StencilWriteMask = 0x00;
	depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return depthStencilDesc;
}
D3D12_BLEND_DESC Shader::CreateBlendState() {
	D3D12_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(D3D12_BLEND_DESC));
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0].BlendEnable = FALSE;
	blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blendDesc;
}
D3D12_STREAM_OUTPUT_DESC Shader::CreateStreamOuputState() {
	D3D12_STREAM_OUTPUT_DESC streamOutputDesc;
	::ZeroMemory(&streamOutputDesc, sizeof(D3D12_STREAM_OUTPUT_DESC));

	streamOutputDesc.NumEntries = 0;
	streamOutputDesc.NumStrides = 0;
	streamOutputDesc.pBufferStrides = NULL;
	streamOutputDesc.pSODeclaration = NULL;
	streamOutputDesc.RasterizedStream = 0;

	return(streamOutputDesc);
}

void Shader::PrepareRender(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	if (pPipelineState) {
		_pCommandList->SetPipelineState(pPipelineState.Get());
	}
}

void Shader::AddMesh(const shared_ptr<Mesh>& _pMesh) {
	//pMeshes.emplace_back(_pMesh);
	wpMeshes.push_back(_pMesh);
}

//////////////////// Basic Shader
BasicShader::BasicShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature) {

	Init(_pDevice, _pRootSignature);
	pipelineStateDesc.VS = CompileShaderFromFile(L"Shaders.hlsl", "DefaultVertexShader", "vs_5_1", pVSBlob);
	pipelineStateDesc.PS = CompileShaderFromFile(L"Shaders.hlsl", "DefaultPixelShader", "ps_5_1", pPSBlob);

	HRESULT hResult = _pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)pPipelineState.GetAddressOf());

	pVSBlob.Reset();
	pPSBlob.Reset();

	inputElementDescs.clear();
}

BasicShader::~BasicShader() {

}

D3D12_INPUT_LAYOUT_DESC BasicShader::CreateInputLayout() {
	inputElementDescs.assign(3, {});

	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = &inputElementDescs[0];
	inputLayoutDesc.NumElements = (UINT)inputElementDescs.size();

	return inputLayoutDesc;
}

void BasicShader::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	PrepareRender(_pCommandList);
	auto func = [_pCommandList](const shared_ptr<Mesh>& _pMesh) {
		if (_pMesh) {	// �ش� �Ž��� �����Ѵٸ�
			_pMesh->RenderWithDrawObjects(_pCommandList);
			return false;
		}
		else {	// �ش� �Ž��� �������� ���
			return true;
		}
	};
	wpMeshes.erase(ranges::remove_if(wpMeshes, func, &weak_ptr<Mesh>::lock).begin(), wpMeshes.end());
}

//////////////////// AlphaBlending Shader
AlphaBlendingShader::AlphaBlendingShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature) {

	Init(_pDevice, _pRootSignature);
	pipelineStateDesc.VS = CompileShaderFromFile(L"Shaders.hlsl", "AlphaBlendingVertexShader", "vs_5_1", pVSBlob);
	pipelineStateDesc.PS = CompileShaderFromFile(L"Shaders.hlsl", "AlphaBlendingPixelShader", "ps_5_1", pPSBlob);

	HRESULT hResult = _pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)pPipelineState.GetAddressOf());

	pVSBlob.Reset();
	pPSBlob.Reset();

	inputElementDescs.clear();
}
AlphaBlendingShader::~AlphaBlendingShader() {

}

D3D12_RASTERIZER_DESC AlphaBlendingShader::CreateRasterizerState() {
	D3D12_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return rasterizerDesc;
}
D3D12_INPUT_LAYOUT_DESC AlphaBlendingShader::CreateInputLayout() {
	inputElementDescs.assign(3, {});

	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = &inputElementDescs[0];
	inputLayoutDesc.NumElements = (UINT)inputElementDescs.size();

	return inputLayoutDesc;
}
D3D12_BLEND_DESC AlphaBlendingShader::CreateBlendState() {
	D3D12_BLEND_DESC blendDesc;
	ZeroMemory(&blendDesc, sizeof(D3D12_BLEND_DESC));
	blendDesc.AlphaToCoverageEnable = FALSE;	//���� ���ø��� ���Ͽ� ���� Ÿ�� 0�� ���İ��� Ŀ������ �Ž�ũ�� ��ȯ
	blendDesc.IndependentBlendEnable = FALSE;	// �� ����Ÿ�ٿ��� �������� ������ ����. FALSE(Render Target[0]�� ���)
	blendDesc.RenderTarget[0].BlendEnable = TRUE;
	blendDesc.RenderTarget[0].LogicOpEnable = FALSE;
	blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	return blendDesc;
}
D3D12_DEPTH_STENCIL_DESC AlphaBlendingShader::CreateDepthStencilState() {
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	depthStencilDesc.DepthEnable = TRUE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = 0x00;
	depthStencilDesc.StencilWriteMask = 0x00;
	depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return depthStencilDesc;
}

void AlphaBlendingShader::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	// �ִ� �׸� ������Ʈ�� ������ ���� ���� �׷��� �Ѵ�.
	auto func = [_pCommandList](const shared_ptr<Mesh>& _pMesh) {
		if (_pMesh) {	// �ش� �Ž��� �����Ѵٸ�
			Shader::AddAlphaObjects(_pMesh->GetDrawObjects());
			return false;
		}
		else {	// �ش� �Ž��� �������� ���
			return true;
		}
	};
	wpMeshes.erase(ranges::remove_if(wpMeshes, func, &weak_ptr<Mesh>::lock).begin(), wpMeshes.end());
}

//////////////////// Terrain Shader
TerrainShader::TerrainShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature) {

	Init(_pDevice, _pRootSignature);
	pipelineStateDesc.VS = CompileShaderFromFile(L"Shaders.hlsl", "TerrainVertexShader", "vs_5_1", pVSBlob);
	pipelineStateDesc.PS = CompileShaderFromFile(L"Shaders.hlsl", "TerrainPixelShader", "ps_5_1", pPSBlob);
	
	_pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)pPipelineState.GetAddressOf());

	pVSBlob.Reset();
	pPSBlob.Reset();

	inputElementDescs.clear();
}
TerrainShader::~TerrainShader() {

}

D3D12_INPUT_LAYOUT_DESC TerrainShader::CreateInputLayout() {
	inputElementDescs.assign(4, {});

	// �ø�ƽ, �ø�ƽ �ε���, ����, ��ǲ����, ������, ��ǲ����Ŭ����, InstaneDataStepRate : https://vitacpp.tistory.com/m/70
	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[1] = { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[2] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[3] = { "TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = &inputElementDescs[0];
	inputLayoutDesc.NumElements = (UINT)inputElementDescs.size();

	return inputLayoutDesc;
}

void TerrainShader::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	PrepareRender(_pCommandList);
	auto func = [_pCommandList](const shared_ptr<Mesh>& _pMesh) {
		if (_pMesh) {	// �ش� �Ž��� �����Ѵٸ�
			_pMesh->RenderWithDrawObjects(_pCommandList); // [����] : �޽��� �ڽ��� �׸� ������Ʈ�� ���� �����ͷ� ��� ������Ʈ�� �׸���.
			return false;
		}
		else {	// �ش� �Ž��� �������� ���
			return true;
		}
	};
	wpMeshes.erase(ranges::remove_if(wpMeshes, func, &weak_ptr<Mesh>::lock).begin(), wpMeshes.end());
}

//////////////////// BillBoard Shader
BillBoardShader::BillBoardShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature) {
	Init(_pDevice, _pRootSignature);

	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	pipelineStateDesc.VS = CompileShaderFromFile(L"Shaders.hlsl", "BillBoardVertexShader", "vs_5_1", pVSBlob);
	pipelineStateDesc.GS = CompileShaderFromFile(L"Shaders.hlsl", "BillBoardGeometryShader", "gs_5_1", pGSBlob);
	pipelineStateDesc.PS = CompileShaderFromFile(L"Shaders.hlsl", "BillBoardPixelShader", "ps_5_1", pPSBlob);

	HRESULT hResult = _pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)pPipelineState.GetAddressOf());

	pVSBlob.Reset();
	pGSBlob.Reset();
	pPSBlob.Reset();

	inputElementDescs.clear();
}
BillBoardShader::~BillBoardShader() {

}

D3D12_INPUT_LAYOUT_DESC BillBoardShader::CreateInputLayout() {
	inputElementDescs.assign(2, {});

	// �ø�ƽ, �ø�ƽ �ε���, ����, ��ǲ����, ������, ��ǲ����Ŭ����, InstaneDataStepRate : https://vitacpp.tistory.com/m/70
	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[1] = { "BOARDSIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = &inputElementDescs[0];
	inputLayoutDesc.NumElements = (UINT)inputElementDescs.size();

	return inputLayoutDesc;
}

void BillBoardShader::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	PrepareRender(_pCommandList);
	auto func = [_pCommandList](const shared_ptr<Mesh>& _pMesh) {
		if (_pMesh) {	// �ش� �Ž��� �����Ѵٸ�
			_pMesh->RenderWithDrawObjects(_pCommandList);
			return false;
		}
		else {	// �ش� �Ž��� �������� ���
			return true;
		}
	};
	wpMeshes.erase(ranges::remove_if(wpMeshes, func, &weak_ptr<Mesh>::lock).begin(), wpMeshes.end());
}

/////////////////// Particle Shader
////////// Streamoutput
ParticleStreamOutShader::ParticleStreamOutShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature) {
	Init(_pDevice, _pRootSignature);

	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	pipelineStateDesc.VS = CompileShaderFromFile(L"Shaders.hlsl", "ParticleStreamOutVertexShader", "vs_5_1", pVSBlob);
	pipelineStateDesc.GS = CompileShaderFromFile(L"Shaders.hlsl", "ParticleStreamOutGeometryShader", "gs_5_1", pGSBlob);

	// PS�� ������ �ʿ䰡 ����.
	D3D12_SHADER_BYTECODE d3dShaderByteCode;
	d3dShaderByteCode.BytecodeLength = 0;
	d3dShaderByteCode.pShaderBytecode = NULL;
	pipelineStateDesc.PS = d3dShaderByteCode;

	HRESULT hResult = _pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)pPipelineState.GetAddressOf());
	if (hResult != S_OK)
		cout << "Streamoutput ���������� ���� ����\n";

	pVSBlob.Reset();
	pGSBlob.Reset();
	pPSBlob.Reset();

	inputElementDescs.clear();
}
ParticleStreamOutShader::~ParticleStreamOutShader() {

}

D3D12_INPUT_LAYOUT_DESC ParticleStreamOutShader::CreateInputLayout() {
	inputElementDescs.assign(5, {});

	// �ø�ƽ, �ø�ƽ �ε���, ����, ��ǲ����, ������, ��ǲ����Ŭ����, InstaneDataStepRate : https://vitacpp.tistory.com/m/70
	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };	// float3
	inputElementDescs[1] = { "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };	// float3
	inputElementDescs[2] = { "BOARDSIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };	// float2
	inputElementDescs[3] = { "LIFETIME", 0, DXGI_FORMAT_R32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };	// float
	inputElementDescs[4] = { "PARTICLETYPE", 0, DXGI_FORMAT_R32_UINT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };	// uint
	
	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = &inputElementDescs[0];
	inputLayoutDesc.NumElements = (UINT)inputElementDescs.size();

	return inputLayoutDesc;
}
D3D12_DEPTH_STENCIL_DESC ParticleStreamOutShader::CreateDepthStencilState() {
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = 0x00;
	depthStencilDesc.StencilWriteMask = 0x00;
	depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return depthStencilDesc;
}
D3D12_STREAM_OUTPUT_DESC ParticleStreamOutShader::CreateStreamOuputState() {
	D3D12_STREAM_OUTPUT_DESC streamOutputDesc;
	::ZeroMemory(&streamOutputDesc, sizeof(D3D12_STREAM_OUTPUT_DESC));

	UINT nStreamOutputDecls = 5;
	D3D12_SO_DECLARATION_ENTRY* pd3dStreamOutputDecls = new D3D12_SO_DECLARATION_ENTRY[nStreamOutputDecls];
	pd3dStreamOutputDecls[0] = { 0, "POSITION", 0, 0, 3, 0 };
	pd3dStreamOutputDecls[1] = { 0, "VELOCITY", 0, 0, 3, 0 };
	pd3dStreamOutputDecls[2] = { 0, "BOARDSIZE", 0, 0, 2, 0 };
	pd3dStreamOutputDecls[3] = { 0, "LIFETIME", 0, 0, 1, 0 };
	pd3dStreamOutputDecls[4] = { 0, "PARTICLETYPE", 0, 0, 1, 0 };

	UINT* pBufferStrides = new UINT[1];
	pBufferStrides[0] = sizeof(VS_ParticleMappedFormat);

	streamOutputDesc.NumEntries = nStreamOutputDecls;
	streamOutputDesc.pSODeclaration = pd3dStreamOutputDecls;
	streamOutputDesc.NumStrides = 1;
	streamOutputDesc.pBufferStrides = pBufferStrides;
	streamOutputDesc.RasterizedStream = D3D12_SO_NO_RASTERIZED_STREAM;

	return(streamOutputDesc);
}

void ParticleStreamOutShader::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList, bool isUploadInput) {

	PrepareRender(_pCommandList);
	_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	D3D12_STREAM_OUTPUT_BUFFER_VIEW streamOutputBufferViews[1] = { Shader::particleResource.defaultStreamOutputBufferView };
	_pCommandList->SOSetTargets(0, 1, streamOutputBufferViews);

	if (isUploadInput) {	// ���ε� �� ���ҽ��� �Է����� SO�ܰ踦 ������ ���
		D3D12_VERTEX_BUFFER_VIEW vertexBuffersViews[1] = { Shader::particleResource.uploadStreamInputBufferView };
		_pCommandList->IASetVertexBuffers(0, 1, vertexBuffersViews);

		_pCommandList->DrawInstanced((UINT)Shader::particleResource.nUploadStreamInputParticle, 1, 0, 0);
	}
	else {	// ����Ʈ �� ���ҽ��� �Է����� SO�ܰ踦 ������ ���
		D3D12_VERTEX_BUFFER_VIEW vertexBuffersViews[1] = { Shader::particleResource.defaultStreamInputBufferView };
		_pCommandList->IASetVertexBuffers(0, 1, vertexBuffersViews);

		_pCommandList->DrawInstanced((UINT)Shader::particleResource.nDefaultStreamInputParticle, 1, 0, 0);
	}
}

////////// Draw
ParticleDrawShader::ParticleDrawShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature) {
	Init(_pDevice, _pRootSignature);

	pipelineStateDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	pipelineStateDesc.VS = CompileShaderFromFile(L"Shaders.hlsl", "ParticleDrawVertexShader", "vs_5_1", pVSBlob);
	pipelineStateDesc.GS = CompileShaderFromFile(L"Shaders.hlsl", "ParticleDrawGeometryShader", "gs_5_1", pGSBlob);
	pipelineStateDesc.PS = CompileShaderFromFile(L"Shaders.hlsl", "ParticleDrawPixelShader", "ps_5_1", pPSBlob);

	HRESULT hResult = _pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)pPipelineState.GetAddressOf());

	pVSBlob.Reset();
	pGSBlob.Reset();
	pPSBlob.Reset();

	inputElementDescs.clear();
}
ParticleDrawShader::~ParticleDrawShader() {

}
D3D12_INPUT_LAYOUT_DESC ParticleDrawShader::CreateInputLayout() {
	inputElementDescs.assign(5, {});

	// �ø�ƽ, �ø�ƽ �ε���, ����, ��ǲ����, ������, ��ǲ����Ŭ����, InstaneDataStepRate : https://vitacpp.tistory.com/m/70
	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };	// float3
	inputElementDescs[1] = { "VELOCITY", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };	// float3
	inputElementDescs[2] = { "BOARDSIZE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };	// float2
	inputElementDescs[3] = { "LIFETIME", 0, DXGI_FORMAT_R32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };	// float
	inputElementDescs[4] = { "PARTICLETYPE", 0, DXGI_FORMAT_R32_UINT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };	// uint

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = &inputElementDescs[0];
	inputLayoutDesc.NumElements = (UINT)inputElementDescs.size();

	return inputLayoutDesc;
}

void ParticleDrawShader::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	PrepareRender(_pCommandList);

	if (particleResource.textureDescriptorHeap && particleResource.texture && particleResource.texture->GetSrvGpuDescriptorHandle().ptr) {
		_pCommandList->SetDescriptorHeaps(1, particleResource.textureDescriptorHeap.GetAddressOf());

		_pCommandList->SetGraphicsRootDescriptorTable(particleResource.texture->GetRootParameterIndex(), particleResource.texture->GetSrvGpuDescriptorHandle());
		
	}

	_pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_POINTLIST);
	_pCommandList->SOSetTargets(0, 1, NULL);	// SO����� ���� �ʵ��� �����Ѵ�. ȭ�鿡 �׷��� �ϹǷ�

	D3D12_VERTEX_BUFFER_VIEW vertexBuffersViews[1] = { Shader::particleResource.defaultDrawBufferView };
	_pCommandList->IASetVertexBuffers(0, 1, vertexBuffersViews);

	_pCommandList->DrawInstanced((UINT)Shader::particleResource.nDefaultStreamOutputParticle, 1, 0, 0);
}

/////////////////// HitBox Shader
HitBoxShader::HitBoxShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature) {

	Init(_pDevice, _pRootSignature);
	pipelineStateDesc.VS = CompileShaderFromFile(L"Shaders.hlsl", "HitboxVertexShader", "vs_5_1", pVSBlob);
	pipelineStateDesc.PS = CompileShaderFromFile(L"Shaders.hlsl", "HitboxPixelShader", "ps_5_1", pPSBlob);

	_pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)pPipelineState.GetAddressOf());

	pVSBlob.Reset();
	pPSBlob.Reset();

	inputElementDescs.clear();
}
HitBoxShader::~HitBoxShader() {

}

D3D12_RASTERIZER_DESC HitBoxShader::CreateRasterizerState() {
	D3D12_RASTERIZER_DESC rasterizerDesc;
	ZeroMemory(&rasterizerDesc, sizeof(D3D12_RASTERIZER_DESC));
	rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
	rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
	rasterizerDesc.FrontCounterClockwise = FALSE;
	rasterizerDesc.DepthBias = 0;
	rasterizerDesc.DepthBiasClamp = 0.0f;
	rasterizerDesc.SlopeScaledDepthBias = 0.0f;
	rasterizerDesc.DepthClipEnable = TRUE;
	rasterizerDesc.MultisampleEnable = FALSE;
	rasterizerDesc.AntialiasedLineEnable = FALSE;
	rasterizerDesc.ForcedSampleCount = 0;
	rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

	return rasterizerDesc;
}
D3D12_INPUT_LAYOUT_DESC HitBoxShader::CreateInputLayout() {
	inputElementDescs.assign(1, {});

	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = &inputElementDescs[0];
	inputLayoutDesc.NumElements = (UINT)inputElementDescs.size();

	return inputLayoutDesc;
}

void HitBoxShader::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	// �ƹ��͵� ����
}

/////////////////// Mutiple RenderTarget Shader
MultipleRenderTargetShader::MultipleRenderTargetShader(const ComPtr<ID3D12Device>& _pDevice, const ComPtr<ID3D12RootSignature>& _pRootSignature) {

	Init(_pDevice, _pRootSignature);
	pipelineStateDesc.VS = CompileShaderFromFile(L"Shaders.hlsl", "MutipleRenderTargetVertexShader", "vs_5_1", pVSBlob);
	pipelineStateDesc.PS = CompileShaderFromFile(L"Shaders.hlsl", "MutipleRenderTargetPixelShader", "ps_5_1", pPSBlob);

	HRESULT hResult = _pDevice->CreateGraphicsPipelineState(&pipelineStateDesc, __uuidof(ID3D12PipelineState), (void**)pPipelineState.GetAddressOf());

	pVSBlob.Reset();
	pPSBlob.Reset();

	inputElementDescs.clear();
}
MultipleRenderTargetShader::~MultipleRenderTargetShader() {

}

D3D12_DEPTH_STENCIL_DESC MultipleRenderTargetShader::CreateDepthStencilState() {
	D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
	ZeroMemory(&depthStencilDesc, sizeof(D3D12_DEPTH_STENCIL_DESC));
	depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = 0x00;
	depthStencilDesc.StencilWriteMask = 0x00;
	depthStencilDesc.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;
	depthStencilDesc.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_NEVER;

	return depthStencilDesc;
}
D3D12_INPUT_LAYOUT_DESC MultipleRenderTargetShader::CreateInputLayout() {
	inputElementDescs.assign(2, {});

	inputElementDescs[0] = { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	inputElementDescs[1] = { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc;
	inputLayoutDesc.pInputElementDescs = &inputElementDescs[0];
	inputLayoutDesc.NumElements = (UINT)inputElementDescs.size();

	return inputLayoutDesc;
}

void MultipleRenderTargetShader::Render(const ComPtr<ID3D12GraphicsCommandList>& _pCommandList) {
	PrepareRender(_pCommandList);
	MultipleRenderTargetMesh::GetFullScreenRectMesh().Render(_pCommandList);
}
