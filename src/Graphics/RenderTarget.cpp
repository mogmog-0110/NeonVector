// RenderTarget.cpp
// RenderTargetクラスの実装

#include "../include/NeonVector/Graphics/RenderTarget.h"
#include <stdexcept>
#include <cstring>

// DirectX12ヘルパー（d3dx12.hから）
#ifndef CD3DX12_RESOURCE_BARRIER
namespace {
    inline D3D12_RESOURCE_BARRIER ResourceBarrierTransition(
        ID3D12Resource* pResource,
        D3D12_RESOURCE_STATES stateBefore,
        D3D12_RESOURCE_STATES stateAfter,
        UINT subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES)
    {
        D3D12_RESOURCE_BARRIER barrier = {};
        barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        barrier.Transition.pResource = pResource;
        barrier.Transition.StateBefore = stateBefore;
        barrier.Transition.StateAfter = stateAfter;
        barrier.Transition.Subresource = subresource;
        return barrier;
    }
}
#endif

namespace NeonVector {
    namespace Graphics {

        RenderTarget::RenderTarget()
            : m_width(0)
            , m_height(0)
            , m_format(DXGI_FORMAT_UNKNOWN)
            , m_currentState(D3D12_RESOURCE_STATE_COMMON)
        {
            m_clearColor[0] = 0.0f;
            m_clearColor[1] = 0.0f;
            m_clearColor[2] = 0.0f;
            m_clearColor[3] = 1.0f;

            m_rtvHandle = {};
            m_srvHandle = {};
            m_gpuSrvHandle = {};
        }

        RenderTarget::~RenderTarget() {
            Shutdown();
        }

        bool RenderTarget::Initialize(
            ID3D12Device* device,
            uint32_t width,
            uint32_t height,
            DXGI_FORMAT format,
            const float clearColor[4])
        {
            if (!device || width == 0 || height == 0) {
                return false;
            }

            m_width = width;
            m_height = height;
            m_format = format;

            if (clearColor) {
                std::memcpy(m_clearColor, clearColor, sizeof(m_clearColor));
            }

            // リソースの作成
            // ヒーププロパティ（GPU上のメモリ）
            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_DEFAULT;  // GPU専用メモリ
            heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heapProps.CreationNodeMask = 1;
            heapProps.VisibleNodeMask = 1;

            // リソースディスクリプション
            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = width;
            resourceDesc.Height = height;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = format;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;  // RTとして使用

            // 最適化されたクリア値
            D3D12_CLEAR_VALUE clearValue = {};
            clearValue.Format = format;
            clearValue.Color[0] = m_clearColor[0];
            clearValue.Color[1] = m_clearColor[1];
            clearValue.Color[2] = m_clearColor[2];
            clearValue.Color[3] = m_clearColor[3];

            // リソースの作成
            HRESULT hr = device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_RENDER_TARGET,  // 初期状態
                &clearValue,
                IID_PPV_ARGS(&m_resource)
            );

            if (FAILED(hr)) {
                return false;
            }

            // 初期状態を設定
            m_currentState = D3D12_RESOURCE_STATE_RENDER_TARGET;

            return true;
        }

        void RenderTarget::Shutdown() {
            m_resource.Reset();
            m_rtvHandle = {};
            m_srvHandle = {};
            m_gpuSrvHandle = {};
            m_width = 0;
            m_height = 0;
            m_format = DXGI_FORMAT_UNKNOWN;
            m_currentState = D3D12_RESOURCE_STATE_COMMON;
        }

        void RenderTarget::TransitionTo(
            ID3D12GraphicsCommandList* commandList,
            D3D12_RESOURCE_STATES stateBefore,
            D3D12_RESOURCE_STATES stateAfter)
        {
            if (!commandList || !m_resource) {
                return;
            }

            // 状態が同じ場合はバリア不要
            if (stateBefore == stateAfter) {
                return;
            }

            if (m_currentState != stateBefore) {
                // 警告ログ（デバッグ用）
#ifdef _DEBUG
                OutputDebugStringA("Warning: Resource state mismatch in TransitionTo\n");
#endif
                // 実際の現在の状態から遷移
                stateBefore = m_currentState;
            }

            // リソースバリアを設定
            D3D12_RESOURCE_BARRIER barrier = {};
            barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
            barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
            barrier.Transition.pResource = m_resource.Get();
            barrier.Transition.StateBefore = stateBefore;
            barrier.Transition.StateAfter = stateAfter;
            barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

            commandList->ResourceBarrier(1, &barrier);

            // 現在の状態を更新
            m_currentState = stateAfter;
        }

        void RenderTarget::Clear(ID3D12GraphicsCommandList* commandList) {
            if (!commandList || !m_resource) {
                return;
            }

            // RTVが設定されていることを確認
            if (m_rtvHandle.ptr == 0) {
                return;
            }

            // レンダーターゲットをクリア
            commandList->ClearRenderTargetView(
                m_rtvHandle,
                m_clearColor,
                0,
                nullptr
            );
        }

    } // namespace Graphics
} // namespace NeonVector
