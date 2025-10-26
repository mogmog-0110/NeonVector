/**
 * @file DX12Context.h
 * @brief DirectX12のコンテキスト管理
 */

#pragma once

#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <vector>
#include <memory>

#include <NeonVector/Graphics/LineBatcher.h>
#include <NeonVector/Graphics/RenderTarget.h>

using Microsoft::WRL::ComPtr;

namespace NeonVector
{

    /**
     * @class DX12Context
     * @brief DirectX12のコンテキスト管理（内部実装用）
     */
    class DX12Context
    {
    public:
        DX12Context();
        ~DX12Context();

        bool Initialize(HWND hwnd, int width, int height);
        void Shutdown();

        void BeginFrame();
        void EndFrame();
        void Present(bool vsync);

        void ClearRenderTarget(float r, float g, float b, float a);

        Graphics::LineBatcher *GetLineBatcher() { return m_lineBatcher.get(); }
        ID3D12Device* GetDevice() { return m_device.Get(); }
        ID3D12GraphicsCommandList* GetCommandList() { return m_commandList.Get(); }
        Graphics::RenderTarget* GetCurrentRenderTarget();

    private:
        bool CreateDevice();
        bool CreateCommandObjects();
        bool CreateSwapChain(HWND hwnd, int width, int height);
        bool CreateRenderTargets();
        bool CreateFence();

        void WaitForGPU();
        void MoveToNextFrame();

    private:
        static constexpr UINT kBackBufferCount = 2;

        ComPtr<ID3D12Device> m_device;
        ComPtr<ID3D12CommandQueue> m_commandQueue;
        ComPtr<ID3D12CommandAllocator> m_commandAllocators[kBackBufferCount];
        ComPtr<ID3D12GraphicsCommandList> m_commandList;

        ComPtr<IDXGISwapChain3> m_swapChain;
        ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
        ComPtr<ID3D12Resource> m_renderTargets[kBackBufferCount];
        UINT m_rtvDescriptorSize;

        ComPtr<ID3D12Fence> m_fence;
        UINT64 m_fenceValues[kBackBufferCount];
        HANDLE m_fenceEvent;

        UINT m_currentBackBufferIndex;
        HWND m_hwnd;
        int m_width, m_height;
        bool m_isInitialized;

        ComPtr<ID3D12DescriptorHeap> m_postProcessRtvHeap;
        ComPtr<ID3D12DescriptorHeap> m_postProcessSrvHeap;
        UINT m_postProcessRtvDescriptorSize;
        UINT m_postProcessSrvDescriptorSize;

        std::unique_ptr<Graphics::LineBatcher> m_lineBatcher;
        std::unique_ptr<Graphics::RenderTarget> m_currentRenderTarget;
    };

} // namespace NeonVector
