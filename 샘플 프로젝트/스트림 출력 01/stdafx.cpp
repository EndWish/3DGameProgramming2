// stdafx.cpp : 표준 포함 파일만 들어 있는 소스 파일입니다.
// LabProject03-1.pch는 미리 컴파일된 헤더가 됩니다.
// stdafx.obj에는 미리 컴파일된 형식 정보가 포함됩니다.

#include "stdafx.h"
#include "DDSTextureLoader12.h"

int gnCurrentParticles = 0;

UINT gnCbvSrvDescriptorIncrementSize = 0;

void OutputString(_TCHAR *pString, float fValue)
{
	TCHAR pstrDebug[256] = { 0 };

	_stprintf_s(pstrDebug, 256, _T("%s %f\n"), pString, fValue);
	OutputDebugString(pstrDebug);
}

void OutputString(_TCHAR *pString, int nValue)
{
	TCHAR pstrDebug[256] = { 0 };

	_stprintf_s(pstrDebug, 256, _T("%s %d\n"), pString, nValue);
	OutputDebugString(pstrDebug);
}

void OutputString(_TCHAR *pString, UINT64 nValue)
{
	TCHAR pstrDebug[256] = { 0 };

	_stprintf_s(pstrDebug, 256, _T("%s %u %u\n"), pString, (unsigned int)((nValue >> 32) & 0x0000ffff), (unsigned int)(nValue & 0x0000ffff));
	OutputDebugString(pstrDebug);
}

void WaitForGpuComplete(ID3D12CommandQueue* pd3dCommandQueue, ID3D12Fence* pd3dFence, UINT64 nFenceValue, HANDLE hFenceEvent)
{
	HRESULT hResult = pd3dCommandQueue->Signal(pd3dFence, nFenceValue);

	if (pd3dFence->GetCompletedValue() < nFenceValue)
	{
		hResult = pd3dFence->SetEventOnCompletion(nFenceValue, hFenceEvent);
		::WaitForSingleObject(hFenceEvent, INFINITE);
	}
}

void ExecuteCommandList(ID3D12GraphicsCommandList* pd3dCommandList, ID3D12CommandQueue* pd3dCommandQueue, ID3D12Fence* pd3dFence, UINT64 nFenceValue, HANDLE hFenceEvent)
{
	pd3dCommandList->Close();

	ID3D12CommandList* ppd3dCommandLists[] = { pd3dCommandList };
	pd3dCommandQueue->ExecuteCommandLists(1, ppd3dCommandLists);

	::WaitForGpuComplete(pd3dCommandQueue, pd3dFence, nFenceValue, hFenceEvent);
}

void SynchronizeResourceTransition(ID3D12GraphicsCommandList* pd3dCommandList, ID3D12Resource* pd3dResource, D3D12_RESOURCE_STATES d3dStateBefore, D3D12_RESOURCE_STATES d3dStateAfter)
{
	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = pd3dResource;
	d3dResourceBarrier.Transition.StateBefore = d3dStateBefore;
	d3dResourceBarrier.Transition.StateAfter = d3dStateAfter;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);
}

inline void MemcpySubresource(D3D12_MEMCPY_DEST* pDest, D3D12_SUBRESOURCE_DATA* pSrc, SIZE_T RowSizeInBytes, UINT NumRows, UINT NumSlices)
{
	for (UINT z = 0; z < NumSlices; ++z)
	{
		BYTE* pDestSlice = reinterpret_cast<BYTE*>(pDest->pData) + pDest->SlicePitch * z;
		const BYTE* pSrcSlice = reinterpret_cast<const BYTE*>(pSrc->pData) + pSrc->SlicePitch * z;
		for (UINT y = 0; y < NumRows; ++y)
		{
			memcpy(pDestSlice + pDest->RowPitch * y, pSrcSlice + pSrc->RowPitch * y, RowSizeInBytes);
		}
	}
}

inline UINT64 UpdateSubresources(ID3D12GraphicsCommandList* pd3dCommandList, ID3D12Resource* pd3dResource, ID3D12Resource* pd3dIntermediate, UINT nFirstSubresource, UINT nSubresources, UINT64 nRequiredSize, D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pd3dLayouts, UINT* pnRows, UINT64* pnRowSizesInBytes, D3D12_SUBRESOURCE_DATA* pd3dSrcData)
{
	D3D12_RESOURCE_DESC d3dIntermediateResourceDesc = pd3dIntermediate->GetDesc();
	D3D12_RESOURCE_DESC d3dDestinationResourceDesc = pd3dResource->GetDesc();
	if ((d3dIntermediateResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_BUFFER) || (d3dIntermediateResourceDesc.Width < nRequiredSize + pd3dLayouts[0].Offset) || (nRequiredSize > (SIZE_T)-1) || (d3dDestinationResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER && (nFirstSubresource != 0 || nSubresources != 1))) return(0);

	BYTE* pMappedData;
	if (FAILED(pd3dIntermediate->Map(0, NULL, reinterpret_cast<void**>(&pMappedData)))) return(0);

	for (UINT i = 0; i < nSubresources; i++)
	{
		if (pnRowSizesInBytes[i] > (SIZE_T)-1) return(0);
		D3D12_MEMCPY_DEST pd3dDestData = { pMappedData + pd3dLayouts[i].Offset, pd3dLayouts[i].Footprint.RowPitch, pd3dLayouts[i].Footprint.RowPitch * pnRows[i] };
		MemcpySubresource(&pd3dDestData, &pd3dSrcData[i], (SIZE_T)pnRowSizesInBytes[i], pnRows[i], pd3dLayouts[i].Footprint.Depth);
	}
	pd3dIntermediate->Unmap(0, NULL);

	if (d3dDestinationResourceDesc.Dimension == D3D12_RESOURCE_DIMENSION_BUFFER)
	{
		pd3dCommandList->CopyBufferRegion(pd3dResource, 0, pd3dIntermediate, pd3dLayouts[0].Offset, pd3dLayouts[0].Footprint.Width);
	}
	else
	{
		for (UINT i = 0; i < nSubresources; i++)
		{
			D3D12_TEXTURE_COPY_LOCATION d3dCopyDst, d3dCopySrc;
			d3dCopyDst.pResource = pd3dResource;
			d3dCopyDst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
			d3dCopyDst.SubresourceIndex = i + nFirstSubresource;
			d3dCopySrc.pResource = pd3dIntermediate;
			d3dCopySrc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
			d3dCopySrc.PlacedFootprint = pd3dLayouts[i];
//			D3D12_TEXTURE_COPY_LOCATION d3dCopyDst = { pd3dResource, D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX, i + nFirstSubresource };
//			D3D12_TEXTURE_COPY_LOCATION d3dCopySrc = { pd3dIntermediate, D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT, pd3dLayouts[i] };
			pd3dCommandList->CopyTextureRegion(&d3dCopyDst, 0, 0, 0, &d3dCopySrc, NULL);
		}
	}
	return(nRequiredSize);
}

inline UINT64 UpdateSubresources(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12Resource* pd3dResource, ID3D12Resource* pd3dIntermediate, UINT64 IntermediateOffset, UINT nFirstSubresource, UINT nSubresources, D3D12_SUBRESOURCE_DATA* pd3dSrcData)
{
	UINT64 nMemToAlloc = static_cast<UINT64>(sizeof(D3D12_PLACED_SUBRESOURCE_FOOTPRINT) + sizeof(UINT) + sizeof(UINT64)) * nSubresources;
	if (nMemToAlloc > SIZE_MAX) return(0);

	void* pMemAlloced = HeapAlloc(GetProcessHeap(), 0, static_cast<SIZE_T>(nMemToAlloc));
	if (pMemAlloced == NULL) return(0);

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT* pd3dLayouts = reinterpret_cast<D3D12_PLACED_SUBRESOURCE_FOOTPRINT *>(pMemAlloced);
	UINT64* pnRowSizesInBytes = reinterpret_cast<UINT64 *>(pd3dLayouts + nSubresources);
	UINT* pnRows = reinterpret_cast<UINT *>(pnRowSizesInBytes + nSubresources);

	UINT64 nRequiredSize = 0;
	D3D12_RESOURCE_DESC d3dResourceDesc = pd3dResource->GetDesc();
	pd3dDevice->GetCopyableFootprints(&d3dResourceDesc, nFirstSubresource, nSubresources, IntermediateOffset, pd3dLayouts, pnRows, pnRowSizesInBytes, &nRequiredSize);

	UINT64 nResult = UpdateSubresources(pd3dCommandList, pd3dResource, pd3dIntermediate, nFirstSubresource, nSubresources, nRequiredSize, pd3dLayouts, pnRows, pnRowSizesInBytes, pd3dSrcData);
	HeapFree(GetProcessHeap(), 0, pMemAlloced);

	return(nResult);
}

template <UINT nMaxSubresources>
inline UINT64 UpdateSubresources(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, ID3D12Resource* pd3dResource, ID3D12Resource* pd3dIntermediate, UINT64 nIntermediateOffset, UINT nFirstSubresource, UINT nSubresources, D3D12_SUBRESOURCE_DATA* pd3dSrcData)
{
	D3D12_PLACED_SUBRESOURCE_FOOTPRINT pnd3dLayouts[nMaxSubresources];
	UINT pnRows[nMaxSubresources];
	UINT64 pnRowSizesInBytes[nMaxSubresources];

	UINT64 nRequiredSize = 0;
	D3D12_RESOURCE_DESC d3dResourceDesc = pd3dResource->GetDesc();
	pd3dDevice->GetCopyableFootprints(&d3dResourceDesc, nFirstSubresource, nSubresources, nIntermediateOffset, pnd3dLayouts, pnRows, pnRowSizesInBytes, &nRequiredSize);

	return(UpdateSubresources(pd3dCommandList, pd3dResource, pd3dIntermediate, nFirstSubresource, nSubresources, nRequiredSize, pnd3dLayouts, pnRows, pnRowSizesInBytes, pd3dSrcData));
}

ID3D12Resource* CreateTextureResource(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nBytes, D3D12_RESOURCE_DIMENSION d3dResourceDimension, UINT nWidth, UINT nHeight, UINT nDepthOrArraySize, UINT nMipLevels, D3D12_RESOURCE_FLAGS d3dResourceFlags, DXGI_FORMAT dxgiFormat, D3D12_HEAP_TYPE d3dHeapType, D3D12_RESOURCE_STATES d3dResourceStates, ID3D12Resource** ppd3dUploadBuffer)
{
	ID3D12Resource* pd3dBuffer = NULL;

	D3D12_HEAP_PROPERTIES d3dHeapPropertiesDesc;
	::ZeroMemory(&d3dHeapPropertiesDesc, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapPropertiesDesc.Type = d3dHeapType;
	d3dHeapPropertiesDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapPropertiesDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapPropertiesDesc.CreationNodeMask = 1;
	d3dHeapPropertiesDesc.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC d3dResourceDesc;
	::ZeroMemory(&d3dResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	d3dResourceDesc.Dimension = d3dResourceDimension; //D3D12_RESOURCE_DIMENSION_BUFFER, D3D12_RESOURCE_DIMENSION_TEXTURE1D, D3D12_RESOURCE_DIMENSION_TEXTURE2D
	d3dResourceDesc.Alignment = (d3dResourceDimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT : 0;
	d3dResourceDesc.Width = (d3dResourceDimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? nBytes : nWidth;
	d3dResourceDesc.Height = (d3dResourceDimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? 1 : nHeight;
	d3dResourceDesc.DepthOrArraySize = (d3dResourceDimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? 1 : nDepthOrArraySize;
	d3dResourceDesc.MipLevels = (d3dResourceDimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? 1 : nMipLevels;
	d3dResourceDesc.Format = (d3dResourceDimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? DXGI_FORMAT_UNKNOWN : dxgiFormat;
	d3dResourceDesc.SampleDesc.Count = 1;
	d3dResourceDesc.SampleDesc.Quality = 0;
	d3dResourceDesc.Layout = (d3dResourceDimension == D3D12_RESOURCE_DIMENSION_BUFFER) ? D3D12_TEXTURE_LAYOUT_ROW_MAJOR : D3D12_TEXTURE_LAYOUT_UNKNOWN;
	d3dResourceDesc.Flags = d3dResourceFlags; //D3D12_RESOURCE_FLAG_NONE

	switch (d3dHeapType)
	{
		case D3D12_HEAP_TYPE_DEFAULT:
		{
			D3D12_RESOURCE_STATES d3dResourceInitialStates = (ppd3dUploadBuffer && pData) ? D3D12_RESOURCE_STATE_COPY_DEST : d3dResourceStates;
			HRESULT hResult = pd3dDevice->CreateCommittedResource(&d3dHeapPropertiesDesc, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, d3dResourceInitialStates, NULL, __uuidof(ID3D12Resource), (void**)&pd3dBuffer);
			if (ppd3dUploadBuffer && pData)
			{
				d3dHeapPropertiesDesc.Type = D3D12_HEAP_TYPE_UPLOAD;

				d3dResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				d3dResourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
				d3dResourceDesc.Width = nBytes;
				d3dResourceDesc.Height = 1;
				d3dResourceDesc.DepthOrArraySize = 1;
				d3dResourceDesc.MipLevels = 1;
				d3dResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
				d3dResourceDesc.SampleDesc.Count = 1;
				d3dResourceDesc.SampleDesc.Quality = 0;
				d3dResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				d3dResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
				hResult = pd3dDevice->CreateCommittedResource(&d3dHeapPropertiesDesc, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, __uuidof(ID3D12Resource), (void**)ppd3dUploadBuffer);
#ifdef _WITH_MAPPING
				D3D12_RANGE d3dReadRange = { 0, 0 };
				UINT8* pBufferDataBegin = NULL;
				(*ppd3dUploadBuffer)->Map(0, &d3dReadRange, (void**)&pBufferDataBegin);
				memcpy(pBufferDataBegin, pData, nBytes);
				(*ppd3dUploadBuffer)->Unmap(0, NULL);

				pd3dCommandList->CopyResource(pd3dBuffer, *ppd3dUploadBuffer);
#else
				D3D12_SUBRESOURCE_DATA d3dSubResourceData;
				::ZeroMemory(&d3dSubResourceData, sizeof(D3D12_SUBRESOURCE_DATA));
				d3dSubResourceData.pData = pData;
				d3dSubResourceData.SlicePitch = d3dSubResourceData.RowPitch = nBytes;
				::UpdateSubresources<1>(pd3dDevice, pd3dCommandList, pd3dBuffer, *ppd3dUploadBuffer, 0, 0, 1, &d3dSubResourceData);
#endif
				D3D12_RESOURCE_BARRIER d3dResourceBarrier;
				::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
				d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
				d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
				d3dResourceBarrier.Transition.pResource = pd3dBuffer;
				d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
				d3dResourceBarrier.Transition.StateAfter = d3dResourceStates;
				d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
				pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);
			}
			break;
		}
		case D3D12_HEAP_TYPE_UPLOAD:
		{
			d3dResourceStates |= D3D12_RESOURCE_STATE_GENERIC_READ;
			HRESULT hResult = pd3dDevice->CreateCommittedResource(&d3dHeapPropertiesDesc, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, d3dResourceStates, NULL, __uuidof(ID3D12Resource), (void**)&pd3dBuffer);
			if (pData)
			{
				D3D12_RANGE d3dReadRange = { 0, 0 };
				UINT8* pBufferDataBegin = NULL;
				pd3dBuffer->Map(0, &d3dReadRange, (void**)&pBufferDataBegin);
				memcpy(pBufferDataBegin, pData, nBytes);
				pd3dBuffer->Unmap(0, NULL);
			}
			break;
		}
		case D3D12_HEAP_TYPE_READBACK:
		{
			d3dResourceStates |= D3D12_RESOURCE_STATE_COPY_DEST;
			HRESULT hResult = pd3dDevice->CreateCommittedResource(&d3dHeapPropertiesDesc, D3D12_HEAP_FLAG_NONE, &d3dResourceDesc, d3dResourceStates, NULL, __uuidof(ID3D12Resource), (void**)&pd3dBuffer);
			if (pData)
			{
				D3D12_RANGE d3dReadRange = { 0, 0 };
				UINT8* pBufferDataBegin = NULL;
				pd3dBuffer->Map(0, &d3dReadRange, (void**)&pBufferDataBegin);
				memcpy(pBufferDataBegin, pData, nBytes);
				pd3dBuffer->Unmap(0, NULL);
			}
			break;
		}
	}
	return(pd3dBuffer);
}

ID3D12Resource* CreateBufferResource(ID3D12Device* pd3dDevice, ID3D12GraphicsCommandList* pd3dCommandList, void* pData, UINT nBytes, D3D12_HEAP_TYPE d3dHeapType, D3D12_RESOURCE_STATES d3dResourceStates, ID3D12Resource** ppd3dUploadBuffer)
{
	return(CreateTextureResource(pd3dDevice, pd3dCommandList, pData, nBytes, D3D12_RESOURCE_DIMENSION_BUFFER, nBytes, 1, 1, 1, D3D12_RESOURCE_FLAG_NONE, DXGI_FORMAT_UNKNOWN, d3dHeapType, d3dResourceStates, ppd3dUploadBuffer));
}

ID3D12Resource *CreateTextureResourceFromDDSFile(ID3D12Device *pd3dDevice, ID3D12GraphicsCommandList *pd3dCommandList, wchar_t *pszFileName, ID3D12Resource **ppd3dUploadBuffer, D3D12_RESOURCE_STATES d3dResourceStates)
{
	ID3D12Resource *pd3dTexture = NULL;
	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> vSubresources;
	DDS_ALPHA_MODE ddsAlphaMode = DDS_ALPHA_MODE_UNKNOWN;
	bool bIsCubeMap = false;

	HRESULT hResult = DirectX::LoadDDSTextureFromFileEx(pd3dDevice, pszFileName, 0, D3D12_RESOURCE_FLAG_NONE, DDS_LOADER_DEFAULT, &pd3dTexture, ddsData, vSubresources, &ddsAlphaMode, &bIsCubeMap);

	D3D12_HEAP_PROPERTIES d3dHeapPropertiesDesc;
	::ZeroMemory(&d3dHeapPropertiesDesc, sizeof(D3D12_HEAP_PROPERTIES));
	d3dHeapPropertiesDesc.Type = D3D12_HEAP_TYPE_UPLOAD;
	d3dHeapPropertiesDesc.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	d3dHeapPropertiesDesc.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	d3dHeapPropertiesDesc.CreationNodeMask = 1;
	d3dHeapPropertiesDesc.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC d3dTextureResourceDesc = pd3dTexture->GetDesc();
	UINT nSubResources = d3dTextureResourceDesc.DepthOrArraySize * d3dTextureResourceDesc.MipLevels; //nSubResources = (UINT)vSubresources.size();
	UINT64 nBytes = 0;
	pd3dDevice->GetCopyableFootprints(&d3dTextureResourceDesc, 0, nSubResources, 0, NULL, NULL, NULL, &nBytes);

	D3D12_RESOURCE_DESC d3dUploadResourceDesc;
	::ZeroMemory(&d3dUploadResourceDesc, sizeof(D3D12_RESOURCE_DESC));
	d3dUploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER; //Upload Heap에는 텍스쳐를 생성할 수 없음
	d3dUploadResourceDesc.Alignment = 0;
	d3dUploadResourceDesc.Width = nBytes;
	d3dUploadResourceDesc.Height = 1;
	d3dUploadResourceDesc.DepthOrArraySize = 1;
	d3dUploadResourceDesc.MipLevels = 1;
	d3dUploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	d3dUploadResourceDesc.SampleDesc.Count = 1;
	d3dUploadResourceDesc.SampleDesc.Quality = 0;
	d3dUploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	d3dUploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	pd3dDevice->CreateCommittedResource(&d3dHeapPropertiesDesc, D3D12_HEAP_FLAG_NONE, &d3dUploadResourceDesc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, __uuidof(ID3D12Resource), (void **)ppd3dUploadBuffer);

	//UINT nSubResources = (UINT)vSubresources.size();
	//D3D12_SUBRESOURCE_DATA *pd3dSubResourceData = new D3D12_SUBRESOURCE_DATA[nSubResources];
	//for (UINT i = 0; i < nSubResources; i++) pd3dSubResourceData[i] = vSubresources.at(i);

	//	std::vector<D3D12_SUBRESOURCE_DATA>::pointer ptr = &vSubresources[0];
	::UpdateSubresources(pd3dDevice, pd3dCommandList, pd3dTexture, *ppd3dUploadBuffer, 0, 0, nSubResources, &vSubresources[0]);

	D3D12_RESOURCE_BARRIER d3dResourceBarrier;
	::ZeroMemory(&d3dResourceBarrier, sizeof(D3D12_RESOURCE_BARRIER));
	d3dResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	d3dResourceBarrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	d3dResourceBarrier.Transition.pResource = pd3dTexture;
	d3dResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	d3dResourceBarrier.Transition.StateAfter = d3dResourceStates;
	d3dResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	pd3dCommandList->ResourceBarrier(1, &d3dResourceBarrier);

	//	delete[] pd3dSubResourceData;

	return(pd3dTexture);
}

void SwapResourcePointer(ID3D12Resource** ppd3dResourceA, ID3D12Resource** ppd3dResourceB)
{
	ID3D12Resource* pd3dTempResource = *ppd3dResourceA;
	*ppd3dResourceA = *ppd3dResourceB;
	*ppd3dResourceB = pd3dTempResource;
}

