#include "EnginePCH.h"
#include "RenderContext.h"

#include "Engine.h"


namespace Engine
{
    RenderContext::RenderContext()
    {
        EnableDebugLayer();
        CreateFactory();
        CreateDevice();
        RetrieveDescriptorIncrementSizes();
    }

    void RenderContext::EnableDebugLayer()
    {
#ifdef _DEBUG
        Microsoft::WRL::ComPtr<ID3D12Debug> debugController = nullptr;
        EG_CONFIRM(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))));
        Microsoft::WRL::ComPtr<ID3D12Debug5> debugController5 = nullptr;
        EG_CONFIRM(SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController5))));
        debugController5->EnableDebugLayer();
        debugController5->SetEnableGPUBasedValidation(true);
        debugController5->SetEnableAutoName(true);

#endif
    }

    void RenderContext::CreateFactory()
    {
        uint32_t dxgiFactoryFlag = 0;
#ifdef _DEBUG
        dxgiFactoryFlag = DXGI_CREATE_FACTORY_DEBUG;

        Microsoft::WRL::ComPtr<IDXGIInfoQueue> dxgiInfoQueue = nullptr;
        EG_CONFIRM(SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))));
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);
#endif

        EG_CONFIRM(SUCCEEDED(CreateDXGIFactory2(dxgiFactoryFlag, IID_PPV_ARGS(&m_Factory))));
    }

    void RenderContext::CreateDevice()
    {
        Microsoft::WRL::ComPtr<IDXGIAdapter1> adapter = nullptr;
        EG_CONFIRM(SUCCEEDED(m_Factory->EnumAdapters1(0, adapter.GetAddressOf())));
        DXGI_ADAPTER_DESC1 adapterDesc{};
        EG_CONFIRM(SUCCEEDED(adapter->GetDesc1(&adapterDesc)));
        EG_CONFIRM(SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device))));

#ifdef _DEBUG
        Microsoft::WRL::ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
        EG_CONFIRM(SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&infoQueue))));
        EG_CONFIRM(SUCCEEDED(infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true)));
        EG_CONFIRM(SUCCEEDED(infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true)));
        EG_CONFIRM(SUCCEEDED(infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true)));

        D3D12_MESSAGE_ID denyIds[] =
        {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE
        };
        D3D12_INFO_QUEUE_FILTER infoQueueFilter = {};
        infoQueueFilter.DenyList.NumIDs = _countof(denyIds);
        infoQueueFilter.DenyList.pIDList = denyIds;
        EG_CONFIRM(SUCCEEDED(infoQueue->AddStorageFilterEntries(&infoQueueFilter)));

#endif
    }

    void RenderContext::RetrieveDescriptorIncrementSizes()
    {
        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        {
            m_DescriptorIncrementSizes[i] = m_Device->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
        }
    }
}
