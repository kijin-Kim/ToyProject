#include "EnginePCH.h"
#include "Renderer.h"

#include "AssetManager.h"
#include "d3dx12.h"
#include "Engine.h"
#include "GraphicsHelper.h"
#include "Mesh.h"
#include "Core/Core.h"
#include "ECS/Components.h"


namespace Engine
{
    struct TransformComponent;

    void Renderer::Initialize(HWND windowHandle, uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;
        InitDirectX(windowHandle);
        Prepare();
    }

    void Renderer::InitDirectX(HWND windowHandle)
    {
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
        CreateSceneTextures();
        CreateVertexAndIndexBuffer();


        {
            const auto lightEntity = m_Registry.create();
            auto& displayName = m_Registry.emplace<DisplayNameComponent>(lightEntity);
            auto& transform = m_Registry.emplace<TransformComponent>(lightEntity);
            auto& light = m_Registry.emplace<LightComponent>(lightEntity);

            displayName.Name = "Light";
            transform.Position = DirectX::SimpleMath::Vector3{0.0f, 0.0f, -100.0f};
            light.LightColor = DirectX::Colors::LightGoldenrodYellow;
        }

        {
            const auto StaticMeshEntity = m_Registry.create();
            auto& displayName = m_Registry.emplace<DisplayNameComponent>(StaticMeshEntity);
            displayName.Name = "StaticMesh";
        }
    }

    void Renderer::Render()
    {
        const uint32_t currentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();


        m_CommandAllocator->Reset();
        m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);
        m_CommandList->RSSetViewports(1, &m_Viewport);
        m_CommandList->RSSetScissorRects(1, &m_ScissorRect);


        m_CommandList->SetPipelineState(m_PipelineState.Get());
        m_CommandList->SetGraphicsRootSignature(m_RootSignature.Get());

        static float angle = 0.0f;
        angle += 0.05f;

        DirectX::SimpleMath::Matrix model = DirectX::SimpleMath::Matrix::CreateScale(1.0f)
            * DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(DirectX::XMConvertToRadians(angle), 0.0f, 0.0f)
            * DirectX::SimpleMath::Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);


        constexpr DirectX::SimpleMath::Vector3 eye = {0.0f, 0.0f, -40.0f};
        constexpr DirectX::SimpleMath::Vector3 target = {0.0f, 0.0f, 0.0f};
        constexpr DirectX::SimpleMath::Vector3 up = {0.0f, 1.0f, 0.0f};
        const DirectX::SimpleMath::Matrix view = XMMatrixLookAtLH(eye, target, up);

        constexpr float fieldOfView = DirectX::XMConvertToRadians(90.0f);
        const float aspectRatio = m_Width / static_cast<float>(m_Height);
        constexpr float nearPlane = 0.1f;
        constexpr float farPlane = 1000.0f;
        const DirectX::SimpleMath::Matrix projection = DirectX::XMMatrixPerspectiveFovLH(fieldOfView, aspectRatio, nearPlane, farPlane);
        DirectX::SimpleMath::Matrix viewProjection = view * projection;

        struct TransformData
        {
            DirectX::SimpleMath::Matrix MatGeo;
            DirectX::SimpleMath::Matrix MatGeoInvert;
            DirectX::SimpleMath::Matrix MatVP;
        } transformData;

        transformData.MatGeo = model.Transpose();
        transformData.MatGeoInvert = model.Invert().Transpose();
        transformData.MatVP = viewProjection.Transpose();

        m_CommandList->SetGraphicsRoot32BitConstants(0, 48, &transformData, 0);

        struct LightData
        {
            DirectX::SimpleMath::Vector4 LightPosition;
            DirectX::SimpleMath::Color LightColor;
            DirectX::SimpleMath::Vector4 CameraPosition;
        } lightData;

        auto lightSystem = m_Registry.view<TransformComponent, LightComponent>();
        for (const auto [entity, transform, light] : lightSystem.each())
        {
            lightData.LightPosition = DirectX::SimpleMath::Vector4{transform.Position.x, transform.Position.y, transform.Position.z, 1.0f};
            lightData.LightColor = light.LightColor;
        }
        lightData.CameraPosition = DirectX::SimpleMath::Vector4(eye.x, eye.y, eye.z, 1.0f);


        m_CommandList->SetGraphicsRoot32BitConstants(1, 12, &lightData, 0);
        m_CommandList->SetDescriptorHeaps(1, m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetAddressOf());
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetGPUDescriptorHandleForHeapStart(), 3,
                                                m_DescriptorHeapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
        m_CommandList->SetGraphicsRootDescriptorTable(2, gpuHandle);

        m_CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_CommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
        m_CommandList->IASetIndexBuffer(&m_IndexBufferView);

        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart(), 0);
        m_CommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(),
                                                    currentBackBufferIndex + 2, m_DescriptorHeapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
            m_CommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);
            {
                const auto& resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_SceneColorBuffers[currentBackBufferIndex].Get(),
                                                                                   D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
                m_CommandList->ResourceBarrier(1, &resourceBarrier);
            }

            m_CommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::DarkSlateGray, 0, nullptr);
        }
        m_CommandList->DrawIndexedInstanced(static_cast<uint32_t>(m_Indices.size()), 1, 0, 0, 0);
        {
            const auto& resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_SceneColorBuffers[currentBackBufferIndex].Get(),
                                                                               D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            m_CommandList->ResourceBarrier(1, &resourceBarrier);
        }


        m_CommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(),
                                                    m_DescriptorHeapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] * currentBackBufferIndex);
            m_CommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);
            {
                const auto& resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_SwapChainBuffers[currentBackBufferIndex].Get(),
                                                                                   D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
                m_CommandList->ResourceBarrier(1, &resourceBarrier);
            }

            m_CommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::DarkSlateGray, 0, nullptr);
        }
        for (const auto& renderCommand : m_RenderCommands)
        {
            renderCommand(m_CommandList.Get());
        }
        m_RenderCommands.clear();


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


    void Renderer::CreateCommandQueue()
    {
        {
            D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
            commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
            commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            commandQueueDesc.NodeMask = 0;
            EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_CommandQueue))));
        }


        {
            D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
            commandQueueDesc.Type = D3D12_COMMAND_LIST_TYPE_COPY;
            commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
            commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
            commandQueueDesc.NodeMask = 0;
            EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&m_CopyCommandQueue))));
        }
    }

    void Renderer::CreateCommandList()
    {
        EG_CONFIRM(
            SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocator))
            ));
        EG_CONFIRM(
            SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_DIRECT, D3D12_COMMAND_LIST_FLAG_NONE,
                IID_PPV_ARGS(&m_CommandList))));

        EG_CONFIRM(
            SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_CopyCommandAllocator)
                )
            ));
        EG_CONFIRM(
            SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommandList1(0, D3D12_COMMAND_LIST_TYPE_COPY, D3D12_COMMAND_LIST_FLAG_NONE,
                IID_PPV_ARGS(&m_CopyCommandList))));
    }

    void Renderer::CreateDescriptorHeaps()
    {
        uint32_t maxDescriptorCount[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};
        maxDescriptorCount[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV] = 10;
        maxDescriptorCount[D3D12_DESCRIPTOR_HEAP_TYPE_RTV] = 4;
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
            EG_CONFIRM(
                SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&m_DescriptorHeaps[i]))));
        }
    }

    void Renderer::CreateSwapChain(HWND windowHandle)
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = m_Width;
        swapChainDesc.Height = m_Height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.Stereo = false;
        swapChainDesc.SampleDesc.Count = 1;
        swapChainDesc.SampleDesc.Quality = 0;
        swapChainDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
        swapChainDesc.BufferCount = m_FrameCount;
        swapChainDesc.Scaling = DXGI_SCALING_NONE;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
        swapChainDesc.Flags = 0;
        Microsoft::WRL::ComPtr<IDXGISwapChain1> swapChain = nullptr;
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetFactory()->CreateSwapChainForHwnd(m_CommandQueue.Get(), windowHandle, &swapChainDesc, nullptr, nullptr, &swapChain)));
        EG_CONFIRM(SUCCEEDED(swapChain.As(&m_SwapChain)));
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetFactory()->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER)));


        for (int i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        {
            m_DescriptorHeapIncrementSizes[i] = Core::GetRenderContext().GetDevice()->GetDescriptorHandleIncrementSize(
                static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
        }
    }

    void Renderer::CreateRenderTarget()
    {
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(), 0);
        m_SwapChainBuffers.resize(m_FrameCount);
        for (uint32_t i = 0; i < m_FrameCount; ++i)
        {
            EG_CONFIRM(SUCCEEDED(m_SwapChain->GetBuffer(i, IID_PPV_ARGS(&m_SwapChainBuffers[i]))));
        }

        for (const auto& buffer : m_SwapChainBuffers)
        {
            Core::GetRenderContext().GetDevice()->CreateRenderTargetView(buffer.Get(), nullptr, rtvHeapHandle);
            rtvHeapHandle.Offset(m_DescriptorHeapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
        }

        const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_Width, m_Height);
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr, IID_PPV_ARGS(&m_DepthStencilBuffer))));

        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart(), 0);
        Core::GetRenderContext().GetDevice()->CreateDepthStencilView(m_DepthStencilBuffer.Get(), nullptr, dsvHeapHandle);

        m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));
        m_ScissorRect = CD3DX12_RECT(0, 0, static_cast<int32_t>(m_Width), static_cast<int32_t>(m_Height));
    }

    void Renderer::CreateSceneTextures()
    {
        m_SceneColorBuffers.resize(m_FrameCount);

        for (int i = 0; i < m_SwapChainBuffers.size(); ++i)
        {
            auto textureDesc = m_SwapChainBuffers[i]->GetDesc();
            const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

            D3D12_CLEAR_VALUE clearValue;
            clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommittedResource(
                &heapProperties,
                D3D12_HEAP_FLAG_NONE,
                &textureDesc,
                D3D12_RESOURCE_STATE_COMMON,
                &clearValue,
                IID_PPV_ARGS(&m_SceneColorBuffers[i]))));
            std::wstring name = L"SceneColor_" + std::to_wstring(i);
            m_SceneColorBuffers[i]->SetName(name.c_str());


            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(),
                                                        i + 2, m_DescriptorHeapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
            Core::GetRenderContext().GetDevice()->CreateRenderTargetView(m_SceneColorBuffers[i].Get(), nullptr, rtvHeapHandle);

            CD3DX12_CPU_DESCRIPTOR_HANDLE srvHeapHandle(
                m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart(), i + 1,
                m_DescriptorHeapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
            Core::GetRenderContext().GetDevice()->CreateShaderResourceView(m_SceneColorBuffers[i].Get(), nullptr, srvHeapHandle);
        }
    }

    void Renderer::CreateFence()
    {
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence))));
        m_FenceEvent.Attach(CreateEvent(nullptr, false, false, nullptr));
        EG_CONFIRM(m_FenceEvent.IsValid());
    }

    void Renderer::WaitForGPU()
    {
        EG_CONFIRM(SUCCEEDED(m_CommandQueue->Signal(m_Fence.Get(), m_FenceValue)));
        if (m_Fence->GetCompletedValue() < m_FenceValue)
        {
            EG_CONFIRM(SUCCEEDED(m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent.Get())));
            WaitForSingleObject(m_FenceEvent.Get(), INFINITE);
        }
        m_FenceValue++;
    }

    void Renderer::CreateVertexAndIndexBuffer()
    {
        const CD3DX12_HEAP_PROPERTIES heapProperty(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_HEAP_PROPERTIES uploadHeapProperty(D3D12_HEAP_TYPE_UPLOAD);
        const D3D12_RESOURCE_DESC vertexBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_Vertices.size() * sizeof(Vertex));

        EG_CONFIRM(SUCCEEDED(
            Core::GetRenderContext().GetDevice()->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc,
                D3D12_RESOURCE_STATE_COMMON, nullptr,
                IID_PPV_ARGS(&m_VertexBuffer
                ))));
        m_VertexBuffer->SetName(L"VertexBuffer");


        Microsoft::WRL::ComPtr<ID3D12Resource> vertexUploadBuffer = nullptr;
        EG_CONFIRM(SUCCEEDED(
            Core::GetRenderContext().GetDevice()->CreateCommittedResource(&uploadHeapProperty, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr, IID_PPV_ARGS(&
                    vertexUploadBuffer))));
        {
            uint8_t* mapped = nullptr;
            EG_CONFIRM(SUCCEEDED(vertexUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mapped))));
            std::memcpy(mapped, m_Vertices.data(), m_Vertices.size() * sizeof(Vertex));
            vertexUploadBuffer->Unmap(0, nullptr);
        }


        const D3D12_RESOURCE_DESC indexBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_Indices.size() * sizeof(uint32_t));
        EG_CONFIRM(SUCCEEDED(
            Core::GetRenderContext().GetDevice()->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &indexBufferResourceDesc,
                D3D12_RESOURCE_STATE_COMMON, nullptr,
                IID_PPV_ARGS(&m_IndexBuffer)
            )));
        m_IndexBuffer->SetName(L"IndexBuffer");
        Microsoft::WRL::ComPtr<ID3D12Resource> indexUploadBuffer = nullptr;
        EG_CONFIRM(SUCCEEDED(
            Core::GetRenderContext().GetDevice()->CreateCommittedResource(&uploadHeapProperty, D3D12_HEAP_FLAG_NONE, &indexBufferResourceDesc,
                D3D12_RESOURCE_STATE_COMMON,
                nullptr, IID_PPV_ARGS(&
                    indexUploadBuffer))));
        {
            uint8_t* mapped = nullptr;
            EG_CONFIRM(SUCCEEDED(indexUploadBuffer->Map(0, nullptr, reinterpret_cast<void**>(&mapped))));
            std::memcpy(mapped, m_Indices.data(), m_Indices.size() * sizeof(uint32_t));
            indexUploadBuffer->Unmap(0, nullptr);
        }

        m_CopyCommandList->Reset(m_CopyCommandAllocator.Get(), nullptr);


        m_CopyCommandList->CopyResource(m_VertexBuffer.Get(), vertexUploadBuffer.Get());
        m_CopyCommandList->CopyResource(m_IndexBuffer.Get(), indexUploadBuffer.Get());
        D3D12_RESOURCE_BARRIER resourceBarriers[] = {
            CD3DX12_RESOURCE_BARRIER::Transition(m_VertexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST,
                                                 D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER),
            CD3DX12_RESOURCE_BARRIER::Transition(m_IndexBuffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_INDEX_BUFFER),
        };


        //m_CopyCommandList->ResourceBarrier(_countof(resourceBarriers), resourceBarriers);
        m_CopyCommandList->Close();
        m_CopyCommandQueue->ExecuteCommandLists(1, CommandListCast(m_CopyCommandList.GetAddressOf()));
        GraphicsHelper::WaitForGPU(m_CopyCommandQueue, m_Fence, m_FenceValue);


        m_VertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        m_VertexBufferView.SizeInBytes = static_cast<uint32_t>(m_Vertices.size() * sizeof(Vertex));
        m_VertexBufferView.StrideInBytes = sizeof(Vertex);

        m_IndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
        m_IndexBufferView.SizeInBytes = static_cast<uint32_t>(m_Indices.size() * sizeof(uint32_t));
        m_IndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    }

    void Renderer::CreatePipelineState()
    {
        uint32_t compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
        Microsoft::WRL::ComPtr<ID3D10Blob> vsBlob = nullptr;
        Microsoft::WRL::ComPtr<ID3D10Blob> vsErrorBlob = nullptr;

        FAILED(
            D3DCompileFromFile(L"Engine/Content/Shader/DefaultShader_VS.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
                "vs_5_1", compileFlags, 0, vsBlob.GetAddressOf(), vsErrorBlob.GetAddressOf()));
        if (vsErrorBlob->GetBufferSize() > 0)
        {
            const std::string_view msg{static_cast<char*>(vsErrorBlob->GetBufferPointer()), vsErrorBlob->GetBufferSize()};
            spdlog::warn(msg);
        }


        Microsoft::WRL::ComPtr<ID3D10Blob> psBlob = nullptr;
        Microsoft::WRL::ComPtr<ID3D10Blob> psErrorBlob = nullptr;
        FAILED(
            D3DCompileFromFile(L"Engine/Content/Shader/DefaultShader_PS.hlsl", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main",
                "ps_5_1", compileFlags, 0, psBlob.GetAddressOf(), psErrorBlob.GetAddressOf()));
        if (psErrorBlob->GetBufferSize() > 0)
        {
            const std::string_view msg{static_cast<char*>(psErrorBlob->GetBufferPointer()), psErrorBlob->GetBufferSize()};
            spdlog::warn(msg);
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


        Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;
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
            const std::string_view msg{static_cast<char*>(vsErrorBlob->GetBufferPointer()), vsErrorBlob->GetBufferSize()};
            spdlog::error(msg);
            EG_CONFIRM(false);
        }
        EG_CONFIRM(
            SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob->
                GetBufferSize(), IID_PPV_ARGS(&
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

        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState))));
    }

    void Renderer::LoadAssets()
    {
        using namespace entt::literals;
        const auto meshHandle = AssetManager::LoadMesh("teapot"_hs, "Content/teapot.obj");
        m_Vertices = meshHandle->Vertices;
        m_Indices = meshHandle->Indices;

        const auto textureHandle = Engine::AssetManager::LoadTexture("yoimiya_texture"_hs, "Content/sticker_6.png");
        m_CommandList->Reset(m_CommandAllocator.Get(), nullptr);
        GraphicsHelper::CreateTextureResource(textureHandle, m_CommandQueue, m_Fence, m_FenceValue, m_CommandList, m_Texture);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = m_Texture->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHeapHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart(),
                                                    3, m_DescriptorHeapIncrementSizes[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
        Core::GetRenderContext().GetDevice()->CreateShaderResourceView(m_Texture.Get(), &srvDesc, srvHeapHandle);
    }

    void Renderer::SubmitGraphicsCommand(const RenderCommand& renderCommand)
    {
        m_RenderCommands.push_back(renderCommand);
    }
}
