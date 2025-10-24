// FullscreenQuad.h
// フルスクリーン描画用のクワッド（矩形）
// ポストプロセスエフェクトで画面全体にシェーダーを適用するために使用

#pragma once

#include <d3d12.h>
#include <wrl/client.h>
#include <cstdint>

namespace NeonVector {
    namespace Graphics {

        using Microsoft::WRL::ComPtr;

        /**
         * @brief フルスクリーンクワッドクラス
         *
         * 画面全体を覆う矩形を描画するためのクラス。
         * ポストプロセスエフェクト（Bloom、ブラーなど）で使用します。
         *
         * 頂点データ:
         * - 位置: NDC座標系 (-1,-1) ～ (1,1)
         * - UV: テクスチャ座標 (0,0) ～ (1,1)
         */
        class FullscreenQuad {
        public:
            FullscreenQuad();
            ~FullscreenQuad();

            /**
             * @brief 初期化
             *
             * @param device DirectX12デバイス
             * @return 成功時true
             */
            bool Initialize(ID3D12Device* device);

            /**
             * @brief リソースを解放
             */
            void Shutdown();

            /**
             * @brief クワッドを描画
             *
             * 注: 事前にPipelineStateとRootSignatureを設定しておく必要があります。
             *
             * @param commandList コマンドリスト
             */
            void Draw(ID3D12GraphicsCommandList* commandList);

            // ゲッター
            D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const { return m_vertexBufferView; }

        private:
            /**
             * @brief 頂点データ構造
             */
            struct Vertex {
                float position[3];  // x, y, z (NDC座標)
                float uv[2];        // u, v (テクスチャ座標)
            };

            // 頂点バッファ
            ComPtr<ID3D12Resource> m_vertexBuffer;
            D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

            // 頂点データ（2つの三角形で矩形を構成）
            static constexpr uint32_t VERTEX_COUNT = 6;
        };

    } // namespace Graphics
} // namespace NeonVector
