// BloomEffect.h
// Bloomエフェクトクラス
// 輝度抽出、ブラー、合成の全パイプラインを管理

#pragma once

#include "../Graphics/RenderTarget.h"
#include "../Graphics/FullscreenQuad.h"
#include <d3d12.h>
#include <wrl/client.h>
#include <memory>
#include <vector>
#include <cstdint>

namespace NeonVector {
    namespace Effects {

        using Microsoft::WRL::ComPtr;

        /**
         * @brief Bloomエフェクトクラス
         *
         * 明るい部分を抽出し、ぼかして元の画像に合成することで、
         * 発光しているような視覚効果を実現。
         *
         * パイプライン:
         * 1. 輝度抽出（BrightPass）
         * 2. ダウンサンプリング（1/4解像度）
         * 3. ガウシアンブラー（水平・垂直）
         * 4. アップサンプリング
         * 5. 合成（Composite）
         */
        class BloomEffect {
        public:
            BloomEffect();
            ~BloomEffect();

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
             * @brief Bloomエフェクトを適用
             *
             * @param commandList コマンドリスト
             * @param sourceRT 入力レンダーターゲット（元のシーン）
             * @param outputRT 出力レンダーターゲット（Bloom適用後）
             */
            void Apply(
                ID3D12GraphicsCommandList* commandList,
                Graphics::RenderTarget* sourceRT,
                Graphics::RenderTarget* outputRT
            );

            // パラメータ設定
            void SetThreshold(float value) { m_threshold = value; }
            void SetIntensity(float value) { m_intensity = value; }
            void SetBloomStrength(float value) { m_bloomStrength = value; }
            void SetBlurRadius(float value) { m_blurRadius = value; }

            // パラメータ取得
            float GetThreshold() const { return m_threshold; }
            float GetIntensity() const { return m_intensity; }
            float GetBloomStrength() const { return m_bloomStrength; }
            float GetBlurRadius() const { return m_blurRadius; }

        private:
            /**
             * @brief 定数バッファ構造体（Bloom用）
             */
            struct BloomParams {
                float threshold;       // 輝度閾値
                float intensity;       // Bloom強度
                float bloomStrength;   // 合成時の強さ
                float padding;         // 16バイトアライメント用
            };

            /**
             * @brief 定数バッファ構造体（ブラー用）
             */
            struct BlurParams {
                float texelSize[2];    // テクセルサイズ
                float direction[2];    // ブラー方向
                float blurRadius;      // ブラー半径
                float padding[3];      // 16バイトアライメント用
            };

            /**
             * @brief ルートシグネチャを作成
             */
            bool CreateRootSignature(ID3D12Device* device);

            /**
             * @brief パイプラインステートを作成
             */
            bool CreatePipelineStates(ID3D12Device* device);

            /**
             * @brief 定数バッファを作成
             */
            bool CreateConstantBuffers(ID3D12Device* device);

            /**
             * @brief 輝度抽出パス
             */
            void BrightPass(
                ID3D12GraphicsCommandList* commandList,
                Graphics::RenderTarget* sourceRT,
                Graphics::RenderTarget* brightRT
            );

            /**
             * @brief ブラーパス
             */
            void BlurPass(
                ID3D12GraphicsCommandList* commandList,
                Graphics::RenderTarget* sourceRT,
                Graphics::RenderTarget* tempRT,
                Graphics::RenderTarget* outputRT
            );

            /**
             * @brief 合成パス
             */
            void CompositePass(
                ID3D12GraphicsCommandList* commandList,
                Graphics::RenderTarget* sceneRT,
                Graphics::RenderTarget* bloomRT,
                Graphics::RenderTarget* outputRT
            );

            /**
             * @brief シェーダーファイルを読み込む
             */
            bool LoadShader(const char* filepath, std::vector<uint8_t>& outData);

            // DirectX12デバイス
            ID3D12Device* m_device;

            // 画面サイズ
            uint32_t m_width;
            uint32_t m_height;

            // パラメータ
            float m_threshold;      // 輝度閾値（0.5 ~ 2.0）
            float m_intensity;      // Bloom強度（1.0 ~ 5.0）
            float m_bloomStrength;  // 合成時の強さ（0.5 ~ 2.0）
            float m_blurRadius;     // ブラー半径（1.0 ~ 5.0）

            // パイプラインステート
            ComPtr<ID3D12PipelineState> m_brightPassPSO;
            ComPtr<ID3D12PipelineState> m_blurPSO;
            ComPtr<ID3D12PipelineState> m_compositePSO;

            // ルートシグネチャ
            ComPtr<ID3D12RootSignature> m_rootSignature;

            // 定数バッファ
            ComPtr<ID3D12Resource> m_bloomParamsBuffer;
            ComPtr<ID3D12Resource> m_blurParamsBuffer;

            // デスクリプタヒープ（RenderTarget用）
            ComPtr<ID3D12DescriptorHeap> m_rtvHeap;
            ComPtr<ID3D12DescriptorHeap> m_srvHeap;

            // フルスクリーンクワッド
            std::unique_ptr<Graphics::FullscreenQuad> m_fullscreenQuad;

            // 中間レンダーターゲット
            std::unique_ptr<Graphics::RenderTarget> m_brightRT;    // 輝度抽出結果
            std::unique_ptr<Graphics::RenderTarget> m_blurTempRT;  // ブラー一時バッファ
        };

    } // namespace Effects
} // namespace NeonVector
