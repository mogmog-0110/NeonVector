/**
 * @file Line.hlsl
 * @brief 線描画用のHLSLシェーダー
 * @author shiggy.
 * @date 2025-10-24
 * 
 * 頂点シェーダー：スクリーン座標をNDC座標に変換
 * ピクセルシェーダー：色を出力
 */

// 定数バッファ：スクリーンサイズ
cbuffer ScreenConstants : register(b0) {
    float g_screenWidth;
    float g_screenHeight;
};

// 頂点シェーダーへの入力
struct VSInput {
    float2 position : POSITION;      // スクリーン座標 (0,0)=左上
    float4 color : COLOR;            // RGBA色
    float thickness : THICKNESS;     // 線の太さ（将来の拡張用）
    float glow : GLOW;              // 発光強度（将来の拡張用）
};

// ピクセルシェーダーへの入力
struct PSInput {
    float4 position : SV_POSITION;   // NDC座標（クリップ空間）
    float4 color : COLOR;            // RGBA色
    float thickness : THICKNESS;
    float glow : GLOW;
};

/**
 * @brief 頂点シェーダーのメイン関数
 * 
 * スクリーン座標をNDC（Normalized Device Coordinates）に変換
 * スクリーン座標: (0,0)=左上, (width,height)=右下
 * NDC座標: (-1,1)=左上, (1,-1)=右下
 */
PSInput VSMain(VSInput input) {
    PSInput output;
    
    // スクリーン座標 → NDC座標への変換
    // X: [0, width] → [-1, 1]
    // Y: [0, height] → [1, -1] (上下反転)
    float x = (input.position.x / g_screenWidth) * 2.0f - 1.0f;
    float y = -((input.position.y / g_screenHeight) * 2.0f - 1.0f);
    
    output.position = float4(x, y, 0.0f, 1.0f);
    output.color = input.color;
    output.thickness = input.thickness;
    output.glow = input.glow;
    
    return output;
}

/**
 * @brief ピクセルシェーダーのメイン関数
 * 
 * 現時点では単純に色を出力
 * 将来的にはアンチエイリアスやグローエフェクトを追加予定
 */
float4 PSMain(PSInput input) : SV_TARGET {
    // 現時点では単純に色を返す
    return input.color;
    
    // 将来の実装:
    // - グロー強度に基づいた発光効果
    // - アンチエイリアス
    // - 線の太さに応じた描画
}
