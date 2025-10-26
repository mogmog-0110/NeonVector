// BloomEffect.cpp
// Bloomエフェクトの完全な実装

#include <d3d12.h>
#include <d3dcompiler.h>
#include "../include/NeonVector/Effects/BloomEffect.h"
#include <stdexcept>
#include <fstream>
#include <vector>
#include <iostream>

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
                std::cerr << "BloomEffect: Invalid parameters" << std::endl;
                return false;
            }

            m_device = device;
            m_width = width;
            m_height = height;

            std::cout << "BloomEffect: Initializing..." << std::endl;

            // フルスクリーンクワッドの作成
            std::cout << "BloomEffect: Creating FullscreenQuad..." << std::endl;
            m_fullscreenQuad = std::make_unique<Graphics::FullscreenQuad>();
            if (!m_fullscreenQuad->Initialize(device)) {
                std::cerr << "BloomEffect: Failed to create FullscreenQuad" << std::endl;
                return false;
            }
            std::cout << "BloomEffect: FullscreenQuad created" << std::endl;

            // ========================================
            // デスクリプタヒープの作成
            // ========================================
            std::cout << "BloomEffect: Creating descriptor heaps..." << std::endl;

            // RTVヒープ（2つのRenderTarget用）
            D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
            rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
            rtvHeapDesc.NumDescriptors = 2;
            rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
            rtvHeapDesc.NodeMask = 0;

            HRESULT hr = device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_rtvHeap));
            if (FAILED(hr)) {
                std::cerr << "BloomEffect: Failed to create RTV heap" << std::endl;
                return false;
            }

            UINT rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

            // SRVヒープ（2つのRenderTarget用）
            D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
            srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
            srvHeapDesc.NumDescriptors = 2;
            srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
            srvHeapDesc.NodeMask = 0;

            hr = device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_srvHeap));
            if (FAILED(hr)) {
                std::cerr << "BloomEffect: Failed to create SRV heap" << std::endl;
                return false;
            }

            UINT srvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
            std::cout << "BloomEffect: Descriptor heaps created" << std::endl;

            // ========================================
            // 中間レンダーターゲットの作成
            // ========================================
            std::cout << "BloomEffect: Creating render targets..." << std::endl;

            const uint32_t bloomWidth = width / 4;
            const uint32_t bloomHeight = height / 4;
            std::cout << "BloomEffect: Bloom resolution: " << bloomWidth << "x" << bloomHeight << std::endl;

            // m_brightRT の作成
            m_brightRT = std::make_unique<Graphics::RenderTarget>();
            if (!m_brightRT->Initialize(device, bloomWidth, bloomHeight)) {
                std::cerr << "BloomEffect: Failed to create m_brightRT" << std::endl;
                return false;
            }

            // m_brightRT にRTVとSRVを設定
            {
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();

                D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
                rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = 0;

                device->CreateRenderTargetView(m_brightRT->GetResource(), &rtvDesc, rtvHandle);
                m_brightRT->SetRTVHandle(rtvHandle);

                D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
                D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Texture2D.MipLevels = 1;
                srvDesc.Texture2D.MostDetailedMip = 0;

                device->CreateShaderResourceView(m_brightRT->GetResource(), &srvDesc, srvCpuHandle);
                m_brightRT->SetSRVHandle(srvCpuHandle, srvGpuHandle);
            }
            std::cout << "BloomEffect: m_brightRT created" << std::endl;

            // m_blurTempRT の作成
            m_blurTempRT = std::make_unique<Graphics::RenderTarget>();
            if (!m_blurTempRT->Initialize(device, bloomWidth, bloomHeight)) {
                std::cerr << "BloomEffect: Failed to create m_blurTempRT" << std::endl;
                return false;
            }

            // m_blurTempRT にRTVとSRVを設定
            {
                D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
                rtvHandle.ptr += rtvDescriptorSize;

                D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
                rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
                rtvDesc.Texture2D.MipSlice = 0;

                device->CreateRenderTargetView(m_blurTempRT->GetResource(), &rtvDesc, rtvHandle);
                m_blurTempRT->SetRTVHandle(rtvHandle);

                D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHandle = m_srvHeap->GetCPUDescriptorHandleForHeapStart();
                srvCpuHandle.ptr += srvDescriptorSize;

                D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHandle = m_srvHeap->GetGPUDescriptorHandleForHeapStart();
                srvGpuHandle.ptr += srvDescriptorSize;

                D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
                srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
                srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                srvDesc.Texture2D.MipLevels = 1;
                srvDesc.Texture2D.MostDetailedMip = 0;

                device->CreateShaderResourceView(m_blurTempRT->GetResource(), &srvDesc, srvCpuHandle);
                m_blurTempRT->SetSRVHandle(srvCpuHandle, srvGpuHandle);
            }
            std::cout << "BloomEffect: m_blurTempRT created" << std::endl;

            // ルートシグネチャの作成
            std::cout << "BloomEffect: Creating root signature..." << std::endl;
            if (!CreateRootSignature(device)) {
                std::cerr << "BloomEffect: Failed to create root signature" << std::endl;
                return false;
            }
            std::cout << "BloomEffect: Root signature created" << std::endl;

            // パイプラインステートの作成
            std::cout << "BloomEffect: Creating pipeline states..." << std::endl;
            if (!CreatePipelineStates(device)) {
                std::cerr << "BloomEffect: Failed to create pipeline states" << std::endl;
                return false;
            }
            std::cout << "BloomEffect: Pipeline states created" << std::endl;

            // 定数バッファの作成
            std::cout << "BloomEffect: Creating constant buffers..." << std::endl;
            if (!CreateConstantBuffers(device)) {
                std::cerr << "BloomEffect: Failed to create constant buffers" << std::endl;
                return false;
            }
            std::cout << "BloomEffect: Constant buffers created" << std::endl;

            std::cout << "BloomEffect: Initialization complete!" << std::endl;
            return true;
        }

        void BloomEffect::Shutdown() {
            m_brightPassPSO.Reset();
            m_blurPSO.Reset();
            m_compositePSO.Reset();
            m_rootSignature.Reset();
            m_bloomParamsBuffer.Reset();
            m_blurParamsBuffer.Reset();

            m_rtvHeap.Reset();
            m_srvHeap.Reset();

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
            std::cout << "  Creating descriptor ranges..." << std::endl;

            // ディスクリプタレンジ
            D3D12_DESCRIPTOR_RANGE ranges[2] = {};

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

            std::cout << "  Creating root parameters..." << std::endl;

            // ルートパラメータ
            D3D12_ROOT_PARAMETER rootParams[3] = {};

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

            // [2] テクスチャ (t1) - 合成時のみ使用
            rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
            rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
            rootParams[2].DescriptorTable.pDescriptorRanges = &ranges[1];
            rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            std::cout << "  Creating static sampler..." << std::endl;

            // スタティックサンプラー (s0)
            D3D12_STATIC_SAMPLER_DESC samplerDesc = {};
            samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
            samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
            samplerDesc.MipLODBias = 0.0f;
            samplerDesc.MaxAnisotropy = 1;
            samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
            samplerDesc.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_BLACK;
            samplerDesc.MinLOD = 0.0f;
            samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;
            samplerDesc.ShaderRegister = 0;  // s0
            samplerDesc.RegisterSpace = 0;
            samplerDesc.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

            std::cout << "  Creating root signature desc..." << std::endl;

            // ルートシグネチャディスクリプタ
            D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
            rootSigDesc.NumParameters = 3;
            rootSigDesc.pParameters = rootParams;
            rootSigDesc.NumStaticSamplers = 1;
            rootSigDesc.pStaticSamplers = &samplerDesc;
            rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

            std::cout << "  Serializing root signature..." << std::endl;

            // シリアライズ
            ComPtr<ID3DBlob> signature;
            ComPtr<ID3DBlob> error;

            HRESULT hr = D3D12SerializeRootSignature(
                &rootSigDesc,
                D3D_ROOT_SIGNATURE_VERSION_1,
                &signature,
                &error
            );

            if (FAILED(hr)) {
                std::cerr << "  Failed to serialize root signature, HRESULT: 0x"
                    << std::hex << hr << std::dec << std::endl;
                if (error) {
                    std::cerr << "  Error message: "
                        << (char*)error->GetBufferPointer() << std::endl;
                }
                return false;
            }

            std::cout << "  Creating root signature object..." << std::endl;

            // ルートシグネチャ作成
            hr = device->CreateRootSignature(
                0,
                signature->GetBufferPointer(),
                signature->GetBufferSize(),
                IID_PPV_ARGS(&m_rootSignature)
            );

            if (FAILED(hr)) {
                std::cerr << "  Failed to create root signature object, HRESULT: 0x"
                    << std::hex << hr << std::dec << std::endl;
                return false;
            }

            std::cout << "  Root signature created successfully" << std::endl;
            return true;
        }

        bool BloomEffect::CreatePipelineStates(ID3D12Device* device) {
            std::cout << "  Loading shaders..." << std::endl;

            // シェーダーの読み込み
            std::vector<uint8_t> vsBloom, psBrightPass, psBlur, psComposite;

            std::cout << "    Loading Bloom_VSMain.cso..." << std::endl;
            if (!LoadShader("shaders/Bloom_VSMain.cso", vsBloom)) {
                std::cerr << "    Failed to load Bloom_VSMain.cso" << std::endl;
                return false;
            }
            std::cout << "    Loaded: " << vsBloom.size() << " bytes" << std::endl;

            std::cout << "    Loading Bloom_PSBrightPass.cso..." << std::endl;
            if (!LoadShader("shaders/Bloom_PSBrightPass.cso", psBrightPass)) {
                std::cerr << "    Failed to load Bloom_PSBrightPass.cso" << std::endl;
                return false;
            }
            std::cout << "    Loaded: " << psBrightPass.size() << " bytes" << std::endl;

            std::cout << "    Loading GaussianBlur_PSMain.cso..." << std::endl;
            if (!LoadShader("shaders/GaussianBlur_PSMain.cso", psBlur)) {
                std::cerr << "    Failed to load GaussianBlur_PSMain.cso" << std::endl;
                return false;
            }
            std::cout << "    Loaded: " << psBlur.size() << " bytes" << std::endl;

            std::cout << "    Loading Bloom_PSComposite.cso..." << std::endl;
            if (!LoadShader("shaders/Bloom_PSComposite.cso", psComposite)) {
                std::cerr << "    Failed to load Bloom_PSComposite.cso" << std::endl;
                return false;
            }
            std::cout << "    Loaded: " << psComposite.size() << " bytes" << std::endl;

            std::cout << "  All shaders loaded successfully" << std::endl;

            // 入力レイアウト（FullscreenQuadの頂点フォーマット）
            D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
            };

            // ========================================
            // 1. BrightPass PSO
            // ========================================
            {
                D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
                psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
                psoDesc.pRootSignature = m_rootSignature.Get();
                psoDesc.VS = { vsBloom.data(), vsBloom.size() };
                psoDesc.PS = { psBrightPass.data(), psBrightPass.size() };
                psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
                psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
                psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
                psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
                psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
                psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
                psoDesc.RasterizerState.DepthClipEnable = TRUE;
                psoDesc.RasterizerState.MultisampleEnable = FALSE;
                psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
                psoDesc.RasterizerState.ForcedSampleCount = 0;
                psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
                psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
                psoDesc.BlendState.IndependentBlendEnable = FALSE;
                psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
                psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
                psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
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

            // ========================================
            // 2. Blur PSO
            // ========================================
            {
                D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
                psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
                psoDesc.pRootSignature = m_rootSignature.Get();
                psoDesc.VS = { vsBloom.data(), vsBloom.size() };
                psoDesc.PS = { psBlur.data(), psBlur.size() };
                psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
                psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
                psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
                psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
                psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
                psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
                psoDesc.RasterizerState.DepthClipEnable = TRUE;
                psoDesc.RasterizerState.MultisampleEnable = FALSE;
                psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
                psoDesc.RasterizerState.ForcedSampleCount = 0;
                psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
                psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
                psoDesc.BlendState.IndependentBlendEnable = FALSE;
                psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
                psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
                psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
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

            // ========================================
            // 3. Composite PSO
            // ========================================
            {
                D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
                psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
                psoDesc.pRootSignature = m_rootSignature.Get();
                psoDesc.VS = { vsBloom.data(), vsBloom.size() };
                psoDesc.PS = { psComposite.data(), psComposite.size() };
                psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
                psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
                psoDesc.RasterizerState.FrontCounterClockwise = FALSE;
                psoDesc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
                psoDesc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
                psoDesc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
                psoDesc.RasterizerState.DepthClipEnable = TRUE;
                psoDesc.RasterizerState.MultisampleEnable = FALSE;
                psoDesc.RasterizerState.AntialiasedLineEnable = FALSE;
                psoDesc.RasterizerState.ForcedSampleCount = 0;
                psoDesc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
                psoDesc.BlendState.AlphaToCoverageEnable = FALSE;
                psoDesc.BlendState.IndependentBlendEnable = FALSE;
                psoDesc.BlendState.RenderTarget[0].BlendEnable = FALSE;
                psoDesc.BlendState.RenderTarget[0].LogicOpEnable = FALSE;
                psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
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
            // sourceRTを RENDER_TARGET → PIXEL_SHADER_RESOURCE に遷移
            sourceRT->TransitionTo(commandList,
                D3D12_RESOURCE_STATE_RENDER_TARGET,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

            // brightRTを PIXEL_SHADER_RESOURCE → RENDER_TARGET に遷移
            brightRT->TransitionTo(commandList,
                D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE,
                D3D12_RESOURCE_STATE_RENDER_TARGET);

            // レンダーターゲット設定
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = brightRT->GetRTVHandle();
            commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

            // ビューポートとシザー矩形
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

            // パイプラインステートとルートシグネチャ設定
            commandList->SetPipelineState(m_brightPassPSO.Get());
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());

            // ディスクリプタヒープを設定
            ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
            commandList->SetDescriptorHeaps(1, heaps);

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

            // 描画
            m_fullscreenQuad->Draw(commandList);

            // brightRTを RENDER_TARGET → PIXEL_SHADER_RESOURCE に遷移
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

                // ★★★ ディスクリプタヒープを設定 ★★★
                ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
                commandList->SetDescriptorHeaps(1, heaps);

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

                // ディスクリプタヒープを設定
                ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
                commandList->SetDescriptorHeaps(1, heaps);

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

            // レンダーターゲット設定
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = outputRT->GetRTVHandle();
            commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

            // ビューポートとシザー矩形
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

            // クリア
            outputRT->Clear(commandList);

            // パイプラインステートとルートシグネチャ設定
            commandList->SetPipelineState(m_compositePSO.Get());
            commandList->SetGraphicsRootSignature(m_rootSignature.Get());

            // ディスクリプタヒープを設定
            ID3D12DescriptorHeap* heaps[] = { m_srvHeap.Get() };
            commandList->SetDescriptorHeaps(1, heaps);

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
            commandList->SetGraphicsRootDescriptorTable(1, sceneRT->GetGPUSRVHandle());   // t0: 元のシーン
            commandList->SetGraphicsRootDescriptorTable(2, bloomRT->GetGPUSRVHandle());   // t1: Bloom

            // 描画
            m_fullscreenQuad->Draw(commandList);

            // 出力を RENDER_TARGET → PIXEL_SHADER_RESOURCE に遷移
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
                std::string("../../../") + filepath,
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
