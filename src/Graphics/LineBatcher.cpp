#include <NeonVector/Graphics/LineBatcher.h>
#include <d3dcompiler.h>
#include <directx/d3dx12.h>
#include <iostream>
#include <filesystem>

#pragma comment(lib, "d3dcompiler.lib")

using namespace DirectX;

namespace NeonVector
{
    namespace Graphics
    {

        // コンストラクタ
        LineBatcher::LineBatcher()
            : m_device(nullptr), m_commandList(nullptr), m_vertexCount(0), m_screenWidth(0), m_screenHeight(0), m_isInitialized(false)
        {
            m_vertices.reserve(kMaxVertices);
        }

        // デストラクタ
        LineBatcher::~LineBatcher()
        {
            Shutdown();
        }

        // 初期化
        bool LineBatcher::Initialize(ID3D12Device *device,
                                     ID3D12GraphicsCommandList *commandList,
                                     int width,
                                     int height)
        {
            if (!device || !commandList)
            {
                OutputDebugStringA("LineBatcher: Invalid device or command list\n");
                return false;
            }

            m_device = device;
            m_commandList = commandList;
            m_screenWidth = width;
            m_screenHeight = height;

            // 頂点バッファ作成
            if (!createVertexBuffer())
            {
                return false;
            }

            // シェーダー読み込み
            if (!loadShaders())
            {
                return false;
            }

            // ルートシグネチャ作成
            if (!createRootSignature())
            {
                return false;
            }

            // パイプラインステート作成
            if (!createPipelineState())
            {
                return false;
            }

            m_isInitialized = true;
            OutputDebugStringA("LineBatcher: Initialization complete\n");
            return true;
        }

        // 終了処理
        void LineBatcher::Shutdown()
        {
            m_vertices.clear();
            m_isInitialized = false;
        }

        // 線を追加
        void LineBatcher::AddLine(const Vector2 &start,
                                  const Vector2 &end,
                                  const Color &color,
                                  float thickness,
                                  float glow)
        {
            if (IsFull())
            {
                OutputDebugStringA("LineBatcher: Buffer full, flushing...\n");
                Flush();
            }

            m_vertices.emplace_back(start, color, thickness, glow);
            m_vertices.emplace_back(end, color, thickness, glow);
            m_vertexCount += 2;
        }

        // クリア
        void LineBatcher::Clear()
        {
            m_vertices.clear();
            m_vertexCount = 0;
        }

        // 画面サイズ更新
        void LineBatcher::UpdateScreenSize(int width, int height)
        {
            m_screenWidth = width;
            m_screenHeight = height;
        }

        // 描画実行
        void LineBatcher::Flush()
        {
            if (m_vertexCount == 0)
            {
                return;
            }

            if (!m_isInitialized || !m_commandList)
            {
                OutputDebugStringA("LineBatcher: Cannot flush, not initialized\n");
                return;
            }

            char buffer[256];
            sprintf_s(buffer, "LineBatcher: Flushing %zu lines\n", m_vertexCount / 2);
            OutputDebugStringA(buffer);

            // 頂点データをアップロード
            uploadVertexData();

            // パイプラインステートを設定
            m_commandList->SetPipelineState(m_pipelineState.Get());
            m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());

            // プリミティブトポロジーを設定
            m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

            // 頂点バッファをバインド
            m_commandList->IASetVertexBuffers(0, 1, &m_vertexBufferView);

            // 定数バッファ（画面サイズ）を設定
            struct ScreenConstants
            {
                float screenWidth;
                float screenHeight;
                float padding[2];
            };

            ScreenConstants constants = {
                static_cast<float>(m_screenWidth),
                static_cast<float>(m_screenHeight),
                {0.0f, 0.0f}};

            m_commandList->SetGraphicsRoot32BitConstants(
                0,
                sizeof(ScreenConstants) / 4,
                &constants,
                0);

            // 描画
            m_commandList->DrawInstanced(
                static_cast<UINT>(m_vertexCount),
                1,
                0,
                0);

            OutputDebugStringA("LineBatcher: Draw command executed\n");

            // クリア
            Clear();
        }

        // 頂点バッファ作成
        bool LineBatcher::createVertexBuffer()
        {
            const size_t vertexBufferSize = sizeof(LineVertex) * kMaxVertices;

            // デフォルトヒープ: 初期状態を VERTEX_AND_CONSTANT_BUFFER に変更
            CD3DX12_HEAP_PROPERTIES defaultHeapProps(D3D12_HEAP_TYPE_DEFAULT);
            CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

            HRESULT hr = m_device->CreateCommittedResource(
                &defaultHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER, // ← ここを変更
                nullptr,
                IID_PPV_ARGS(&m_vertexBuffer));

            if (FAILED(hr))
            {
                OutputDebugStringA("LineBatcher: Failed to create vertex buffer\n");
                return false;
            }

            // アップロードヒープ
            CD3DX12_HEAP_PROPERTIES uploadHeapProps(D3D12_HEAP_TYPE_UPLOAD);

            hr = m_device->CreateCommittedResource(
                &uploadHeapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_vertexBufferUpload));

            if (FAILED(hr))
            {
                OutputDebugStringA("LineBatcher: Failed to create upload buffer\n");
                return false;
            }

            m_vertexBufferView.BufferLocation = m_vertexBuffer->GetGPUVirtualAddress();
            m_vertexBufferView.SizeInBytes = static_cast<UINT>(vertexBufferSize);
            m_vertexBufferView.StrideInBytes = sizeof(LineVertex);

            OutputDebugStringA("LineBatcher: Vertex buffer created\n");
            return true;
        }

        // シェーダー読み込み
        bool LineBatcher::loadShaders()
        {
            wchar_t exePath[MAX_PATH];
            GetModuleFileNameW(nullptr, exePath, MAX_PATH);
            std::filesystem::path exeDir = std::filesystem::path(exePath).parent_path();

            std::vector<std::filesystem::path> searchPaths = {
                exeDir / "shaders",
                exeDir.parent_path() / "shaders",
                exeDir.parent_path().parent_path() / "shaders"};

            std::filesystem::path vsPath, psPath;
            bool found = false;

            for (const auto &basePath : searchPaths)
            {
                auto vs = basePath / "Line_VS.cso";
                auto ps = basePath / "Line_PS.cso";

                if (std::filesystem::exists(vs) && std::filesystem::exists(ps))
                {
                    vsPath = vs;
                    psPath = ps;
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                OutputDebugStringA("LineBatcher: Shader files not found\n");
                return false;
            }

            HRESULT hr = D3DReadFileToBlob(vsPath.c_str(), &m_vertexShader);
            if (FAILED(hr))
            {
                OutputDebugStringA("LineBatcher: Failed to load vertex shader\n");
                return false;
            }

            hr = D3DReadFileToBlob(psPath.c_str(), &m_pixelShader);
            if (FAILED(hr))
            {
                OutputDebugStringA("LineBatcher: Failed to load pixel shader\n");
                return false;
            }

            OutputDebugStringA("LineBatcher: Shaders loaded\n");
            return true;
        }

        // ルートシグネチャ作成
        bool LineBatcher::createRootSignature()
        {
            CD3DX12_ROOT_PARAMETER rootParameters[1];
            rootParameters[0].InitAsConstants(4, 0, 0);

            CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
            rootSignatureDesc.Init(
                1,
                rootParameters,
                0,
                nullptr,
                D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> error;

            HRESULT hr = D3D12SerializeRootSignature(
                &rootSignatureDesc,
                D3D_ROOT_SIGNATURE_VERSION_1,
                &signature,
                &error);

            if (FAILED(hr))
            {
                if (error)
                {
                    OutputDebugStringA((char *)error->GetBufferPointer());
                }
                return false;
            }

            hr = m_device->CreateRootSignature(
                0,
                signature->GetBufferPointer(),
                signature->GetBufferSize(),
                IID_PPV_ARGS(&m_rootSignature));

            if (FAILED(hr))
            {
                OutputDebugStringA("LineBatcher: Failed to create root signature\n");
                return false;
            }

            OutputDebugStringA("LineBatcher: Root signature created\n");
            return true;
        }

        // パイプラインステート作成
        bool LineBatcher::createPipelineState()
        {
            D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
                {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0,
                 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"THICKNESS", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                {"GLOW", 0, DXGI_FORMAT_R32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT,
                 D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}};

            D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
            psoDesc.InputLayout = {inputLayout, _countof(inputLayout)};
            psoDesc.pRootSignature = m_rootSignature.Get();
            psoDesc.VS = {m_vertexShader->GetBufferPointer(), m_vertexShader->GetBufferSize()};
            psoDesc.PS = {m_pixelShader->GetBufferPointer(), m_pixelShader->GetBufferSize()};

            psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
            psoDesc.RasterizerState.AntialiasedLineEnable = TRUE;

            psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

            psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            psoDesc.DepthStencilState.DepthEnable = FALSE;

            psoDesc.SampleMask = UINT_MAX;
            psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
            psoDesc.NumRenderTargets = 1;
            psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
            psoDesc.SampleDesc.Count = 1;

            HRESULT hr = m_device->CreateGraphicsPipelineState(
                &psoDesc,
                IID_PPV_ARGS(&m_pipelineState));

            if (FAILED(hr))
            {
                OutputDebugStringA("LineBatcher: Failed to create pipeline state\n");
                return false;
            }

            OutputDebugStringA("LineBatcher: Pipeline state created\n");
            return true;
        }

        // 頂点データアップロード
        void LineBatcher::uploadVertexData()
        {
            if (m_vertices.empty())
            {
                return;
            }

            // アップロードバッファにデータをコピー
            void *pData = nullptr;
            CD3DX12_RANGE readRange(0, 0);

            HRESULT hr = m_vertexBufferUpload->Map(0, &readRange, &pData);
            if (FAILED(hr))
            {
                OutputDebugStringA("LineBatcher: Failed to map upload buffer\n");
                return;
            }

            memcpy(pData, m_vertices.data(), m_vertices.size() * sizeof(LineVertex));
            m_vertexBufferUpload->Unmap(0, nullptr);

            // リソースバリア: COPY_DEST に遷移
            CD3DX12_RESOURCE_BARRIER barrierToCopy = CD3DX12_RESOURCE_BARRIER::Transition(
                m_vertexBuffer.Get(),
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
                D3D12_RESOURCE_STATE_COPY_DEST);
            m_commandList->ResourceBarrier(1, &barrierToCopy);

            // アップロードバッファからデフォルトバッファにコピー
            m_commandList->CopyResource(m_vertexBuffer.Get(), m_vertexBufferUpload.Get());

            // リソースバリア: VERTEX_AND_CONSTANT_BUFFER に遷移
            CD3DX12_RESOURCE_BARRIER barrierToVertex = CD3DX12_RESOURCE_BARRIER::Transition(
                m_vertexBuffer.Get(),
                D3D12_RESOURCE_STATE_COPY_DEST,
                D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER);
            m_commandList->ResourceBarrier(1, &barrierToVertex);

            OutputDebugStringA("LineBatcher: Vertex data uploaded to GPU\n");
        }

    } // namespace Graphics
} // namespace NeonVector