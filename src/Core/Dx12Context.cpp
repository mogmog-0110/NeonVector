#include "../DX12Context.h"
#include "../../thirdparty/d3dx12.h"
#include <iostream>
#include <stdexcept>

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

// ヘルパーマクロ
#define ThrowIfFailed(hr)                          \
    if (FAILED(hr))                                \
    {                                              \
        throw std::runtime_error("DirectX Error"); \
    }

namespace NeonVector
{

    DX12Context::DX12Context()
        : m_device(nullptr), m_commandQueue(nullptr), m_swapChain(nullptr), m_rtvHeap(nullptr), m_rtvDescriptorSize(0), m_fence(nullptr), m_fenceEvent(nullptr), m_currentBackBufferIndex(0), m_hwnd(nullptr), m_width(0), m_height(0), m_isInitialized(false)
    {
        for (UINT i = 0; i < kBackBufferCount; i++)
        {
            m_fenceValues[i] = 0;
        }
    }

    DX12Context::~DX12Context()
    {
        Shutdown();
    }

    bool DX12Context::Initialize(HWND hwnd, int width, int height)
    {
        m_hwnd = hwnd;
        m_width = width;
        m_height = height;

        try
        {
            if (!CreateDevice())
                return false;
            if (!CreateCommandObjects())
                return false;
            if (!CreateSwapChain(hwnd, width, height))
                return false;
            if (!CreateRenderTargets())
                return false;
            if (!CreateFence())
                return false;

            m_isInitialized = true;
            std::cout << "DirectX12 initialized successfully" << std::endl;
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "DX12 Initialization failed: " << e.what() << std::endl;
            return false;
        }
    }

    void DX12Context::Shutdown()
    {
        if (!m_isInitialized)
            return;

        WaitForGPU();

        if (m_fenceEvent)
        {
            CloseHandle(m_fenceEvent);
            m_fenceEvent = nullptr;
        }

        m_isInitialized = false;
        std::cout << "DirectX12 shutdown complete" << std::endl;
    }

    bool DX12Context::CreateDevice()
    {
#ifdef _DEBUG
        // デバッグレイヤー有効化
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            std::cout << "D3D12 Debug Layer enabled" << std::endl;
        }
#endif

        // デバイス作成
        HRESULT hr = D3D12CreateDevice(
            nullptr,
            D3D_FEATURE_LEVEL_12_0,
            IID_PPV_ARGS(&m_device));

        ThrowIfFailed(hr);
        std::cout << "D3D12 Device created" << std::endl;
        return true;
    }

    bool DX12Context::CreateCommandObjects()
    {
        // コマンドキュー作成
        D3D12_COMMAND_QUEUE_DESC queueDesc = {};
        queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;

        ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));

        // コマンドアロケーター作成
        for (UINT i = 0; i < kBackBufferCount; i++)
        {
            ThrowIfFailed(m_device->CreateCommandAllocator(
                D3D12_COMMAND_LIST_TYPE_DIRECT,
                IID_PPV_ARGS(&m_commandAllocators[i])));
        }

        // コマンドリスト作成
        ThrowIfFailed(m_device->CreateCommandList(
            0,
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            m_commandAllocators[0].Get(),
            nullptr,
            IID_PPV_ARGS(&m_commandList)));

        m_commandList->Close();

        std::cout << "Command objects created" << std::endl;
        return true;
    }

    bool DX12Context::CreateSwapChain(HWND hwnd, int width, int height)
    {
        ComPtr<IDXGIFactory4> factory;
        ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

        DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
        swapChainDesc.BufferCount = kBackBufferCount;
        swapChainDesc.Width = width;
        swapChainDesc.Height = height;
        swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapChainDesc.SampleDesc.Count = 1;

        ComPtr<IDXGISwapChain1> swapChain;
        ThrowIfFailed(factory->CreateSwapChainForHwnd(
            m_commandQueue.Get(),
            hwnd,
            &swapChainDesc,
            nullptr,
            nullptr,
            &swapChain));

        ThrowIfFailed(swapChain.As(&m_swapChain));
        m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

        std::cout << "SwapChain created" << std::endl;
        return true;
    }

    bool DX12Context::CreateRenderTargets()
    {
        // RTVヒープ作成
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = kBackBufferCount;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

        ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap)));
        m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

        // レンダーターゲットビュー作成
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvHeap->GetCPUDescriptorHandleForHeapStart());

        for (UINT i = 0; i < kBackBufferCount; i++)
        {
            ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_renderTargets[i])));
            m_device->CreateRenderTargetView(m_renderTargets[i].Get(), nullptr, rtvHandle);
            rtvHandle.Offset(1, m_rtvDescriptorSize);
        }

        std::cout << "Render targets created" << std::endl;
        return true;
    }

    bool DX12Context::CreateFence()
    {
        ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
        m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);

        if (m_fenceEvent == nullptr)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        std::cout << "Fence created" << std::endl;
        return true;
    }

    void DX12Context::BeginFrame()
    {
        ThrowIfFailed(m_commandAllocators[m_currentBackBufferIndex]->Reset());
        ThrowIfFailed(m_commandList->Reset(m_commandAllocators[m_currentBackBufferIndex].Get(), nullptr));

        // リソースバリア（Present → RenderTarget）
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_renderTargets[m_currentBackBufferIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        m_commandList->ResourceBarrier(1, &barrier);
    }

    void DX12Context::ClearRenderTarget(float r, float g, float b, float a)
    {
        float clearColor[] = {r, g, b, a};

        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
            m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            m_currentBackBufferIndex,
            m_rtvDescriptorSize);

        m_commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    }

    void DX12Context::EndFrame()
    {
        // リソースバリア（RenderTarget → Present）
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Transition.pResource = m_renderTargets[m_currentBackBufferIndex].Get();
        barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
        barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

        m_commandList->ResourceBarrier(1, &barrier);
        ThrowIfFailed(m_commandList->Close());
    }

    void DX12Context::Present(bool vsync)
    {
        ID3D12CommandList *commandLists[] = {m_commandList.Get()};
        m_commandQueue->ExecuteCommandLists(1, commandLists);

        ThrowIfFailed(m_swapChain->Present(vsync ? 1 : 0, 0));

        MoveToNextFrame();
    }

    void DX12Context::WaitForGPU()
    {
        const UINT64 fence = m_fenceValues[m_currentBackBufferIndex];
        ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));

        if (m_fence->GetCompletedValue() < fence)
        {
            ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }
    }

    void DX12Context::MoveToNextFrame()
    {
        const UINT64 currentFenceValue = m_fenceValues[m_currentBackBufferIndex];
        ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), currentFenceValue));

        m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

        if (m_fence->GetCompletedValue() < m_fenceValues[m_currentBackBufferIndex])
        {
            ThrowIfFailed(m_fence->SetEventOnCompletion(m_fenceValues[m_currentBackBufferIndex], m_fenceEvent));
            WaitForSingleObject(m_fenceEvent, INFINITE);
        }

        m_fenceValues[m_currentBackBufferIndex] = currentFenceValue + 1;
    }

} // namespace NeonVector