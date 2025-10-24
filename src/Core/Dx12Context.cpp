#include "../DX12Context.h"
#include <directx/d3dx12.h>
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
        m_width = width;
        m_height = height;

        if (!CreateDevice())
        {
            std::cerr << "Failed to create device" << std::endl;
            return false;
        }

        if (!CreateCommandObjects())
        {
            std::cerr << "Failed to create command objects" << std::endl;
            return false;
        }

        if (!CreateSwapChain(hwnd, width, height))
        {
            std::cerr << "Failed to create swap chain" << std::endl;
            return false;
        }

        if (!CreateRenderTargets())
        {
            std::cerr << "Failed to create render targets" << std::endl;
            return false;
        }

        if (!CreateFence())
        {
            std::cerr << "Failed to create fence" << std::endl;
            return false;
        }

        m_lineBatcher = std::make_unique<Graphics::LineBatcher>();
        if (!m_lineBatcher->Initialize(m_device.Get(), m_commandList.Get(), m_width, m_height))
        {
            std::cerr << "Failed to initialize LineBatcher" << std::endl;
            return false;
        }

        std::cout << "DX12Context: All systems initialized successfully" << std::endl;
        return true;
    }

    void DX12Context::Shutdown()
    {
        if (!m_isInitialized)
            return;

        WaitForGPU();

        if (m_lineBatcher)
        {
            m_lineBatcher->Shutdown();
            m_lineBatcher.reset();
        }

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
        // コマンドアロケーターのリセット
        m_commandAllocators[m_currentBackBufferIndex]->Reset();
        m_commandList->Reset(m_commandAllocators[m_currentBackBufferIndex].Get(), nullptr);

        // リソースバリア: PRESENT → RENDER_TARGET
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
            m_renderTargets[m_currentBackBufferIndex].Get(),
            D3D12_RESOURCE_STATE_PRESENT,
            D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_commandList->ResourceBarrier(1, &barrier);

        // レンダーターゲットを設定
        CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(
            m_rtvHeap->GetCPUDescriptorHandleForHeapStart(),
            m_currentBackBufferIndex,
            m_rtvDescriptorSize);
        m_commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

        // ビューポートとシザー矩形を設定（これが重要！
        D3D12_VIEWPORT viewport = {
            0.0f, 0.0f,                   // TopLeftX, TopLeftY
            static_cast<float>(m_width),  // Width
            static_cast<float>(m_height), // Height
            0.0f, 1.0f                    // MinDepth, MaxDepth
        };
        m_commandList->RSSetViewports(1, &viewport);

        D3D12_RECT scissorRect = {
            0, 0,                       // left, top
            static_cast<LONG>(m_width), // right
            static_cast<LONG>(m_height) // bottom
        };
        m_commandList->RSSetScissorRects(1, &scissorRect);
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