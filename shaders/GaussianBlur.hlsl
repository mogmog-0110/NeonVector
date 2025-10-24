// GaussianBlur.hlsl
// ガウシアンブラーシェーダー
// 5タップ版：高速だが範囲は狭い

#include "GaussianBlur_Common.hlsli"

// ピクセルシェーダー
float4 PSMain(PSInput input) : SV_TARGET
{
    // 中心ピクセルのサンプリング
    float4 result = sourceTexture.Sample(samplerLinear, input.uv) * GAUSSIAN_WEIGHTS[0];
    
    // 方向に沿ってサンプリング
    // direction = (1, 0) なら水平方向
    // direction = (0, 1) なら垂直方向
    
    [unroll]
    for (int i = 1; i < 5; i++)
    {
        float2 offset = direction * texelSize * i * blurRadius;
        
        // 正方向
        result += sourceTexture.Sample(samplerLinear, input.uv + offset) * GAUSSIAN_WEIGHTS[i];
        
        // 負方向
        result += sourceTexture.Sample(samplerLinear, input.uv - offset) * GAUSSIAN_WEIGHTS[i];
    }
    
    return result;
}
