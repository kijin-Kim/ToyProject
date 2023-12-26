#pragma once

namespace Engine
{
    class RenderContext final
    {
        friend class Core;
    public:
        Microsoft::WRL::ComPtr<ID3D12Device4> GetDevice() const { return m_Device; }
        Microsoft::WRL::ComPtr<IDXGIFactory5> GetFactory() const { return m_Factory; }

    private:
        RenderContext();
        void EnableDebugLayer();
        void CreateFactory();
        void CreateDevice();

    private:
        Microsoft::WRL::ComPtr<ID3D12Device4> m_Device = nullptr;
        Microsoft::WRL::ComPtr<IDXGIFactory5> m_Factory = nullptr;

    };
}
