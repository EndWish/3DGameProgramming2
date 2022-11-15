//-----------------------------------------------------------------------------
// File: Scene.h
//-----------------------------------------------------------------------------

#pragma once

#include "Shader.h"

class CScene
{
public:
    CScene();
    ~CScene();

	bool OnProcessingMouseMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);
	bool OnProcessingKeyboardMessage(HWND hWnd, UINT nMessageID, WPARAM wParam, LPARAM lParam);

	void BuildObjects(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList);
	void ReleaseObjects();

	ID3D12RootSignature *CreateGraphicsRootSignature(ID3D12Device *pd3dDevice);
	ID3D12RootSignature *GetGraphicsRootSignature() { return(m_pd3dGraphicsRootSignature); }
	void SetGraphicsRootSignature(ID3D12GraphicsCommandList *pd3dCommandList) { pd3dCommandList->SetGraphicsRootSignature(m_pd3dGraphicsRootSignature); }

	bool ProcessInput(UCHAR *pKeysBuffer);
    void AnimateObjects(CCamera* pCamera, float fDeltaTime);

	void OnPrepareRender(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	void Render(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera = NULL);

	void RenderParticle(ID3D12GraphicsCommandList* pd3dCommandList, CCamera* pCamera);
	void OnPostRenderParticle();

	void ReleaseUploadBuffers();

protected:
	CGameObject**				m_ppObjects = NULL;
	int							m_nObjects = 0;

	CParticleObject**			m_ppParticleObjects = NULL;
	int							m_nParticleObjects = 0;

	ID3D12RootSignature			*m_pd3dGraphicsRootSignature = NULL;
};
