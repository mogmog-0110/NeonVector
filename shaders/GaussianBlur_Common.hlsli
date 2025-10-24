// GaussianBlur_Common.hlsli
// ガウシアンブラー用の共通定義

// 定数バッファ
cbuffer BlurParameters : register(b0)
{
    float2 texelSize;      // 1ピクセルのUVサイズ (1/width, 1/height)
    float2 direction;      // ブラー方向 (1,0)=水平, (0,1)=垂直
    float blurRadius;      // ブラー半径（ピクセル単位）
    float padding[3];      // 16バイトアライメント用
};

// テクスチャとサンプラー
Texture2D sourceTexture : register(t0);
SamplerState samplerLinear : register(s0);

// 入力・出力構造体
struct VSInput
{
    float3 position : POSITION;
    float2 uv : TEXCOORD0;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float2 uv : TEXCOORD0;
};

// 頂点シェーダー（FullscreenQuad用）
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 1.0f);
    output.uv = input.uv;
    return output;
}

// ガウシアン重み計算
// σ(シグマ) = 1.0の場合の5タップブラー用の重み
static const float GAUSSIAN_WEIGHTS[5] = {
    0.227027,   // 中心
    0.1945946,  // ±1
    0.1216216,  // ±2
    0.054054,   // ±3
    0.016216    // ±4
};

// より広範囲のブラー用（9タップ）
static const float GAUSSIAN_WEIGHTS_9[9] = {
    0.147761,   // 中心
    0.144533,   // ±1
    0.135335,   // ±2
    0.120985,   // ±3
    0.102520,   // ±4
    0.081521,   // ±5
    0.060774,   // ±6
    0.042393,   // ±7
    0.027325    // ±8
};
