#include "EnginePCH.h"
#include "Renderer.h"

#include "AssetManager.h"
#include "d3dx12.h"
#include "Engine.h"
#include "GraphicsHelper.h"
#include "Mesh.h"
#include "Shader.h"
#include "Core/Core.h"
#include "ECS/Components.h"
#include "ECS/Entity.h"


namespace Engine
{
    struct TransformComponent;

    void Renderer::Initialize(HWND windowHandle, uint32_t width, uint32_t height)
    {
        m_Width = width;
        m_Height = height;
        m_AspectRatio = static_cast<float>(m_Width) / static_cast<float>(m_Height);
        m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Width), static_cast<float>(m_Height));
        m_ScissorRect = CD3DX12_RECT(0, 0, static_cast<int32_t>(m_Width), static_cast<int32_t>(m_Height));
        m_FieldOfView = DirectX::XMConvertToRadians(60.0f);
        InitDirectX(windowHandle);
        Prepare();
    }

    void Renderer::InitDirectX(HWND windowHandle)
    {
        CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT, m_DirectCommandQueue);
        CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, m_DirectCommandAllocator);
        CreateCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT, m_DirectCommandList);

        CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY, m_CopyCommandQueue);
        CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, m_CopyCommandAllocator);
        CreateCommandList(D3D12_COMMAND_LIST_TYPE_COPY, m_CopyCommandList);

        CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 10, m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]);
        CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, 1, m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER]);
        CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 4, m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
        CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE, 1, m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]);

        CreateSwapChain(m_Width, m_Height, windowHandle);
        CreateRenderTarget(m_Width, m_Height);
        CreateSceneTextures(m_Width, m_Height);
        CreateFence();
    }


    void Renderer::Prepare()
    {
        CD3DX12_ROOT_PARAMETER rootParameters[3];
        // MVP Matrix
        rootParameters[0].InitAsConstants(48, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
        // Light
        rootParameters[1].InitAsConstants(12, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
        // Texture
        CD3DX12_DESCRIPTOR_RANGE descriptorRange{D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0};
        rootParameters[2].InitAsDescriptorTable(1, &descriptorRange, D3D12_SHADER_VISIBILITY_PIXEL);
        CreateRootSignature(_countof(rootParameters), rootParameters, m_RootSignature);


        CreatePipelineState();
        LoadAssets();

        {
            m_CameraEntity = m_Registry.create();
            auto& displayName = m_Registry.emplace<DisplayNameComponent>(m_CameraEntity);
            auto& transform = m_Registry.emplace<TransformComponent>(m_CameraEntity);
            displayName.Name = "SceneCamera";
        }


        {
            const auto lightEntity = m_Registry.create();
            auto& displayName = m_Registry.emplace<DisplayNameComponent>(lightEntity);
            auto& transform = m_Registry.emplace<TransformComponent>(lightEntity);
            auto& light = m_Registry.emplace<LightComponent>(lightEntity);

            displayName.Name = "Light";
            transform.Rotation = DirectX::SimpleMath::Vector3{0.0f, 0.0f, 0.0f};
            light.LightColor = DirectX::Colors::LightGoldenrodYellow;
        }

        {
            const auto meshEntity = m_Registry.create();
            auto& displayName = m_Registry.emplace<DisplayNameComponent>(meshEntity);
            auto& meshRenderComponent = m_Registry.emplace<MeshRenderComponent>(meshEntity);
            displayName.Name = "Mesh";

            using namespace entt::literals;
            meshRenderComponent.Mesh = AssetManager::LoadMesh("teapot"_hs, "Content/teapot.obj");
        }
    }

    void Renderer::Render()
    {
        const uint32_t currentBackBufferIndex = m_SwapChain->GetCurrentBackBufferIndex();


        m_DirectCommandAllocator->Reset();
        m_DirectCommandList->Reset(m_DirectCommandAllocator.Get(), nullptr);
        m_DirectCommandList->RSSetViewports(1, &m_Viewport);
        m_DirectCommandList->RSSetScissorRects(1, &m_ScissorRect);


        m_DirectCommandList->SetPipelineState(m_PipelineState.Get());
        m_DirectCommandList->SetGraphicsRootSignature(m_RootSignature.Get());

        static float angle = 0.0f;
        angle += 0.05f;


        DirectX::SimpleMath::Matrix modelScale = DirectX::SimpleMath::Matrix::CreateScale(1.0f);
        DirectX::SimpleMath::Matrix modelRotation = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(DirectX::XMConvertToRadians(angle), 0.0f, 0.0f);
        DirectX::SimpleMath::Matrix modelTranslation = DirectX::SimpleMath::Matrix::CreateTranslation(0.0f, 0.0f, 0.0f);
        DirectX::SimpleMath::Matrix model = modelScale * modelRotation * modelTranslation;


        const auto& cameraTransform = m_Registry.get<TransformComponent>(m_CameraEntity);
        DirectX::SimpleMath::Matrix cameraScale = DirectX::SimpleMath::Matrix::CreateScale(1.0f);
        DirectX::SimpleMath::Matrix cameraRotation = DirectX::SimpleMath::Matrix::CreateFromYawPitchRoll(DirectX::XMConvertToRadians(cameraTransform.Rotation.y), DirectX::XMConvertToRadians(cameraTransform.Rotation.x), DirectX::XMConvertToRadians(cameraTransform.Rotation.z));
        DirectX::SimpleMath::Matrix cameraTranslation = DirectX::SimpleMath::Matrix::CreateTranslation(cameraTransform.Position);
        m_CameraTransform = cameraScale * cameraRotation * cameraTranslation;


        constexpr float nearPlane = 0.1f;
        constexpr float farPlane = 1000.0f;
        const DirectX::SimpleMath::Matrix projection = DirectX::SimpleMath::Matrix::CreatePerspectiveFieldOfView(m_FieldOfView, m_AspectRatio, nearPlane, farPlane);
        DirectX::SimpleMath::Matrix viewProjection = m_CameraTransform.Invert() * projection;


        struct TransformData
        {
            DirectX::SimpleMath::Matrix MatGeo;
            DirectX::SimpleMath::Matrix MatGeoInvert;
            DirectX::SimpleMath::Matrix MatVP;
        } transformData;

        transformData.MatGeo = model.Transpose();
        transformData.MatGeoInvert = model.Invert().Transpose();
        transformData.MatVP = viewProjection.Transpose();

        m_DirectCommandList->SetGraphicsRoot32BitConstants(0, 48, &transformData, 0);

        struct LightData
        {
            DirectX::SimpleMath::Vector4 LightDirection;
            DirectX::SimpleMath::Color LightColor;
            DirectX::SimpleMath::Vector4 CameraPosition;
        } lightData;

        auto lightSystem = m_Registry.view<TransformComponent, LightComponent>();
        for (const auto [entity, transform, light] : lightSystem.each())
        {
            DirectX::SimpleMath::Quaternion q = DirectX::SimpleMath::Quaternion::CreateFromYawPitchRoll(DirectX::XMConvertToRadians(transform.Rotation.y), DirectX::XMConvertToRadians(transform.Rotation.x), DirectX::XMConvertToRadians(transform.Rotation.z));
            DirectX::SimpleMath::Vector3 direction = DirectX::SimpleMath::Vector3::Transform(DirectX::SimpleMath::Vector3::UnitZ, q);
            lightData.LightDirection = DirectX::SimpleMath::Vector4{direction.x, direction.y, direction.z, 1.0f};
            lightData.LightColor = light.LightColor;
        }
        lightData.CameraPosition = DirectX::SimpleMath::Vector4(cameraTransform.Position.x, cameraTransform.Position.y, cameraTransform.Position.z, 1.0f);


        m_DirectCommandList->SetGraphicsRoot32BitConstants(1, 12, &lightData, 0);
        m_DirectCommandList->SetDescriptorHeaps(1, m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].GetAddressOf());
        CD3DX12_GPU_DESCRIPTOR_HANDLE gpuHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetGPUDescriptorHandleForHeapStart(), 3, Core::GetRenderContext().GetIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
        m_DirectCommandList->SetGraphicsRootDescriptorTable(2, gpuHandle);

        m_DirectCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        m_DirectCommandList->IASetVertexBuffers(0, 1, &m_VertexBufferView);
        m_DirectCommandList->IASetIndexBuffer(&m_IndexBufferView);

        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart(), 0);
        m_DirectCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIndex + 2, Core::GetRenderContext().GetIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
            m_DirectCommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);
            {
                const auto& resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_SceneColorBuffers[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
                m_DirectCommandList->ResourceBarrier(1, &resourceBarrier);
            }

            m_DirectCommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::DarkSlateGray, 0, nullptr);
        }
        m_DirectCommandList->DrawIndexedInstanced(static_cast<uint32_t>(m_IndexBufferView.SizeInBytes / sizeof(uint32_t)), 1, 0, 0, 0);
        {
            const auto& resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_SceneColorBuffers[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            m_DirectCommandList->ResourceBarrier(1, &resourceBarrier);
        }


        m_DirectCommandList->ClearDepthStencilView(dsvHandle, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
        {
            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(), Core::GetRenderContext().GetIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV) * currentBackBufferIndex);
            m_DirectCommandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);
            {
                const auto& resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_SwapChainBuffers[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
                m_DirectCommandList->ResourceBarrier(1, &resourceBarrier);
            }

            m_DirectCommandList->ClearRenderTargetView(rtvHandle, DirectX::Colors::DarkSlateGray, 0, nullptr);
        }
        for (const auto& renderCommand : m_RenderCommands)
        {
            renderCommand(m_DirectCommandList.Get());
        }
        m_RenderCommands.clear();


        {
            const auto& resourceBarrier = CD3DX12_RESOURCE_BARRIER::Transition(m_SwapChainBuffers[currentBackBufferIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            m_DirectCommandList->ResourceBarrier(1, &resourceBarrier);
        }


        m_DirectCommandList->Close();
        m_DirectCommandQueue->ExecuteCommandLists(1, CommandListCast(m_DirectCommandList.GetAddressOf()));


        m_SwapChain->Present(0, 0);
        WaitForGPU();
    }

    void Renderer::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE commandListType, Microsoft::WRL::ComPtr<ID3D12CommandQueue>& outCommandQueue)
    {
        D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {};
        commandQueueDesc.Type = commandListType;
        commandQueueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
        commandQueueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        commandQueueDesc.NodeMask = 0;
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(outCommandQueue.GetAddressOf()))));
    }

    void Renderer::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE commandListType, Microsoft::WRL::ComPtr<ID3D12CommandAllocator>& outCommandAllocator)
    {
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommandAllocator(commandListType, IID_PPV_ARGS(outCommandAllocator.GetAddressOf()))));
    }

    void Renderer::CreateCommandList(D3D12_COMMAND_LIST_TYPE commandListType, Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList>& outCommandList)
    {
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommandList1(0, commandListType, D3D12_COMMAND_LIST_FLAG_NONE, IID_PPV_ARGS(outCommandList.GetAddressOf()))));
    }

    void Renderer::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE descriptorHeapType, D3D12_DESCRIPTOR_HEAP_FLAGS descriptorHeapFlag, uint32_t maximumDescriptorsCount, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& outDescriptorHeap)
    {
        EG_CONFIRM(maximumDescriptorsCount > 0);
        D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
        descriptorHeapDesc.Type = descriptorHeapType;
        descriptorHeapDesc.NumDescriptors = maximumDescriptorsCount;
        descriptorHeapDesc.Flags = descriptorHeapFlag;
        descriptorHeapDesc.NodeMask = 0;
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(outDescriptorHeap.GetAddressOf()))));
    }

    void Renderer::CreateSwapChain(uint32_t width, uint32_t height, HWND windowHandle)
    {
        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
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
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetFactory()->CreateSwapChainForHwnd(m_DirectCommandQueue.Get(), windowHandle, &swapChainDesc, nullptr, nullptr, &swapChain)));
        EG_CONFIRM(SUCCEEDED(swapChain.As(&m_SwapChain)));
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetFactory()->MakeWindowAssociation(windowHandle, DXGI_MWA_NO_ALT_ENTER)));
    }

    void Renderer::CreateRenderTarget(uint32_t width, uint32_t height)
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
            rtvHeapHandle.Offset(Core::GetRenderContext().GetIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
        }

        const CD3DX12_HEAP_PROPERTIES heapProperties(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC resourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, m_Width, m_Height);
        resourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, nullptr, IID_PPV_ARGS(&m_DepthStencilBuffer))));

        CD3DX12_CPU_DESCRIPTOR_HANDLE dsvHeapHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart(), 0);
        Core::GetRenderContext().GetDevice()->CreateDepthStencilView(m_DepthStencilBuffer.Get(), nullptr, dsvHeapHandle);
    }

    void Renderer::CreateSceneTextures(uint64_t width, uint32_t height)
    {
        m_SceneColorBuffers.resize(m_FrameCount);

        for (int i = 0; i < m_SwapChainBuffers.size(); ++i)
        {
            auto textureDesc = m_SwapChainBuffers[i]->GetDesc();
            textureDesc.Width = width;
            textureDesc.Height = height;
            const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

            D3D12_CLEAR_VALUE clearValue;
            clearValue.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommittedResource(&heapProperties, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, & clearValue, IID_PPV_ARGS(&m_SceneColorBuffers[i]))));
            std::wstring name = L"SceneColor_" + std::to_wstring(i);
            m_SceneColorBuffers[i]->SetName(name.c_str());


            CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(), i + 2, Core::GetRenderContext().GetIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
            Core::GetRenderContext().GetDevice()->CreateRenderTargetView(m_SceneColorBuffers[i].Get(), nullptr, rtvHeapHandle);

            CD3DX12_CPU_DESCRIPTOR_HANDLE srvHeapHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart(), i + 1, Core::GetRenderContext().GetIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
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
        EG_CONFIRM(SUCCEEDED(m_DirectCommandQueue->Signal(m_Fence.Get(), m_FenceValue)));
        if (m_Fence->GetCompletedValue() < m_FenceValue)
        {
            EG_CONFIRM(SUCCEEDED(m_Fence->SetEventOnCompletion(m_FenceValue, m_FenceEvent.Get())));
            WaitForSingleObject(m_FenceEvent.Get(), INFINITE);
        }
        m_FenceValue++;
    }

    void Renderer::CreateVertexAndIndexBufferView(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices, Microsoft::WRL::ComPtr<ID3D12Resource>& outVertexBuffer, Microsoft::WRL::ComPtr<ID3D12Resource>& outIndexBuffer, D3D12_VERTEX_BUFFER_VIEW& outVertexBufferView, D3D12_INDEX_BUFFER_VIEW& outIndexBufferView)
    {
        const CD3DX12_HEAP_PROPERTIES heapProperty(D3D12_HEAP_TYPE_DEFAULT);
        const CD3DX12_HEAP_PROPERTIES uploadHeapProperty(D3D12_HEAP_TYPE_UPLOAD);
        const D3D12_RESOURCE_DESC vertexBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(vertices.size() * sizeof(Vertex));

        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(outVertexBuffer.GetAddressOf()))));
        Microsoft::WRL::ComPtr<ID3D12Resource> vertexIntermediateBuffer = nullptr;
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommittedResource(&uploadHeapProperty, D3D12_HEAP_FLAG_NONE, &vertexBufferResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&vertexIntermediateBuffer))));


        const D3D12_RESOURCE_DESC indexBufferResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(indices.size() * sizeof(uint32_t));
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommittedResource(&heapProperty, D3D12_HEAP_FLAG_NONE, &indexBufferResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(outIndexBuffer.GetAddressOf()))));
        Microsoft::WRL::ComPtr<ID3D12Resource> indexIntermediateBuffer = nullptr;
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateCommittedResource(&uploadHeapProperty, D3D12_HEAP_FLAG_NONE, &indexBufferResourceDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&indexIntermediateBuffer))));

        D3D12_SUBRESOURCE_DATA vertexSubresourceData = {};
        vertexSubresourceData.pData = vertices.data();
        vertexSubresourceData.RowPitch = vertices.size() * sizeof(Vertex);
        vertexSubresourceData.SlicePitch = vertices.size() * sizeof(Vertex);

        D3D12_SUBRESOURCE_DATA indexSubresourceData = {};
        indexSubresourceData.pData = indices.data();
        indexSubresourceData.RowPitch = indices.size() * sizeof(uint32_t);
        indexSubresourceData.SlicePitch = indices.size() * sizeof(uint32_t);


        m_CopyCommandList->Reset(m_CopyCommandAllocator.Get(), nullptr);
        UpdateSubresources(m_CopyCommandList.Get(), m_VertexBuffer.Get(), vertexIntermediateBuffer.Get(), 0, 0, 1, &vertexSubresourceData);
        UpdateSubresources(m_CopyCommandList.Get(), m_IndexBuffer.Get(), indexIntermediateBuffer.Get(), 0, 0, 1, &indexSubresourceData);
        m_CopyCommandList->Close();
        m_CopyCommandQueue->ExecuteCommandLists(1, CommandListCast(m_CopyCommandList.GetAddressOf()));
        GraphicsHelper::WaitForGPU(m_CopyCommandQueue, m_Fence, m_FenceValue);


        outVertexBufferView.BufferLocation = m_VertexBuffer->GetGPUVirtualAddress();
        outVertexBufferView.SizeInBytes = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
        outVertexBufferView.StrideInBytes = sizeof(Vertex);

        outIndexBufferView.BufferLocation = m_IndexBuffer->GetGPUVirtualAddress();
        outIndexBufferView.SizeInBytes = static_cast<uint32_t>(indices.size() * sizeof(uint32_t));
        outIndexBufferView.Format = DXGI_FORMAT_R32_UINT;
    }


    void Renderer::CreateRootSignature(uint32_t rootParameterCount, CD3DX12_ROOT_PARAMETER* rootParameters, Microsoft::WRL::ComPtr<ID3D12RootSignature>& outRootSignature)
    {
        Microsoft::WRL::ComPtr<ID3DBlob> rootSignatureBlob = nullptr;
        Microsoft::WRL::ComPtr<ID3DBlob> errorBlob = nullptr;

        CD3DX12_STATIC_SAMPLER_DESC samplerDescs[1];
        samplerDescs[0] = {0, D3D12_FILTER_MIN_LINEAR_MAG_MIP_POINT};

        CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc{};
        rootSignatureDesc.Init(rootParameterCount, rootParameters, _countof(samplerDescs), samplerDescs, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
        const bool bSerializationFailed = FAILED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1_0, &rootSignatureBlob, &errorBlob));
        if (bSerializationFailed || (errorBlob && errorBlob->GetBufferSize() > 0))
        {
            const std::string_view msg{static_cast<char*>(errorBlob->GetBufferPointer()), errorBlob->GetBufferSize()};
            spdlog::error(msg);
            EG_CONFIRM(!bSerializationFailed);
        }
        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreateRootSignature(0, rootSignatureBlob->GetBufferPointer(), rootSignatureBlob-> GetBufferSize(), IID_PPV_ARGS(outRootSignature.GetAddressOf()))));
    }

    void Renderer::CreatePipelineState()
    {
        using namespace entt::literals;
        auto vertexShaderHandle = AssetManager::LoadShader("DefaultShader_VS"_hs, L"Engine/Content/Shader/DefaultShader_VS.hlsl", ShaderType::Vertex);
        auto pixelShaderHandle = AssetManager::LoadShader("DefaultShader_PS"_hs, L"Engine/Content/Shader/DefaultShader_PS.hlsl", ShaderType::Pixel);


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


        std::vector<D3D12_INPUT_ELEMENT_DESC> InputElements = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
        };


        pipelinStateSteam.RootSignature = m_RootSignature.Get();
        pipelinStateSteam.InputLayout = {InputElements.data(), static_cast<uint32_t>(InputElements.size())};
        pipelinStateSteam.VS = {vertexShaderHandle->Blob->GetBufferPointer(), vertexShaderHandle->Blob->GetBufferSize()};
        pipelinStateSteam.PS = {pixelShaderHandle->Blob->GetBufferPointer(), pixelShaderHandle->Blob->GetBufferSize()};
        pipelinStateSteam.PrimitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        D3D12_RT_FORMAT_ARRAY renderTargetFormats{};
        renderTargetFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        renderTargetFormats.NumRenderTargets = 1;
        pipelinStateSteam.RenderTargetFormats = renderTargetFormats;
        CD3DX12_RASTERIZER_DESC rasterizerDesc(D3D12_DEFAULT);
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
        rasterizerDesc.FrontCounterClockwise = true;
        pipelinStateSteam.Rasterizer = rasterizerDesc;
        pipelinStateSteam.DepthStencil = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
        pipelinStateSteam.DepthStencilFormat = DXGI_FORMAT_D32_FLOAT;


        D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {sizeof(pipelinStateSteam), &pipelinStateSteam};

        EG_CONFIRM(SUCCEEDED(Core::GetRenderContext().GetDevice()->CreatePipelineState(&pipelineStateStreamDesc, IID_PPV_ARGS(&m_PipelineState))));
    }


    void Renderer::LoadAssets()
    {
        using namespace entt::literals;
        auto meshHandle = AssetManager::LoadMesh("teapot"_hs, "Content/teapot.obj");

        CreateVertexAndIndexBufferView(meshHandle->Vertices, meshHandle->Indices, m_VertexBuffer, m_IndexBuffer, m_VertexBufferView, m_IndexBufferView);


        const auto textureHandle = Engine::AssetManager::LoadTexture("yoimiya_texture"_hs, "Content/sticker_6.png");
        m_DirectCommandList->Reset(m_DirectCommandAllocator.Get(), nullptr);
        GraphicsHelper::CreateTextureResource(textureHandle, m_DirectCommandQueue, m_Fence, m_FenceValue, m_DirectCommandList, m_Texture);
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Format = m_Texture->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Texture2D.MipLevels = 1;
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHeapHandle(m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->GetCPUDescriptorHandleForHeapStart(), 3, Core::GetRenderContext().GetIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
        Core::GetRenderContext().GetDevice()->CreateShaderResourceView(m_Texture.Get(), &srvDesc, srvHeapHandle);
    }

    void Renderer::SubmitGraphicsCommand(const RenderCommand& renderCommand)
    {
        m_RenderCommands.push_back(renderCommand);
    }
}
