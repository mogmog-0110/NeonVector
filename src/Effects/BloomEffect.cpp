// BloomEffect.cpp
// Bloomエフェクトの完全な実装

#include "../include/NeonVector/Effects/BloomEffect.h"
#include <stdexcept>
#include <fstream>
#include <vector>

namespace NeonVector {
    namespace Effects {

        BloomEffect::BloomEffect()
            : m_device(nullptr)
            , m_width(0)
            , m_height(0)
            , m_threshold(1.0f)
            , m_intensity(1.5f)
            , m_bloomStrength(1.0f)
            , m_blurRadius(2.0f)
        {
        }

        BloomEffect::~BloomEffect() {
            Shutdown();
        }

        bool BloomEffect::Initialize(ID3D12Device* device, uint32_t width, uint32_t height) {
            if (!device || width == 0 || height == 0) {
                return false;
            }

            m_device = device;
            m_width = width;
            m_height = height;

            // フルスクリーンクワッドの作成
            m_fullscreenQuad = std::make_unique<Graphics::FullscreenQuad>();
            if (!m_fullscreenQuad->Initialize(device)) {
                return false;
            }

            // 中間レンダーターゲットの作成
            // Bloomは1/4解像度で処理（高速化）
            const uint32_t bloomWidth = width / 4;
            const uint32_t bloomHeight = height / 4;

            m_brightRT = std::make_unique<Graphics::RenderTarget>();
            if (!m_brightRT->Initialize(device, bloomWidth, bloomHeight)) {
                return false;
            }

            m_blurTempRT = std::make_unique<Graphics::RenderTarget>();
            if (!m_blurTempRT->Initialize(device, bloomWidth, bloomHeight)) {
                return false;
            }

            // ルートシグネチャの作成
            if (!CreateRootSignature(device)) {
                return false;
            }

            // パイプラインステートの作成
            if (!CreatePipelineStates(device)) {
                return false;
            }

            // 定数バッファの作成
            if (!CreateConstantBuffers(device)) {
                return false;
            }

            return true;
        }

        void BloomEffect::Shutdown() {
            m_brightPassPSO.Reset();
            m_blurPSO.Reset();
            m_compositePSO.Reset();
            m_rootSignature.Reset();
            m_bloomParamsBuffer.Reset();
            m_blurParamsBuffer.Reset();

            m_fullscreenQuad.reset();
            m_brightRT.reset();
            m_blurTempRT.reset();

            m_device = nullptr;
            m_width = 0;
            m_height = 0;
        }

        void BloomEffect::Apply(
            ID3D12GraphicsCommandList* commandList,
            Graphics::RenderTarget* sourceRT,
            Graphics::RenderTarget* outputRT)
        {
            if (!commandList || !sourceRT || !outputRT) {
                return;
            }

            // Bloomパイプライン
            // 輝度抽出: sourceRT → brightRT
            BrightPass(commandList, sourceRT, m_brightRT.get());

            // ブラー: brightRT → blurTempRT → brightRT
            BlurPass(commandList, m_brightRT.get(), m_blurTempRT.get(), m_brightRT.get());

            // 合成: sourceRT + brightRT → outputRT
            CompositePass(commandList, sourceRT, m_brightRT.get(), outputRT);
        }

        // ========================================
        // プライベート関数の実装
        // ========================================

        bool BloomEffect::CreateRootSignature(ID3D12Device* device) {
            // ルートパラメータ定義
            // [0] 定数バッファ (b0)
            // [1] テクスチャ (t0)
            // [2] テクスチャ (t1) - 合成時のみ使用
            // [3] サンプラー (s0)

            D3D12_DESCRIPTOR_RANGE ranges[3] = {};

            // テクスチャ1 (t0)
            ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            ranges[0].NumDescriptors = 1;
            ranges[0].BaseShaderRegister = 0;
            ranges[0].RegisterSpace = 0;
            ranges[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            // テクスチャ2 (t1) - 合成用
            ranges[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
            ranges[1].NumDescriptors = 1;
            ranges[1].BaseShaderRegister = 1;
            ranges[1].RegisterSpace = 0;
            ranges[1].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            // サンプラー (s0)
            ranges[2].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
            ranges[2].NumDescriptors = 1;
            ranges[2].BaseShaderRegister = 0;
            ranges[2].RegisterSpace = 0;
            ranges[2].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

            D3D12_ROOT_PARAMETER rootParams[4] = {};

            // [0] 定数バッファ (b0)
            rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
            rootParams[0].Descriptor.ShaderRegister = 0;
            rootParams[0].Descriptor.RegisterSpace = 0;
            rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            // [1] テクスチャ (t0)
            rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
            rootParams[1].DescriptorTable.pDescriptorRanges = &ranges[0];
            rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            // [2] テクスチャ (t1)
            rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
            rootParams[2].DescriptorTable.pDescriptorRanges = &ranges[1];
            rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            // [3] サンプラー (s0)
            rootParams[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParams[3].DescriptorTable.NumDescriptorRanges = 1;
            rootParams[3].DescriptorTable.pDescriptorRanges = &ranges[2];
            rootParams[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
            rootSigDesc.NumParameters = 4;
            rootSigDesc.pParameters = rootParams;
            rootSigDesc.NumStaticSamplers = 0;
            rootSigDesc.pStaticSamplers = nullptr;
            rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> error;

            HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error);
            if (FAILED(hr)) {
                if (error) {
                    OutputDebugStringA((char*)error->GetBufferPointer());
                }
                return false;
            }

            hr = device->CreateRootSignature(
                0,
                signature->GetBufferPointer(),
                signature->GetBufferSize(),
                IID_PPV_ARGS(&m_rootSignature)
            );

            return SUCCEEDED(hr);
        }

        bool BloomEffect::CreatePipelineStates(ID3D12Device* device) {
            // シェーダーの読み込み
            std::vector<uint8_t> vsBloom, psBrightPass, psBlur, psComposite;

            if (!LoadShader("shaders/Bloom_VSMain.cso", vsBloom)) {
                return false;
            }
            if (!LoadShader("shaders/Bloom_PSBrightPass.cso", psBrightPass)) {
                return false;
            }
            if (!LoadShader("shaders/GaussianBlur_PSMain.cso", psBlur)) {
                return false;
            }
            if (!LoadShader("shaders/Bloom_PSComposite.cso", psComposite)) {
                return false;
            }

            // 入力レイアウト（FullscreenQuad用）
            D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            // ラスタライザーステート
            D3D12_RASTERIZER_DESC rasterizerDesc = {};
            rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
            rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
            rasterizerDesc.FrontCounterClockwise = FALSE;
            rasterizerDesc.DepthBias = 0;
            rasterizerDesc.DepthBiasClamp = 0.0f;
            rasterizerDesc.SlopeScaledDepthBias = 0.0f;
            rasterizerDesc.DepthClipEnable = TRUE;
            rasterizerDesc.MultisampleEnable = FALSE;
            rasterizerDesc.AntialiasedLineEnable = FALSE;
            rasterizerDesc.ForcedSampleCount = 0;
            rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

            // ブレンドステート（加算合成なし）
            D3D12_BLEND_DESC blendDesc = {};
            blendDesc.AlphaToCoverageEnable = FALSE;
            blendDesc.IndependentBlendEnable = FALSE;
            blendDesc.RenderTarget[0].BlendEnable = FALSE;
            blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

            // 1. BrightPass PSO
            {
                D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
                psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
                psoDesc.pRootSignature = m_rootSignature.Get();
                psoDesc.VS = { vsBloom.data(), vsBloom.size() };
                psoDesc.PS = { psBrightPass.data(), psBrightPass.size() };
                psoDesc.RasterizerState = rasterizerDesc;
                psoDesc.BlendState = blendDesc;
                psoDesc.DepthStencilState.DepthEnable = FALSE;
                psoDesc.DepthStencilState.StencilEnable = FALSE;
                psoDesc.SampleMask = UINT_MAX;
                psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                psoDesc.NumRenderTargets = 1;
                psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
                psoDesc.SampleDesc.Count = 1;

                HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_brightPassPSO));
                if (FAILED(hr)) {
                    return false;
                }
            }

            // 2. Blur PSO
            {
                D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
                psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
                psoDesc.pRootSignature = m_rootSignature.Get();
                psoDesc.VS = { vsBloom.data(), vsBloom.size() };
                psoDesc.PS = { psBlur.data(), psBlur.size() };
                psoDesc.RasterizerState = rasterizerDesc;
                psoDesc.BlendState = blendDesc;
                psoDesc.DepthStencilState.DepthEnable = FALSE;
                psoDesc.DepthStencilState.StencilEnable = FALSE;
                psoDesc.SampleMask = UINT_MAX;
                psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                psoDesc.NumRenderTargets = 1;
                psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
                psoDesc.SampleDesc.Count = 1;

                HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_blurPSO));
                if (FAILED(hr)) {
                    return false;
                }
            }

            // 3. Composite PSO
            {
                D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
                psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
                psoDesc.pRootSignature = m_rootSignature.Get();
                psoDesc.VS = { vsBloom.data(), vsBloom.size() };
                psoDesc.PS = { psComposite.data(), psComposite.size() };
                psoDesc.RasterizerState = rasterizerDesc;
                psoDesc.BlendState = blendDesc;
                psoDesc.DepthStencilState.DepthEnable = FALSE;
                psoDesc.DepthStencilState.StencilEnable = FALSE;
                psoDesc.SampleMask = UINT_MAX;
                psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
                psoDesc.NumRenderTargets = 1;
                psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
                psoDesc.SampleDesc.Count = 1;

                HRESULT hr = device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_compositePSO));
                if (FAILED(hr)) {
                    return false;
                }
            }

            return true;
        }

        bool BloomEffect::CreateConstantBuffers(ID3D12Device* device) {
            // Bloomパラメータ用定数バッファ
            const uint32_t bloomBufferSize = (sizeof(BloomParams) + 255) & ~255; // 256バイトアライメント

            D3D12_HEAP_PROPERTIES heapProps = {};
            heapProps.Type = D3D12_HEAP_TYPE_UPLOAD;
            heapProps.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
            heapProps.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
            heapProps.CreationNodeMask = 1;
            heapProps.VisibleNodeMask = 1;

            D3D12_RESOURCE_DESC bufferDesc = {};
            bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
            bufferDesc.Width = bloomBufferSize;
            bufferDesc.Height = 1;
            bufferDesc.DepthOrArraySize = 1;
            bufferDesc.MipLevels = 1;
            bufferDesc.Format = DXGI_FORMAT_UNKNOWN;
            bufferDesc.SampleDesc.Count = 1;
            bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
            bufferDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

            HRESULT hr = device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_bloomParamsBuffer)
            );

            if (FAILED(hr)) {
                return false;
            }

            // ブラーパラメータ用定数バッファ
            const uint32_t blurBufferSize = (sizeof(BlurParams) + 255) & ~255;

            bufferDesc.Width = blurBufferSize;
            hr = device->CreateCommittedResource(
                &heapProps,
                D3D12_HEAP_FLAG_NONE,
                &bufferDesc,
                D3D12_RESOURCE_STATE_GENERIC_READ,
                nullptr,
                IID_PPV_ARGS(&m_blurParamsBuffer)
            );

            return SUCCEEDED(hr);
        }

        void BloomEffect::BrightPass(
            ID3D12GraphicsCommandList* commandList,
            Graphics::RenderTarget* sourceRT,
            Graphics::RenderTarget* brightRT)
        {
            // sourceRT を Shader Resource に遷移
            sourceRT->TransitionTo(commandList,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            // brightRT を Render Target に遷移
            brightRT->TransitionTo(commandList,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET);

            // レンダーターゲット設定
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = brightRT->GetRTVHandle();
            commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

            // ビューポート設定
            D3D12_VIEWPORT viewport = {};
            viewport.Width = static_cast<float>(brightRT->GetWidth());
            viewport.Height = static_cast<float>(brightRT->GetHeight());
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            commandList->RSSetViewports(1, &viewport);

            D3D12_RECT scissorRect = { 0, 0,
                static_cast<LONG>(brightRT->GetWidth()),
                static_cast<LONG>(brightRT->GetHeight()) };
            commandList->RSSetScissorRects(1, &scissorRect);

            // クリア
            brightRT->Clear(commandList);

            // パイプライン設定
            commandList->SetPipelineState(m_brightPassPSO.Get());
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());

            // 定数バッファ更新
            BloomParams params = {};
            params.threshold = m_threshold;
            params.intensity = m_intensity;
            params.bloomStrength = m_bloomStrength;
            params.padding = 0.0f;

            void* mapped = nullptr;
            m_bloomParamsBuffer->Map(0, nullptr, &mapped);
            memcpy(mapped, &params, sizeof(params));
            m_bloomParamsBuffer->Unmap(0, nullptr);

            // ルートパラメータ設定
            commandList->SetGraphicsRootConstantBufferView(0, m_bloomParamsBuffer->GetGPUVirtualAddress());
            commandList->SetGraphicsRootDescriptorTable(1, sourceRT->GetGPUSRVHandle());
            // サンプラーは省略（デフォルトで動作するはず）

            // 描画
            m_fullscreenQuad->Draw(commandList);

            // brightRT を Shader Resource に遷移
            brightRT->TransitionTo(commandList,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }

        void BloomEffect::BlurPass(
            ID3D12GraphicsCommandList* commandList,
            Graphics::RenderTarget* sourceRT,
            Graphics::RenderTarget* tempRT,
            Graphics::RenderTarget* outputRT)
        {
            // 水平ブラー: sourceRT → tempRT
            {
                sourceRT->TransitionTo(commandList,
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

                tempRT->TransitionTo(commandList,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_RENDER_TARGET);

                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = tempRT->GetRTVHandle();
                commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

                D3D12_VIEWPORT viewport = {};
                viewport.Width = static_cast<float>(tempRT->GetWidth());
                viewport.Height = static_cast<float>(tempRT->GetHeight());
                viewport.MinDepth = 0.0f;
                viewport.MaxDepth = 1.0f;
                commandList->RSSetViewports(1, &viewport);

                D3D12_RECT scissorRect = { 0, 0,
                    static_cast<LONG>(tempRT->GetWidth()),
                    static_cast<LONG>(tempRT->GetHeight()) };
                commandList->RSSetScissorRects(1, &scissorRect);

                tempRT->Clear(commandList);

                commandList->SetPipelineState(m_blurPSO.Get());
                commandList->SetGraphicsRootSignature(m_rootSignature.Get());

                // 定数バッファ更新（水平方向）
                BlurParams params = {};
                params.texelSize[0] = 1.0f / sourceRT->GetWidth();
                params.texelSize[1] = 1.0f / sourceRT->GetHeight();
                params.direction[0] = 1.0f;  // 水平
                params.direction[1] = 0.0f;
                params.blurRadius = m_blurRadius;

                void* mapped = nullptr;
                m_blurParamsBuffer->Map(0, nullptr, &mapped);
                memcpy(mapped, &params, sizeof(params));
                m_blurParamsBuffer->Unmap(0, nullptr);

                commandList->SetGraphicsRootConstantBufferView(0, m_blurParamsBuffer->GetGPUVirtualAddress());
                commandList->SetGraphicsRootDescriptorTable(1, sourceRT->GetGPUSRVHandle());

                m_fullscreenQuad->Draw(commandList);

                tempRT->TransitionTo(commandList,
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            }

            // 垂直ブラー: tempRT → outputRT
            {
                outputRT->TransitionTo(commandList,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                    D3D12_RESOURCE_STATE_RENDER_TARGET);

                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = outputRT->GetRTVHandle();
                commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

                D3D12_VIEWPORT viewport = {};
                viewport.Width = static_cast<float>(outputRT->GetWidth());
                viewport.Height = static_cast<float>(outputRT->GetHeight());
                viewport.MinDepth = 0.0f;
                viewport.MaxDepth = 1.0f;
                commandList->RSSetViewports(1, &viewport);

                D3D12_RECT scissorRect = { 0, 0,
                    static_cast<LONG>(outputRT->GetWidth()),
                    static_cast<LONG>(outputRT->GetHeight()) };
                commandList->RSSetScissorRects(1, &scissorRect);

                outputRT->Clear(commandList);

                // 定数バッファ更新（垂直方向）
                BlurParams params = {};
                params.texelSize[0] = 1.0f / tempRT->GetWidth();
                params.texelSize[1] = 1.0f / tempRT->GetHeight();
                params.direction[0] = 0.0f;
                params.direction[1] = 1.0f;  // 垂直
                params.blurRadius = m_blurRadius;

                void* mapped = nullptr;
                m_blurParamsBuffer->Map(0, nullptr, &mapped);
                memcpy(mapped, &params, sizeof(params));
                m_blurParamsBuffer->Unmap(0, nullptr);

                commandList->SetGraphicsRootConstantBufferView(0, m_blurParamsBuffer->GetGPUVirtualAddress());
                commandList->SetGraphicsRootDescriptorTable(1, tempRT->GetGPUSRVHandle());

                m_fullscreenQuad->Draw(commandList);

                outputRT->TransitionTo(commandList,
                    D3D12_RESOURCE_STATE_RENDER_TARGET,
                    D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            }
        }

        void BloomEffect::CompositePass(
            ID3D12GraphicsCommandList* commandList,
            Graphics::RenderTarget* sceneRT,
            Graphics::RenderTarget* bloomRT,
            Graphics::RenderTarget* outputRT)
        {
            // 両方の入力を Shader Resource に
            sceneRT->TransitionTo(commandList,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            bloomRT->TransitionTo(commandList,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            // 出力を Render Target に
            outputRT->TransitionTo(commandList,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET);

            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = outputRT->GetRTVHandle();
            commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

            D3D12_VIEWPORT viewport = {};
            viewport.Width = static_cast<float>(outputRT->GetWidth());
            viewport.Height = static_cast<float>(outputRT->GetHeight());
            viewport.MinDepth = 0.0f;
            viewport.MaxDepth = 1.0f;
            commandList->RSSetViewports(1, &viewport);

            D3D12_RECT scissorRect = { 0, 0,
                static_cast<LONG>(outputRT->GetWidth()),
                static_cast<LONG>(outputRT->GetHeight()) };
            commandList->RSSetScissorRects(1, &scissorRect);

            outputRT->Clear(commandList);

            commandList->SetPipelineState(m_compositePSO.Get());
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());

            // 定数バッファ更新
            BloomParams params = {};
            params.threshold = m_threshold;
            params.intensity = m_intensity;
            params.bloomStrength = m_bloomStrength;
            params.padding = 0.0f;

            void* mapped = nullptr;
            m_bloomParamsBuffer->Map(0, nullptr, &mapped);
            memcpy(mapped, &params, sizeof(params));
            m_bloomParamsBuffer->Unmap(0, nullptr);

            commandList->SetGraphicsRootConstantBufferView(0, m_bloomParamsBuffer->GetGPUVirtualAddress());
            commandList->SetGraphicsRootDescriptorTable(1, sceneRT->GetGPUSRVHandle());
            commandList->SetGraphicsRootDescriptorTable(2, bloomRT->GetGPUSRVHandle());

            m_fullscreenQuad->Draw(commandList);

            outputRT->TransitionTo(commandList,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        }

        bool BloomEffect::LoadShader(const char* filepath, std::vector<uint8_t>& outData) {
            // 複数のパス候補を試す
            std::vector<std::string> searchPaths = {
                filepath,
                std::string("../") + filepath,
                std::string("../../") + filepath,
            };

            for (const auto& path : searchPaths) {
                std::ifstream file(path, std::ios::binary | std::ios::ate);
                if (file.is_open()) {
                    size_t fileSize = static_cast<size_t>(file.tellg());
                    file.seekg(0, std::ios::beg);

                    outData.resize(fileSize);
                    file.read(reinterpret_cast<char*>(outData.data()), fileSize);
                    file.close();

                    return true;
                }
            }

            // デバッグ出力
            OutputDebugStringA("Failed to load shader: ");
            OutputDebugStringA(filepath);
            OutputDebugStringA("\n");

            return false;
        }

    } // namespace Effects
} // namespace NeonVector
