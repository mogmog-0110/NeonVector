// Bloom.hlsl
// Bloomエフェクト用シェーダー
// 1. 輝度抽出パス: 明るい部分だけを取り出す
// 2. 合成パス: 元のシーンとBloomを合成

// 定数バッファ
cbuffer BloomParameters : register(b0)
{
    float threshold;       // 輝度閾値（この値以上の明るさを抽出）
    float intensity;       // Bloom強度
    float bloomStrength;   // Bloom合成時の強さ
    float padding;         // 16バイトアライメント用
};

// テクスチャとサンプラー
Texture2D sceneTexture : register(t0);   // 元のシーンテクスチャ
Texture2D bloomTexture : register(t1);   // ぼかしたBloomテクスチャ
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

// 頂点シェーダー
PSInput VSMain(VSInput input)
{
    PSInput output;
    output.position = float4(input.position, 1.0f);
    output.uv = input.uv;
    return output;
}

// ========================================
// 輝度抽出パス
// ========================================

// 輝度計算（人間の視覚に基づいた重み付け）
// R: 0.299, G: 0.587, B: 0.114
float Luminance(float3 color)
{
    return dot(color, float3(0.299, 0.587, 0.114));
}

// 輝度抽出ピクセルシェーダー
float4 PSBrightPass(PSInput input) : SV_TARGET
{
    // シーンからカラーをサンプリング
    float4 color = sceneTexture.Sample(samplerLinear, input.uv);
    
    // 輝度を計算
    float brightness = Luminance(color.rgb);
    
    // 閾値以上の明るさだけを残す
    if (brightness > threshold)
    {
        // 閾値を超えた分を強調
        float excess = brightness - threshold;
        return color * (excess * intensity);
    }
    
    // 暗い部分は黒に
    return float4(0.0, 0.0, 0.0, 1.0);
}

// ========================================
// 合成パス
// ========================================

// Bloom合成ピクセルシェーダー
float4 PSComposite(PSInput input) : SV_TARGET
{
    // 元のシーン
    float4 sceneColor = sceneTexture.Sample(samplerLinear, input.uv);
    
    // Bloom（ぼかした明るい部分）
    float4 bloomColor = bloomTexture.Sample(samplerLinear, input.uv);
    
    // 加算合成
    // bloomStrength で Bloom の影響度を調整
    float4 result = sceneColor + bloomColor * bloomStrength;
    
    // アルファ値は元のまま
    result.a = sceneColor.a;
    
    return result;
}

// ========================================
// シンプルなコピーパス（デバッグ用）
// ========================================

// テクスチャをそのままコピー
float4 PSCopy(PSInput input) : SV_TARGET
{
    return sceneTexture.Sample(samplerLinear, input.uv);
}

// ========================================
// ダウンサンプリング/アップサンプリング
// ========================================

// ダウンサンプリング（4x4領域の平均を取る）
float4 PSDownsample(PSInput input) : SV_TARGET
{
    // テクセルサイズ
    float2 texelSize = float2(1.0 / 1920.0, 1.0 / 1080.0);  // TODO: 動的に設定
    
    // 4x4領域をサンプリング
    float4 result = float4(0, 0, 0, 0);
    
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            float2 offset = float2(x, y) * texelSize;
            result += sceneTexture.Sample(samplerLinear, input.uv + offset);
        }
    }
    
    // 平均を取る（9サンプル）
    return result / 9.0;
}

// アップサンプリング（バイリニア補間）
float4 PSUpsample(PSInput input) : SV_TARGET
{
    // GPUのバイリニア補間を利用
    return sceneTexture.Sample(samplerLinear, input.uv);
}
