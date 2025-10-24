// GaussianBlur_HighQuality.hlsl
// ガウシアンブラーシェーダー（高品質版）
// 9タップ版：より広範囲で滑らかなブラー

#include "GaussianBlur_Common.hlsli"

// ピクセルシェーダー（高品質版）
float4 PSMain(PSInput input) : SV_TARGET
{
    // 中心ピクセルのサンプリング
    float4 result = sourceTexture.Sample(samplerLinear, input.uv) * GAUSSIAN_WEIGHTS_9[0];
    
    // 方向に沿ってサンプリング（9タップ）
    [unroll]
    for (int i = 1; i < 9; i++)
    {
        float2 offset = direction * texelSize * i * blurRadius;
        
        // 正方向
        result += sourceTexture.Sample(samplerLinear, input.uv + offset) * GAUSSIAN_WEIGHTS_9[i];
        
        // 負方向
        result += sourceTexture.Sample(samplerLinear, input.uv - offset) * GAUSSIAN_WEIGHTS_9[i];
    }
    
    return result;
}
