// PostProcessManager.cpp
// PostProcessManagerクラスの実装

#include "../include/NeonVector/Graphics/PostProcessManager.h"
#include <stdexcept>

namespace NeonVector {
    namespace Graphics {

        PostProcessManager::PostProcessManager()
            : m_device(nullptr)
            , m_width(0)
            , m_height(0)
            , m_rtvDescriptorSize(0)
            , m_srvDescriptorSize(0)
            , m_currentRT(nullptr)
        {
        }

        PostProcessManager::~PostProcessManager() {
            Shutdown();
        }

        bool PostProcessManager::Initialize(ID3D12Device* device, uint32_t width, uint32_t height) {
            if (!device || width == 0 || height == 0) {
                return false;
            }

            m_device = device;
            m_width = width;
            m_height = height;

            // デスクリプタサイズを取得
            m_rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
            m_srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

            // デスクリプタヒープの作成
            if (!CreateDescriptorHeaps(device)) {
                return false;
            }

            // レンダーターゲットの作成
            if (!CreateRenderTargets(device)) {
                return false;
            }

            // フルスクリーンクワッドの作成
            if (!CreateFullscreenQuad(device)) {
                return false;
            }

            return true;
        }

        void PostProcessManager::Shutdown() {
            m_fullscreenQuad.reset();
            m_sceneRT.reset();
            m_bloomRT.reset();
            m_blurTempRT.reset();

            m_rtvHeap.Reset();
            m_srvHeap.Reset();

            m_device = nullptr;
            m_currentRT = nullptr;
            m_width = 0;
            m_height = 0;
        }

        bool PostProcessManager::CreateDescriptorHeaps(ID3D12Device* device) {
            // RTV用デスクリプタヒープ
            // 必要数: sceneRT(1) + bloomRT(1) + blurTempRT(1) = 3
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.NumDescriptors = 3;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            rtvHeapDesc.NodeMask = 0;

            HRESULT hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
            if (FAILED(hr)) {
                return false;
            }

            // SRV用デスクリプタヒープ
            // 必要数: sceneRT(1) + bloomRT(1) + blurTempRT(1) = 3
            // シェーダーからアクセス可能にするため、SHADER_VISIBLEフラグを設定
            D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
            srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            srvHeapDesc.NumDescriptors = 3;
            srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            srvHeapDesc.NodeMask = 0;

            hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));
            if (FAILED(hr)) {
                return false;
            }

            return true;
        }

        bool PostProcessManager::CreateRenderTargets(ID3D12Device* device) {
            // シーン用RT（フル解像度）
            m_sceneRT = std::make_unique<RenderTarget>();
            if (!m_sceneRT->Initialize(device, m_width, m_height)) {
                return false;
            }

            // Bloom用RT（1/4解像度で高速化）
            const uint32_t bloomWidth = m_width / 4;
            const uint32_t bloomHeight = m_height / 4;

            m_bloomRT = std::make_unique<RenderTarget>();
            if (!m_bloomRT->Initialize(device, bloomWidth, bloomHeight)) {
                return false;
            }

            // ブラー一時バッファ用RT（Bloom と同じサイズ）
            m_blurTempRT = std::make_unique<RenderTarget>();
            if (!m_blurTempRT->Initialize(device, bloomWidth, bloomHeight)) {
                return false;
            }

            // RTV（Render Target View）の作成
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

            // Scene RT
            device->CreateRenderTargetView(m_sceneRT->GetResource(), nullptr, rtvHandle);
            m_sceneRT->SetRTVHandle(rtvHandle);
            rtvHandle.ptr += m_rtvDescriptorSize;

            // Bloom RT
            device->CreateRenderTargetView(m_bloomRT->GetResource(), nullptr, rtvHandle);
            m_bloomRT->SetRTVHandle(rtvHandle);
            rtvHandle.ptr += m_rtvDescriptorSize;

            // Blur Temp RT
            device->CreateRenderTargetView(m_blurTempRT->GetResource(), nullptr, rtvHandle);
            m_blurTempRT->SetRTVHandle(rtvHandle);

            // SRV（Shader Resource View）の作成
            D3D12_CPU_DESCRIPTOR_HANDLE cpuSrvHandle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
            D3D12_GPU_DESCRIPTOR_HANDLE gpuSrvHandle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();

            D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
            srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
            srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
            srvDesc.Texture2D.MipLevels = 1;

            // Scene RT
            device->CreateShaderResourceView(m_sceneRT->GetResource(), &srvDesc, cpuSrvHandle);
            m_sceneRT->SetSRVHandle(cpuSrvHandle, gpuSrvHandle);
            cpuSrvHandle.ptr += m_srvDescriptorSize;
            gpuSrvHandle.ptr += m_srvDescriptorSize;

            // Bloom RT
            device->CreateShaderResourceView(m_bloomRT->GetResource(), &srvDesc, cpuSrvHandle);
            m_bloomRT->SetSRVHandle(cpuSrvHandle, gpuSrvHandle);
            cpuSrvHandle.ptr += m_srvDescriptorSize;
            gpuSrvHandle.ptr += m_srvDescriptorSize;

            // Blur Temp RT
            device->CreateShaderResourceView(m_blurTempRT->GetResource(), &srvDesc, cpuSrvHandle);
            m_blurTempRT->SetSRVHandle(cpuSrvHandle, gpuSrvHandle);

            return true;
        }

        bool PostProcessManager::CreateFullscreenQuad(ID3D12Device* device) {
            m_fullscreenQuad = std::make_unique<FullscreenQuad>();
            return m_fullscreenQuad->Initialize(device);
        }

        void PostProcessManager::BeginScene(ID3D12GraphicsCommandList* commandList) {
            if (!commandList || !m_sceneRT) {
                return;
            }

            // シーンRTをレンダーターゲット状態に遷移
            m_sceneRT->TransitionTo(
                commandList,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,  // 前の状態（テクスチャとして使用）
                D3D12_RESOURCE_STATE_RENDER_TARGET           // レンダーターゲットとして使用
            );

            // レンダーターゲットをクリア
            m_sceneRT->Clear(commandList);

            // レンダーターゲットを設定
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_sceneRT->GetRTVHandle();
            commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

            // ビューポートとシザー矩形を設定
            D3D12_VIEWPORT viewport = {};
            viewport.TopLeftX = 0.0f;
            viewport.TopLeftY = 0.0f;
            viewport.Width = static_cast<float>(m_width);
            viewport.Height = static_cast<float>(m_height);
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            commandList->RSSetViewports(1, &viewport);

            D3D12_RECT scissorRect = {};
            scissorRect.left = 0;
            scissorRect.top = 0;
            scissorRect.right = static_cast<LONG>(m_width);
            scissorRect.bottom = static_cast<LONG>(m_height);
            commandList->RSSetScissorRects(1, &scissorRect);

            m_currentRT = m_sceneRT.get();
        }

        void PostProcessManager::EndScene(ID3D12GraphicsCommandList* commandList) {
            if (!commandList || !m_sceneRT) {
                return;
            }

            // シーンRTをシェーダーリソース状態に遷移
            m_sceneRT->TransitionTo(
                commandList,
                D3D12_RESOURCE_STATE_RENDER_TARGET,          // レンダーターゲットとして使用していた
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE   // テクスチャとして使用する
            );

            m_currentRT = nullptr;
        }

        void PostProcessManager::ApplyBloom(
            ID3D12GraphicsCommandList* commandList,
            float threshold,
            float intensity)
        {
            // TODO: Week 3 Day 17-20で実装
            // 現時点では空実装
            // 実装内容:
            // 1. 輝度抽出パス
            // 2. ダウンサンプリング
            // 3. ガウシアンブラー（水平・垂直）
            // 4. アップサンプリング
            // 5. 合成
        }

        void PostProcessManager::Present(
            ID3D12GraphicsCommandList* commandList,
            D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV)
        {
            if (!commandList || !m_sceneRT || !m_fullscreenQuad) {
                return;
            }

            // バックバッファをレンダーターゲットに設定
            commandList->OMSetRenderTargets(1, &backBufferRTV, FALSE, nullptr);

            // ビューポートとシザー矩形を設定
            D3D12_VIEWPORT viewport = {};
            viewport.TopLeftX = 0.0f;
            viewport.TopLeftY = 0.0f;
            viewport.Width = static_cast<float>(m_width);
            viewport.Height = static_cast<float>(m_height);
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            commandList->RSSetViewports(1, &viewport);

            D3D12_RECT scissorRect = {};
            scissorRect.left = 0;
            scissorRect.top = 0;
            scissorRect.right = static_cast<LONG>(m_width);
            scissorRect.bottom = static_cast<LONG>(m_height);
            commandList->RSSetScissorRects(1, &scissorRect);

            // SRVデスクリプタヒープを設定
            ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
            commandList->SetDescriptorHeaps(1, heaps);

            // TODO: ここでPipelineStateとRootSignatureを設定
            // 現時点では実装していないため、フルスクリーンクワッドは描画しない

            // シーンテクスチャをバックバッファにコピー
            // （Bloomなどのエフェクトが実装されたら、エフェクト適用後のテクスチャを使用）

            // フルスクリーンクワッドを描画
            // m_fullscreenQuad->Draw(commandList);
        }

    } // namespace Graphics
} // namespace NeonVector
