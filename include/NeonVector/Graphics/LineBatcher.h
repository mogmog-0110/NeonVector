/**
 * @file LineBatcher.h
 * @brief 複数の線描画を効率的にバッチング処理するクラス
 */
#pragma once

#include <NeonVector/Core/Types.h>
#include <NeonVector/Math/Vector2.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>
#include <vector>
#include <DirectXMath.h>
#include <directx/d3dx12.h>

using Microsoft::WRL::ComPtr;

namespace NeonVector {
    namespace Graphics {

        /**
         * @struct LineVertex
         * @brief 線描画用の頂点データ
         */
        struct LineVertex {
            DirectX::XMFLOAT2 position;
            DirectX::XMFLOAT4 color;
            float thickness;
            float glow;

            LineVertex()
                : position(0.0f, 0.0f)
                , color(1.0f, 1.0f, 1.0f, 1.0f)
                , thickness(1.0f)
                , glow(1.0f)
            {
            }

            LineVertex(const Vector2& pos, const Color& col,
                float thick = 1.0f, float glowVal = 1.0f)
                : position(pos.x, pos.y)
                , color(col.r, col.g, col.b, col.a)
                , thickness(thick)
                , glow(glowVal)
            {
            }
        };

        /**
         * @class LineBatcher
         * @brief 線描画のバッチング処理を行うクラス
         */
        class LineBatcher {
        public:
            LineBatcher();
            ~LineBatcher();

            bool Initialize(ID3D12Device* device,
                ID3D12GraphicsCommandList* commandList,
                int width,
                int height);

            void Shutdown();

            void AddLine(const Vector2& start,
                const Vector2& end,
                const Color& color,
                float thickness = 1.0f,
                float glow = 1.0f);

            void Flush();
            void Clear();

            size_t GetLineCount() const { return m_vertexCount / 2; }
            bool IsFull() const { return m_vertexCount >= kMaxVertices; }

            void UpdateScreenSize(int width, int height);

        private:
            bool createVertexBuffer();
            bool createPipelineState();
            bool createRootSignature();
            bool loadShaders();
            void uploadVertexData();

        private:
            static constexpr size_t kMaxLines = 10000;
            static constexpr size_t kMaxVertices = kMaxLines * 2;

            ComPtr<ID3D12Device> m_device;
            ComPtr<ID3D12GraphicsCommandList> m_commandList;

            ComPtr<ID3D12Resource> m_vertexBuffer;
            ComPtr<ID3D12Resource> m_vertexBufferUpload;
            D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;

            ComPtr<ID3D12RootSignature> m_rootSignature;
            ComPtr<ID3D12PipelineState> m_pipelineState;

            ComPtr<ID3DBlob> m_vertexShader;
            ComPtr<ID3DBlob> m_pixelShader;

            std::vector<LineVertex> m_vertices;
            size_t m_vertexCount;

            int m_screenWidth;
            int m_screenHeight;

            bool m_isInitialized;
        };

    } // namespace Graphics
} // namespace NeonVector