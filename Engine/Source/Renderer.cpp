#include "Renderer.h"
#include <dxgidebug.h>
#include <d3dcompiler.h>
#include <DirectXColors.h>

#include "d3dx12.h"
#include "Mesh.h"
#include "ResourceManager.h"
#include "Texture.h"


#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")



#ifdef _DEBUG
#define CONFIRM_ENABLED 1
#else
#define CONFIRM_ENABLED 0
#endif

#if CONFIRM_ENABLED == 1
#define GP_CONFIRM(x) \
if(!(x))\
{\
__debugbreak();\
}
#else
#define GP_CONFIRM(x) x
#endif

namespace Engine
{
    void Renderer::Initialize(HWND windowHandle, uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;
        InitDirectX(windowHandle);
        Prepare();
    }

    void Renderer::InitDirectX(HWND windowHandle)
    {
        EnableDebugLayer();
        CreateFactory();
        CreateDevice();
        CreateCommandQueue();
        CreateCommandList();
        CreateDescriptorHeaps();
        CreateSwapChain(windowHandle);
        CreateRenderTarget();
        CreateFence();
    }

    void Renderer::Prepare()
    {
        LoadAssets();
        CreatePipelineState();
        CreateVertexAndIndexBuffer();
    }

    void Renderer::Render()
    {
        const uint32_t currentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(),
                                                m_DescriptorHeapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] * currentBackBufferIndex);
        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart(), 0);


        m_CommandAllocator->Reset();
        m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);
        m_CommandList->RSSetViewports(1, &m_Viewport);
        m_CommandList->RSSetScissorRects(1, &m_ScissorRect);
        m_CommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);


        {
            const auto& resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_SwapChainBuffers[currentBackBufferIndex].Get(),
                                                                               D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            m_CommandList->ResourceBarrier(1, &resourceBarrier);
        }

        m_CommandList->ClearRenderTargetView(rtvHandle, Colors::DarkSlateGray, 0, nullptr);
        m_CommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        m_CommandList->SetPipelineState(m_PipelineState.Get());
        m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

        static float angle = 0.0f;
        angle += 0.016f * 1.0f;
        SimpleMath::Matrix model = SimpleMath::Matrix::CreateScale(1.0f)
            //* SimpleMath::Matrix::CreateFromYawPitchRoll(angle, XMConvertToRadians(90.0f), 0.0f)
            * SimpleMath::Matrix::CreateFromYawPitchRoll(angle, XMConvertToRadians(0.0f), 0.0f)
            * SimpleMath::Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);


        constexpr SimpleMath::Vector3 eye = {0.0f, 0.0f, -40.0f};
        constexpr SimpleMath::Vector3 target = {0.0f, 0.0f, 0.0f};
        constexpr SimpleMath::Vector3 up = {0.0f, 1.0f, 0.0f};
        const SimpleMath::Matrix view = XMMatrixLookAtLH(eye, target, up);

        constexpr float fieldOfView = XMConvertToRadians(90.0f);
        const float aspectRatio = m_Width / static_cast<float>(m_Height);
        constexpr float nearPlane = 0.1f;
        constexpr float farPlane = 1000.0f;
        const SimpleMath::Matrix projection = XMMatrixPerspectiveFovLH(fieldOfView, aspectRatio, nearPlane, farPlane);
        SimpleMath::Matrix viewProjection = view * projection;

        struct TransformData
        {
            SimpleMath::Matrix MatGeo;
            SimpleMath::Matrix MatGeoInvert;
            SimpleMath::Matrix MatVP;
        } transformData;

        transformData.MatGeo = model.Transpose();
        transformData.MatGeoInvert = model.Invert().Transpose();
        transformData.MatVP = viewProjection.Transpose();

        m_CommandList->SetGraphicsRoot32BitConstants(0, 48, &transformData, 0);

        struct LightData
        {
            SimpleMath::Vector4 LightPosition;
            SimpleMath::Color LightColor;
            SimpleMath::Vector4 CameraPosition;
        } lightData;

        lightData.LightPosition = {0.0f, 0.0f, -100.0f, 0.0f};
        lightData.LightColor = Colors::LightGoldenrodYellow;
        lightData.CameraPosition = SimpleMath::Vector4(eye.x, eye.y, eye.z, 1.0f);


        m_CommandList->SetGraphicsRoot32BitConstants(1, 12, &lightData, 0);
        m_CommandList->SetDescriptorHeaps(1, m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetAddressOf());
        m_CommandList->SetGraphicsRootDescriptorTable(
            2, m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetGPUDescriptorHandleForHeapStart());

        m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_CommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
        m_CommandList->IASetIndexBuffer(&m_IndexBufferView);
        m_CommandList->DrawIndexedInstanced(static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);


        //ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), m_CommandList.Get());


        {
            const auto& resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_SwapChainBuffers[currentBackBufferIndex].Get(),
                                                                               D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            m_CommandList->ResourceBarrier(1, &resourceBarrier);
        }


        m_CommandList->Close();
        m_CommandQueue->ExecuteCommandLists(1, CommandListCast(m_CommandList.GetAddressOf()));
        m_SwapChain->Present(0, 0);

        WaitForGPU();
    }


    void Renderer::EnableDebugLayer()
    {
#ifdef _DEBUG
        ComPtr<ID3D12Debug> debugController = nullptr;
        GP_CONFIRM(SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))));
        ComPtr<ID3D12Debug5> debugController5 = nullptr;
        GP_CONFIRM(SUCCEEDED(debugController->QueryInterface(IID_PPV_ARGS(&debugController5))));
        debugController5->EnableDebugLayer();
        debugController5->SetEnableGPUBasedValidation(true);
        debugController5->SetEnableAutoName(true);

#endif
    }

    void Renderer::CreateFactory()
    {
        uint32_t dxgiFactoryFlag = 0;
#ifdef _DEBUG
        dxgiFactoryFlag = DXGI_CREATE_FACTORY_DEBUG;

        ComPtr<IDXGIInfoQueue> dxgiInfoQueue = nullptr;
        GP_CONFIRM(SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgiInfoQueue.GetAddressOf()))));
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_CORRUPTION, true);
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_ERROR, true);
        dxgiInfoQueue->SetBreakOnSeverity(DXGI_DEBUG_ALL, DXGI_INFO_QUEUE_MESSAGE_SEVERITY_WARNING, true);
#endif

        GP_CONFIRM(SUCCEEDED(CreateDXGIFactory2(dxgiFactoryFlag, IID_PPV_ARGS(&m_Factory))));
    }

    void Renderer::CreateDevice()
    {
        ComPtr<IDXGIAdapter1> adapter = nullptr;
        GP_CONFIRM(SUCCEEDED(m_Factory->EnumAdapters1(0, adapter.GetAddressOf())));
        DXGI_ADAPTER_DESC1 adapterDesc{};
        GP_CONFIRM(SUCCEEDED(adapter->GetDesc1(&adapterDesc)));
        GP_CONFIRM(SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_Device))));

#ifdef _DEBUG
        ComPtr<ID3D12InfoQueue> infoQueue = nullptr;
        GP_CONFIRM(SUCCEEDED(m_Device->QueryInterface(IID_PPV_ARGS(&infoQueue))));
        GP_CONFIRM(SUCCEEDED(infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true)));
        GP_CONFIRM(SUCCEEDED(infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true)));
        GP_CONFIRM(SUCCEEDED(infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true)));

        D3D12_MESSAGE_ID denyIds[] =
        {
            D3D12_MESSAGE_ID_CLEARDEPTHSTENCILVIEW_MISMATCHINGCLEARVALUE
        };
        D3D12_INFO_QUEUE_FILTER infoQueueFilter = {};
        infoQueueFilter.DenyList.NumIDs = _countof(denyIds);
        infoQueueFilter.DenyList.pIDList = denyIds;
        GP_CONFIRM(SUCCEEDED(infoQueue->AddStorageFilterEntries(&infoQueueFilter)));

#endif
    }

    void Renderer::CreateCommandQueue()
    {
        D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
        commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        commandQueueDesc.NodeMask = 0;
        GP_CONFIRM(SUCCEEDED(m_Device->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_CommandQueue))));
    }

    void Renderer::CreateCommandList()
    {
        GP_CONFIRM(SUCCEEDED(m_Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator))));
        GP_CONFIRM(
            SUCCEEDED(m_Device->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(&m_CommandList))));
    }

    void Renderer::CreateDescriptorHeaps()
    {
        uint32_t maxDescriptorCount[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
        maxDescriptorCount[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = 10;
        maxDescriptorCount[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = 2;
        maxDescriptorCount[D3D12_DESCRIPTOR_HEAP_TYPE_DSV] = 1;
        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        {
            if (maxDescriptorCount[i] == 0)
            {
                continue;
            }

            D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
            descriptorHeapDesc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);
            descriptorHeapDesc.NumDescriptors = maxDescriptorCount[i];
            descriptorHeapDesc.Flags = descriptorHeapDesc.Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
                                           ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE
                                           : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            descriptorHeapDesc.NodeMask = 0;
            GP_CONFIRM(SUCCEEDED(m_Device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_DescriptorHeaps[i]))));
        }
    }

    void Renderer::CreateSwapChain(HWND windowHandle)
    {
        const uint32_t swapChainBufferCount = 2;
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = m_Width;
        swapChainDesc.Height = m_Height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
        swapChainDesc.BufferCount = swapChainBufferCount;
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags = 0;
        ComPtr<IDXGISwapChain1> swapChain = nullptr;
        GP_CONFIRM(SUCCEEDED(m_Factory->CreateSwapChainForHwnd(m_CommandQueue.Get(), windowHandle, &swapChainDesc, nullptr, nullptr, &swapChain)));
        GP_CONFIRM(SUCCEEDED(swapChain.As(&m_SwapChain)));
        GP_CONFIRM(SUCCEEDED(m_Factory->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER)));


        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        {
            m_DescriptorHeapIncrementSizes[i] = m_Device->GetDescriptorHandleIncrementSize(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
        }

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(), 0);
        m_SwapChainBuffers.resize(swapChainBufferCount);
        for (uint32_t i = 0; i < swapChainBufferCount; ++i)
        {
            GP_CONFIRM(SUCCEEDED(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_SwapChainBuffers[i]))));
            m_Device->CreateRenderTargetView(m_SwapChainBuffers[i].Get(), nullptr, rtvHeapHandle);
            rtvHeapHandle.Offset(m_DescriptorHeapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
        }
    }

    void Renderer::CreateRenderTarget()
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(), 0);
        for (const auto buffer : m_SwapChainBuffers)
        {
            m_Device->CreateRenderTargetView(buffer.Get(), nullptr, rtvHeapHandle);
            rtvHeapHandle.Offset(m_DescriptorHeapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
        }

        const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_Width, m_Height);
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        GP_CONFIRM(SUCCEEDED(m_Device->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr, IID_PPV_ARGS(&m_DepthStencilBuffer))));

        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart(), 0);
        m_Device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), nullptr, dsvHeapHandle);

        m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));
        m_ScissorRect = CD3DX12_RECT(0, 0, m_Width, m_Height);
    }

    void Renderer::CreateFence()
    {
        GP_CONFIRM(SUCCEEDED(m_Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence))));
        m_FenceEvent.Attach(CreateEvent(nullptr, false, false, nullptr));
        GP_CONFIRM(m_FenceEvent.IsValid());
    }

    void Renderer::WaitForGPU()
    {
        GP_CONFIRM(SUCCEEDED(m_CommandQueue->Signal(m_Fence.Get(), m_FenceValue)));
        if (m_Fence->GetCompletedValue() < m_FenceValue)
        {
            GP_CONFIRM(SUCCEEDED(m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent.Get())));
            WaitForSingleObject(m_FenceEvent.Get(), INFINITE);
        }
        m_FenceValue++;
    }

    void Renderer::CreateVertexAndIndexBuffer()
    {
        const CD3DX12_HEAP_PROPERTIES heapProperty(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_HEAP_PROPERTIES uploadHeapProperty(D3D12_HEAP_TYPE_UPLOAD);
        const D3D12_RESOURCE_DESC vertexBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_Vertices.size() * sizeof(Vertex));

        GP_CONFIRM(SUCCEEDED(
            m_Device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                IID_PPV_ARGS(&m_VertexBuffer
                ))));
        ComPtr<ID3D12Resource> vertexUploadBuffer = nullptr;
        GP_CONFIRM(SUCCEEDED(
            m_Device->CreateCommittedResource(&uploadHeapProperty, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc, D3D12_RESOURCE_STATE_COMMON,
                nullptr, IID_PPV_ARGS(&
                    vertexUploadBuffer))));
        {
            uint8_t* mapped = nullptr;
            GP_CONFIRM(SUCCEEDED(vertexUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mapped))));
            std::memcpy(mapped, m_Vertices.data(), m_Vertices.size() * sizeof(Vertex));
            vertexUploadBuffer->Unmap(0, nullptr);
        }


        const D3D12_RESOURCE_DESC indexBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_Indices.size() * sizeof(uint32_t));
        GP_CONFIRM(SUCCEEDED(
            m_Device->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &indexBufferResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr,
                IID_PPV_ARGS(&m_IndexBuffer)
            )));
        ComPtr<ID3D12Resource> indexUploadBuffer = nullptr;
        GP_CONFIRM(SUCCEEDED(
            m_Device->CreateCommittedResource(&uploadHeapProperty, D3D12_HEAP_FLAG_NONE, &indexBufferResourceDesc, D3D12_RESOURCE_STATE_COMMON,
                nullptr, IID_PPV_ARGS(&
                    indexUploadBuffer))));
        {
            uint8_t* mapped = nullptr;
            GP_CONFIRM(SUCCEEDED(indexUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mapped))));
            std::memcpy(mapped, m_Indices.data(), m_Indices.size() * sizeof(uint32_t));
            indexUploadBuffer->Unmap(0, nullptr);
        }

        m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);
        m_CommandList->CopyResource(m_VertexBuffer.Get(), vertexUploadBuffer.Get());
        m_CommandList->CopyResource(m_IndexBuffer.Get(), indexUploadBuffer.Get());
        m_CommandList->Close();
        m_CommandQueue->ExecuteCommandLists(1, CommandListCast(m_CommandList.GetAddressOf()));

        WaitForGPU();


        m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        m_VertexBufferView.SizeInBytes = static_cast<uint32_t>(m_Vertices.size() * sizeof(Vertex));
        m_VertexBufferView.StrideInBytes = sizeof(Vertex);

        m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
        m_IndexBufferView.SizeInBytes = static_cast<uint32_t>(m_Indices.size() * sizeof(uint32_t));
        m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    }

    void Renderer::CreateTextureShaderResourceView(entt::resource<Texture> textureHandle)
    {
        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = textureHandle->Data;
        subresourceData.RowPitch = textureHandle->Width * textureHandle->channelCount;
        subresourceData.SlicePitch = textureHandle->Height * textureHandle->channelCount;

        const auto textureDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8G8B8A8_UNORM, 512, 512);
        const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
        const auto uploadHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);

        GP_CONFIRM(SUCCEEDED(m_Device->CreateCommittedResource(
            &heapProperties,
            D3D12_HEAP_FLAG_NONE,
            &textureDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            IID_PPV_ARGS(&m_Texture))));

        const auto uploadBufferSize = GetRequiredIntermediateSize(m_Texture.Get(), 0, 1);

        CD3DX12_RESOURCE_DESC bufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);
        ComPtr<ID3D12Resource> textureUploadHeap;
        GP_CONFIRM(SUCCEEDED(m_Device->CreateCommittedResource(
            &uploadHeapProperties,
            D3D12_HEAP_FLAG_NONE,
            &bufferResourceDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            IID_PPV_ARGS(&textureUploadHeap))));

        m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);
        UpdateSubresources(m_CommandList.Get(), m_Texture.Get(), textureUploadHeap.Get(), 0, 0, 1, &subresourceData);
        CD3DX12_RESOURCE_BARRIER barrier{};
        barrier = CD3DX12_RESOURCE_BARRIER::Transition(m_Texture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        m_CommandList->ResourceBarrier(1, &barrier);
        m_CommandList->Close();
        m_CommandQueue->ExecuteCommandLists(1, CommandListCast(m_CommandList.GetAddressOf()));
        WaitForGPU();

        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = textureDesc.Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;

        m_Device->CreateShaderResourceView(m_Texture.Get(), &srvDesc,
                                           m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart());
    }

    void Renderer::CreatePipelineState()
    {
        uint32_t compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        ComPtr<ID3D10Blob> vsBlob = nullptr;
        ComPtr<ID3D10Blob> vsErrorBlob = nullptr;

        FAILED(
            D3DCompileFromFile(L"Engine/Content/Shader/DefaultShader_VS.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
                "vs_5_1", compileFlags, 0, vsBlob.GetAddressOf(), vsErrorBlob.GetAddressOf()));
        if (vsErrorBlob->GetBufferSize() > 0)
        {
            OutputDebugStringA((LPCSTR)vsErrorBlob->GetBufferPointer());
        }


        ComPtr<ID3D10Blob> psBlob = nullptr;
        ComPtr<ID3D10Blob> psErrorBlob = nullptr;
        FAILED(
            D3DCompileFromFile(L"Engine/Content/Shader/DefaultShader_PS.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
                "ps_5_1", compileFlags, 0, psBlob.GetAddressOf(), psErrorBlob.GetAddressOf()));
        if (psErrorBlob->GetBufferSize() > 0)
        {
            // TODO: SPDLOG
            OutputDebugStringA((LPCSTR)psErrorBlob->GetBufferPointer());
        };


        struct PipelineStateStream
        {
            CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE RootSignature;
            CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT InputLayout;
            CD3DX12_PIPELINE_STATE_STREAM_VS VS;
            CD3DX12_PIPELINE_STATE_STREAM_PS PS;
            CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY PrimitiveTopology;
            CD3DX12_PIPELINE_STATE_STREAM_RASTERIZER Rasterizer;
            CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS RenderTargetFormats;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL DepthStencil;
            CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT DepthStencilFormat;
        } pipelinStateSteam;


        ComPtr<ID3DBlob> rootSignatureBlob = nullptr;
        ComPtr<ID3DBlob> errorBlob = nullptr;
        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        D3D12_ROOT_PARAMETER rootParameters[3];

        // MVP Matrix
        rootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
        rootParameters[0].Constants.ShaderRegister = 0;
        rootParameters[0].Constants.RegisterSpace = 0;
        rootParameters[0].Constants.Num32BitValues = 48;

        // Light Color
        rootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
        rootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[1].Constants.ShaderRegister = 1;
        rootParameters[1].Constants.RegisterSpace = 0;
        rootParameters[1].Constants.Num32BitValues = 12;

        rootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        rootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
        rootParameters[2].DescriptorTable.NumDescriptorRanges = 1;
        CD3DX12_DESCRIPTOR_RANGE descriptorRange{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0};
        rootParameters[2].DescriptorTable.pDescriptorRanges = &descriptorRange;

        CD3DX12_STATIC_SAMPLER_DESC samplerDescs[1];
        samplerDescs[0] = {0, D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT};

        rootSignatureDesc.Init(_countof(rootParameters), rootParameters, _countof(samplerDescs), samplerDescs,
                               D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        if (FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSignatureBlob, &errorBlob)))
        {
            //spdlog::debug("{}", std::string(static_cast<char*>(errorBlob->GetBufferPointer())));
            GP_CONFIRM(false);
        }
        GP_CONFIRM(
            SUCCEEDED(m_Device->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->GetBufferSize(), IID_PPV_ARGS(&
                m_RootSignature))));


        std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
        };


        pipelinStateSteam.RootSignature = m_RootSignature.Get();
        pipelinStateSteam.InputLayout = {InputElements.data(), static_cast<uint32_t>(InputElements.size())};
        pipelinStateSteam.VS = {vsBlob->GetBufferPointer(), vsBlob->GetBufferSize()};
        pipelinStateSteam.PS = {psBlob->GetBufferPointer(), psBlob->GetBufferSize()};
        pipelinStateSteam.PrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        D3D12_RT_FORMAT_ARRAY renderTargetFormats{};
        renderTargetFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        renderTargetFormats.NumRenderTargets = 1;
        pipelinStateSteam.RenderTargetFormats = renderTargetFormats;
        CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FrontCounterClockwise = false;
        pipelinStateSteam.Rasterizer = rasterizerDesc;
        pipelinStateSteam.DepthStencil = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        pipelinStateSteam.DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;


        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
            sizeof(pipelinStateSteam), &pipelinStateSteam
        };

        GP_CONFIRM(SUCCEEDED(m_Device->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState))));
    }

    void Renderer::LoadAssets()
    {
        using namespace entt::literals;
        const auto meshHandle = ResourceManager::LoadMesh("teapot"_hs, "Content/teapot.obj");
        m_Vertices = meshHandle->Vertices;
        m_Indices = meshHandle->Indices;

        const auto textureHandle = Engine::ResourceManager::LoadTexture("yoimiya_texture"_hs, "Content/sticker_6.png");
        CreateTextureShaderResourceView(textureHandle);
    }
}
