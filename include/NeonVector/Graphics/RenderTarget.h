// RenderTarget.h
// レンダーターゲット（中間描画先）の管理クラス
// Bloomなどのポストプロセスに必要

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace NeonVector {
    namespace Graphics {

        using Microsoft::WRL::ComPtr;

        /**
         * @brief レンダーターゲットクラス
         *
         * ポストプロセスエフェクトで使用する中間描画先を管理します。
         * DirectX12のリソース、RTV（Render Target View）、SRV（Shader Resource View）を保持します。
         */
        class RenderTarget {
        public:
            RenderTarget();
            ~RenderTarget();

            /**
             * @brief レンダーターゲットを初期化
             *
             * @param device DirectX12デバイス
             * @param width 幅（ピクセル）
             * @param height 高さ（ピクセル）
             * @param format ピクセルフォーマット（デフォルト: RGBA8）
             * @param clearColor クリア時の色
             * @return 成功時true
             */
            bool Initialize(
                ID3D12Device* device,
                uint32_t width,
                uint32_t height,
                DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM,
                const float clearColor[4] = nullptr
            );

            /**
             * @brief リソースを解放
             */
            void Shutdown();

            /**
             * @brief リソースの状態遷移バリアを追加
             *
             * @param commandList コマンドリスト
             * @param stateBefore 変更前の状態
             * @param stateAfter 変更後の状態
             */
            void TransitionTo(
                ID3D12GraphicsCommandList* commandList,
                D3D12_RESOURCE_STATES stateBefore,
                D3D12_RESOURCE_STATES stateAfter
            );

            /**
             * @brief レンダーターゲットをクリア
             *
             * @param commandList コマンドリスト
             */
            void Clear(ID3D12GraphicsCommandList* commandList);

            // ゲッター
            ID3D12Resource* GetResource() const { return m_resource.Get(); }
            D3D12_CPU_DESCRIPTOR_HANDLE GetRTVHandle() const { return m_rtvHandle; }
            D3D12_CPU_DESCRIPTOR_HANDLE GetSRVHandle() const { return m_srvHandle; }
            D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSRVHandle() const { return m_gpuSrvHandle; }

            uint32_t GetWidth() const { return m_width; }
            uint32_t GetHeight() const { return m_height; }
            DXGI_FORMAT GetFormat() const { return m_format; }
            D3D12_RESOURCE_STATES GetCurrentState() const { return m_currentState; }

            // セッター（デスクリプタヒープから取得したハンドルを設定）
            void SetRTVHandle(D3D12_CPU_DESCRIPTOR_HANDLE handle) { m_rtvHandle = handle; }
            void SetSRVHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) {
                m_srvHandle = cpuHandle;
                m_gpuSrvHandle = gpuHandle;
            }

        private:
            // DirectX12リソース
            ComPtr<ID3D12Resource> m_resource;

            // デスクリプタハンドル
            D3D12_CPU_DESCRIPTOR_HANDLE m_rtvHandle;  // Render Target View
            D3D12_CPU_DESCRIPTOR_HANDLE m_srvHandle;  // Shader Resource View (CPU)
            D3D12_GPU_DESCRIPTOR_HANDLE m_gpuSrvHandle;  // Shader Resource View (GPU)

            // プロパティ
            uint32_t m_width;
            uint32_t m_height;
            DXGI_FORMAT m_format;
            float m_clearColor[4];

            // 現在のリソース状態（バリア管理用）
            D3D12_RESOURCE_STATES m_currentState;
        };

    } // namespace Graphics
} // namespace NeonVector
