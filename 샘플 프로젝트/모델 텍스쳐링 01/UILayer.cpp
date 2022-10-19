#include "stdafx.h"
#include "UILayer.h"

using namespace std;

UILayer::UILayer(UINT nFrame, ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue)
{
    m_fWidth = 0.0f;
    m_fHeight = 0.0f;
    m_vWrappedRenderTargets.resize(nFrame);
    m_vd2dRenderTargets.resize(nFrame);
    m_vTextBlocks.resize(1);
    Initialize(pd3dDevice, pd3dCommandQueue);
}

void UILayer::Initialize(ID3D12Device* pd3dDevice, ID3D12CommandQueue* pd3dCommandQueue)
{
    UINT d3d11DeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D2D1_FACTORY_OPTIONS d2dFactoryOptions = { };

#if defined(_DEBUG) || defined(DBG)
    d2dFactoryOptions.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION;
    d3d11DeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    ID3D11Device* pd3d11Device;
    ID3D12CommandQueue* ppd3dCommandQueues[] = { pd3dCommandQueue };
    ::D3D11On12CreateDevice(pd3dDevice, d3d11DeviceFlags, nullptr, 0, reinterpret_cast<IUnknown**>(ppd3dCommandQueues), _countof(ppd3dCommandQueues), 0, &d3d11Device, &m_pd3d11DeviceContext, nullptr);

    d3d11Device.As(&m_pd3d11On12Device);

#if defined(_DEBUG) || defined(DBG)
    ID3D12InfoQueue* pd3dInfoQueue;
    if (SUCCEEDED(pd3dDevice->QueryInterface(IID_PPV_ARGS(&pd3dInfoQueue))))
    {
        D3D12_MESSAGE_SEVERITY pd3dSeverities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO,
        };

        D3D12_MESSAGE_ID pd3dDenyIds[] =
        {
            D3D12_MESSAGE_ID_INVALID_DESCRIPTOR_HANDLE,
        };

        D3D12_INFO_QUEUE_FILTER d3dInforQueueFilter = { };
        d3dInforQueueFilter.DenyList.NumSeverities = _countof(pd3dSeverities);
        d3dInforQueueFilter.DenyList.pSeverityList = pd3dSeverities;
        d3dInforQueueFilter.DenyList.NumIDs = _countof(pd3dDenyIds);
        d3dInforQueueFilter.DenyList.pIDList = pd3dDenyIds;

        pd3dInfoQueue->PushStorageFilter(&d3dInforQueueFilter);
    }
#endif

    {
        IDXGIDevice* ppdxgiDevice;
        m_pd3d11On12Device.As(&pdxgiDevice);

        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory3), &d2dFactoryOptions, &m_pd2dFactory);
        m_pd2dFactory->CreateDevice(pdxgiDevice, &m_pd2dDevice);
        m_pd2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_pd2dDeviceContext);

        m_pd2dDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
        ThrowIfFailed(m_pd2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_textBrush));

        ThrowIfFailed(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), &m_pd3dWriteFactory));
    }
}

void UILayer::UpdateLabels(const wstring& uiText)
{
    // Update the UI elements.
    m_vTextBlocks[0] = { uiText, D2D1::RectF(0.0f, 0.0f, m_nWidth, m_nHeight), m_textFormat.Get() };
}

void UILayer::Render(UINT nFrame)
{
    ID3D11Resource* ppResources[] = { m_vWrappedRenderTargets[nFrame].Get() };

    m_pd2dDeviceContext->SetTarget(m_vd2dRenderTargets[nFrame].Get());

    // Acquire our wrapped render target resource for the current back buffer.
    m_pd3d11On12Device->AcquireWrappedResources(ppResources, _countof(ppResources));
    m_pd2dDeviceContext->BeginDraw();
    for (auto textBlock : m_vTextBlocks)
    {
        m_pd2dDeviceContext->DrawText(
            textBlock.text.c_str(),
            static_cast<UINT>(textBlock.text.length()),
            textBlock.pFormat,
            textBlock.layout,
            m_textBrush.Get());
    }
    m_pd2dDeviceContext->EndDraw();

    // Release our wrapped render target resource. Releasing
    // transitions the back buffer resource to the state specified
    // as the OutState when the wrapped resource was created.
    m_pd3d11On12Device->ReleaseWrappedResources(ppResources, _countof(ppResources));

    // Flush to submit the 11 command list to the shared command queue.
    m_pd3d11DeviceContext->Flush();
}

// Releases resources that reference the DXGI swap chain so that it can be resized.
void UILayer::ReleaseResources()
{
    for (UINT i = 0; i < FrameCount(); i++)
    {
        ID3D11Resource* ppResources[] = { m_vWrappedRenderTargets[i].Get() };
        m_pd3d11On12Device->ReleaseWrappedResources(ppResources, _countof(ppResources));
    }
    m_pd2dDeviceContext->SetTarget(nullptr);
    m_pd3d11DeviceContext->Flush();
    for (UINT i = 0; i < FrameCount(); i++)
    {
        m_vd2dRenderTargets[i].Reset();
        m_vWrappedRenderTargets[i].Reset();
    }
    m_textBrush.Reset();
    m_pd2dDeviceContext.Reset();
    m_textFormat.Reset();
    m_pd3dWriteFactory.Reset();
    m_pd2dDevice.Reset();
    m_pd2dFactory.Reset();
    m_pd3d11DeviceContext.Reset();
    m_pd3d11On12Device.Reset();
}

void UILayer::Resize(ComPtr<ID3D12Resource>* ppd3dRenderTargets, UINT width, UINT height)
{
    m_nWidth = static_cast<float>(width);
    m_nHeight = static_cast<float>(height);

    // Query the desktop's dpi settings, which will be used to create
    // D2D's render targets.
    D2D1_BITMAP_PROPERTIES1 bitmapProperties = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET | D2D1_BITMAP_OPTIONS_CANNOT_DRAW,
        D2D1::PixelFormat(DXGI_FORMAT_UNKNOWN, D2D1_ALPHA_MODE_PREMULTIPLIED));

    // Create a wrapped 11On12 resource of this back buffer. Since we are 
    // rendering all D3D12 content first and then all D2D content, we specify 
    // the In resource state as RENDER_TARGET - because D3D12 will have last 
    // used it in this state - and the Out resource state as PRESENT. When 
    // ReleaseWrappedResources() is called on the 11On12 device, the resource 
    // will be transitioned to the PRESENT state.
    for (UINT i = 0; i < FrameCount(); i++)
    {
        D3D11_RESOURCE_FLAGS d3d11Flags = { D3D11_BIND_RENDER_TARGET };
        ThrowIfFailed(m_pd3d11On12Device->CreateWrappedResource(
            ppd3dRenderTargets[i].Get(),
            &d3d11Flags,
            D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_STATE_PRESENT,
            IID_PPV_ARGS(&m_vWrappedRenderTargets[i])));

        // Create a render target for D2D to draw directly to this back buffer.
        ComPtr<IDXGISurface> surface;
        ThrowIfFailed(m_vWrappedRenderTargets[i].As(&surface));
        ThrowIfFailed(m_pd2dDeviceContext->CreateBitmapFromDxgiSurface(
            surface.Get(),
            &bitmapProperties,
            &m_vd2dRenderTargets[i]));
    }

    ThrowIfFailed(m_pd2dDevice->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &m_pd2dDeviceContext));
    m_pd2dDeviceContext->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    ThrowIfFailed(m_pd2dDeviceContext->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &m_textBrush));

    // Create DWrite text format objects.
    const float fontSize = m_nHeight / 30.0f;
    const float smallFontSize = m_nHeight / 40.0f;

    ThrowIfFailed(m_pd3dWriteFactory->CreateTextFormat(
        L"Arial",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        fontSize,
        L"en-us",
        &m_textFormat));

    ThrowIfFailed(m_textFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_CENTER));
    ThrowIfFailed(m_textFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR));
    ThrowIfFailed(m_pd3dWriteFactory->CreateTextFormat(
        L"Arial",
        nullptr,
        DWRITE_FONT_WEIGHT_NORMAL,
        DWRITE_FONT_STYLE_NORMAL,
        DWRITE_FONT_STRETCH_NORMAL,
        smallFontSize,
        L"en-us",
        &m_textFormat));
}