#pragma once

#include <NeonVector/Math/Vector2.h>
#include <NeonVector/Core/Types.h>
#include <vector>

namespace NeonVector {
    namespace Graphics {

        class LineBatcher;

        // すべての図形は線で描く（ベクターグラフィックス）。
        // glow は BloomEffect が拾う発光強度（1.0 = 標準、大きいほど強く光る）。

        /** @brief 2 点を結ぶ線（AddLine の薄いラッパ、記述統一用） */
        void DrawLine(LineBatcher* batcher,
            const Vector2& a, const Vector2& b,
            const Color& color, float thickness = 1.0f, float glow = 1.0f);

        /** @brief 円 */
        void DrawCircle(LineBatcher* batcher,
            const Vector2& center, float radius, const Color& color,
            int segments = 32, float thickness = 1.0f, float glow = 1.0f);

        /** @brief 円弧（startAngle→endAngle, ラジアン） */
        void DrawArc(LineBatcher* batcher,
            const Vector2& center, float radius,
            float startAngle, float endAngle, const Color& color,
            int segments = 32, float thickness = 1.0f, float glow = 1.0f);

        /** @brief 矩形（左上 + サイズ） */
        void DrawRect(LineBatcher* batcher,
            const Vector2& topLeft, const Vector2& size, const Color& color,
            float thickness = 1.0f, float glow = 1.0f);

        /** @brief 任意頂点列（closed で始点終点を結ぶ = 折れ線 or 多角形） */
        void DrawPolygon(LineBatcher* batcher,
            const std::vector<Vector2>& points, const Color& color,
            bool closed = true, float thickness = 1.0f, float glow = 1.0f);

        /** @brief 正多角形（中心 + 外接半径 + 辺数, rotation ラジアン） */
        void DrawRegularPolygon(LineBatcher* batcher,
            const Vector2& center, float radius, int sides, const Color& color,
            float rotation = 0.0f, float thickness = 1.0f, float glow = 1.0f);

        /** @brief 星形（外半径 + 内半径 + 頂点数） */
        void DrawStar(LineBatcher* batcher,
            const Vector2& center, float outerRadius, float innerRadius,
            int points, const Color& color,
            float rotation = 0.0f, float thickness = 1.0f, float glow = 1.0f);

        /** @brief グリッド（矩形領域 topLeft+size を cellSize 間隔で分割） */
        void DrawGrid(LineBatcher* batcher,
            const Vector2& topLeft, const Vector2& size, float cellSize,
            const Color& color, float thickness = 1.0f, float glow = 1.0f);

    } // namespace Graphics
} // namespace NeonVector
