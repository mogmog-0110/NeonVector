// FullscreenQuad.cpp
// FullscreenQuadクラスの実装

#include "../include/NeonVector/Graphics/FullscreenQuad.h"
#include <stdexcept>

namespace NeonVector {
    namespace Graphics {

        FullscreenQuad::FullscreenQuad()
        {
            m_vertexBufferView = {};
        }

        FullscreenQuad::~FullscreenQuad() {
            Shutdown();
        }

        bool FullscreenQuad::Initialize(ID3D12Device* device) {
            if (!device) {
                return false;
            }

            // フルスクリーンクワッドの頂点データ
            // NDC座標系: (-1,-1)が左下、(1,1)が右上
            // UV座標系: (0,0)が左上、(1,1)が右下
            const Vertex vertices[VERTEX_COUNT] = {
                // 第1三角形（左下 → 右上 → 左上）
                { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },  // 左下
                { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f } },  // 右上
                { { -1.0f,  1.0f, 0.0f }, { 0.0f, 0.0f } },  // 左上

                // 第2三角形（左下 → 右下 → 右上）
                { { -1.0f, -1.0f, 0.0f }, { 0.0f, 1.0f } },  // 左下
                { {  1.0f, -1.0f, 0.0f }, { 1.0f, 1.0f } },  // 右下
                { {  1.0f,  1.0f, 0.0f }, { 1.0f, 0.0f } },  // 右上
            };

            const uint32_t vertexBufferSize = sizeof(vertices);

            // ヒーププロパティ（CPU→GPU転送用）
            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;  // CPUから書き込み可能
            heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heapProps.CreationNodeMask = 1;
            heapProps.VisibleNodeMask = 1;

            // リソースディスクリプション
            D3D12_RESOURCE_DESC resourceDesc = {};
            resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            resourceDesc.Alignment = 0;
            resourceDesc.Width = vertexBufferSize;
            resourceDesc.Height = 1;
            resourceDesc.DepthOrArraySize = 1;
            resourceDesc.MipLevels = 1;
            resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
            resourceDesc.SampleDesc.Count = 1;
            resourceDesc.SampleDesc.Quality = 0;
            resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            // 頂点バッファの作成
            HRESULT hr = device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &resourceDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_vertexBuffer)
            );

            if (FAILED(hr)) {
                return false;
            }

            // 頂点データをバッファにコピー
            void* mappedData = nullptr;
            D3D12_RANGE readRange = { 0, 0 };  // CPUから読み取らない

            hr = m_vertexBuffer->Map(0, &readRange, &mappedData);
            if (FAILED(hr)) {
                return false;
            }

            memcpy(mappedData, vertices, vertexBufferSize);
            m_vertexBuffer->Unmap(0, nullptr);

            // 頂点バッファビューの設定
            m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
            m_vertexBufferView.StrideInBytes = sizeof(Vertex);
            m_vertexBufferView.SizeInBytes = vertexBufferSize;

            return true;
        }

        void FullscreenQuad::Shutdown() {
            m_vertexBuffer.Reset();
            m_vertexBufferView = {};
        }

        void FullscreenQuad::Draw(ID3D12GraphicsCommandList* commandList) {
            if (!commandList || !m_vertexBuffer) {
                return;
            }

            // 頂点バッファをセット
            commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

            // トポロジーを設定（三角形リスト）
            commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            // 描画（6頂点 = 2三角形）
            commandList->DrawInstanced(VERTEX_COUNT, 1, 0, 0);
        }

    } // namespace Graphics
} // namespace NeonVector
