// PostProcessManager.h
// ポストプロセスエフェクト管理クラス
// Bloom、ブラー、CRTなどのエフェクトを統合管理

#pragma once

#include "RenderTarget.h"
#include "FullscreenQuad.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <vector>

namespace NeonVector {
    namespace Graphics {

        using Microsoft::WRL::ComPtr;

        /**
         * @brief ポストプロセスマネージャークラス
         *
         * ポストプロセスエフェクトのパイプラインを管理します。
         * - レンダーターゲットの切り替え
         * - エフェクトの適用順序
         * - デスクリプタヒープの管理
         */
        class PostProcessManager {
        public:
            PostProcessManager();
            ~PostProcessManager();

            /**
             * @brief 初期化
             *
             * @param device DirectX12デバイス
             * @param width 画面幅
             * @param height 画面高さ
             * @return 成功時true
             */
            bool Initialize(ID3D12Device* device, uint32_t width, uint32_t height);

            /**
             * @brief リソースを解放
             */
            void Shutdown();

            /**
             * @brief シーンの描画開始
             *
             * 中間レンダーターゲットに描画を開始します。
             *
             * @param commandList コマンドリスト
             */
            void BeginScene(ID3D12GraphicsCommandList* commandList);

            /**
             * @brief シーンの描画終了
             *
             * 中間レンダーターゲットへの描画を終了します。
             *
             * @param commandList コマンドリスト
             */
            void EndScene(ID3D12GraphicsCommandList* commandList);

            /**
             * @brief Bloomエフェクトを適用
             *
             * @param commandList コマンドリスト
             * @param threshold 輝度閾値（0.5 ~ 2.0）
             * @param intensity Bloom強度（1.0 ~ 5.0）
             */
            void ApplyBloom(
                ID3D12GraphicsCommandList* commandList,
                float threshold = 1.0f,
                float intensity = 1.5f
            );

            /**
             * @brief 最終結果をバックバッファに描画
             *
             * @param commandList コマンドリスト
             * @param backBufferRTV バックバッファのRTV
             */
            void Present(
                ID3D12GraphicsCommandList* commandList,
                D3D12_CPU_DESCRIPTOR_HANDLE backBufferRTV
            );

            // ゲッター
            RenderTarget* GetSceneRT() { return m_sceneRT.get(); }
            RenderTarget* GetBloomRT() { return m_bloomRT.get(); }

            uint32_t GetWidth() const { return m_width; }
            uint32_t GetHeight() const { return m_height; }

        private:
            /**
             * @brief デスクリプタヒープを作成
             */
            bool CreateDescriptorHeaps(ID3D12Device* device);

            /**
             * @brief レンダーターゲットを作成
             */
            bool CreateRenderTargets(ID3D12Device* device);

            /**
             * @brief フルスクリーンクワッドを初期化
             */
            bool CreateFullscreenQuad(ID3D12Device* device);

            // DirectX12デバイス
            ID3D12Device* m_device;

            // 画面サイズ
            uint32_t m_width;
            uint32_t m_height;

            // レンダーターゲット
            std::unique_ptr<RenderTarget> m_sceneRT;      // シーン描画用
            std::unique_ptr<RenderTarget> m_bloomRT;      // Bloom用（小さいサイズ）
            std::unique_ptr<RenderTarget> m_blurTempRT;   // ブラー一時バッファ

            // フルスクリーンクワッド
            std::unique_ptr<FullscreenQuad> m_fullscreenQuad;

            // デスクリプタヒープ
            ComPtr<ID3D12DescriptorHeap> m_rtvHeap;  // Render Target View用
            ComPtr<ID3D12DescriptorHeap> m_srvHeap;  // Shader Resource View用

            // デスクリプタサイズ
            uint32_t m_rtvDescriptorSize;
            uint32_t m_srvDescriptorSize;

            // 現在使用中のRT
            RenderTarget* m_currentRT;
        };

    } // namespace Graphics
} // namespace NeonVector
