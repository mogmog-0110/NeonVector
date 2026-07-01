#include <NeonVector/Graphics/Primitives.h>
#include <NeonVector/Graphics/LineBatcher.h>
#include <cmath>

namespace NeonVector {
    namespace Graphics {

        namespace { constexpr float kTwoPi = 6.28318530718f; }

        void DrawLine(LineBatcher* batcher,
            const Vector2& a, const Vector2& b,
            const Color& color, float thickness, float glow)
        {
            if (!batcher) return;
            batcher->AddLine(a, b, color, thickness, glow);
        }

        void DrawArc(LineBatcher* batcher,
            const Vector2& center, float radius,
            float startAngle, float endAngle, const Color& color,
            int segments, float thickness, float glow)
        {
            if (!batcher || segments < 1 || radius <= 0.0f) return;

            const float step = (endAngle - startAngle) / segments;
            Vector2 prev{
                center.x + std::cos(startAngle) * radius,
                center.y + std::sin(startAngle) * radius };
            for (int i = 1; i <= segments; ++i) {
                float a = startAngle + step * i;
                Vector2 cur{ center.x + std::cos(a) * radius,
                             center.y + std::sin(a) * radius };
                batcher->AddLine(prev, cur, color, thickness, glow);
                prev = cur;
            }
        }

        void DrawCircle(LineBatcher* batcher,
            const Vector2& center, float radius, const Color& color,
            int segments, float thickness, float glow)
        {
            if (segments < 3) return;
            DrawArc(batcher, center, radius, 0.0f, kTwoPi, color, segments, thickness, glow);
        }

        void DrawRect(LineBatcher* batcher,
            const Vector2& topLeft, const Vector2& size, const Color& color,
            float thickness, float glow)
        {
            if (!batcher) return;
            Vector2 tr{ topLeft.x + size.x, topLeft.y };
            Vector2 br{ topLeft.x + size.x, topLeft.y + size.y };
            Vector2 bl{ topLeft.x, topLeft.y + size.y };
            batcher->AddLine(topLeft, tr, color, thickness, glow);
            batcher->AddLine(tr, br, color, thickness, glow);
            batcher->AddLine(br, bl, color, thickness, glow);
            batcher->AddLine(bl, topLeft, color, thickness, glow);
        }

        void DrawPolygon(LineBatcher* batcher,
            const std::vector<Vector2>& points, const Color& color,
            bool closed, float thickness, float glow)
        {
            if (!batcher || points.size() < 2) return;
            for (size_t i = 0; i + 1 < points.size(); ++i)
                batcher->AddLine(points[i], points[i + 1], color, thickness, glow);
            if (closed && points.size() > 2)
                batcher->AddLine(points.back(), points.front(), color, thickness, glow);
        }

        void DrawRegularPolygon(LineBatcher* batcher,
            const Vector2& center, float radius, int sides, const Color& color,
            float rotation, float thickness, float glow)
        {
            if (!batcher || sides < 3 || radius <= 0.0f) return;
            const float step = kTwoPi / sides;
            Vector2 prev{ center.x + std::cos(rotation) * radius,
                          center.y + std::sin(rotation) * radius };
            for (int i = 1; i <= sides; ++i) {
                float a = rotation + step * i;
                Vector2 cur{ center.x + std::cos(a) * radius,
                             center.y + std::sin(a) * radius };
                batcher->AddLine(prev, cur, color, thickness, glow);
                prev = cur;
            }
        }

        void DrawStar(LineBatcher* batcher,
            const Vector2& center, float outerRadius, float innerRadius,
            int points, const Color& color,
            float rotation, float thickness, float glow)
        {
            if (!batcher || points < 2 || outerRadius <= 0.0f) return;
            const int verts = points * 2;
            const float step = kTwoPi / verts;
            Vector2 first{}, prev{};
            for (int i = 0; i < verts; ++i) {
                float r = (i % 2 == 0) ? outerRadius : innerRadius;
                float a = rotation + step * i;
                Vector2 cur{ center.x + std::cos(a) * r, center.y + std::sin(a) * r };
                if (i == 0) first = cur;
                else        batcher->AddLine(prev, cur, color, thickness, glow);
                prev = cur;
            }
            batcher->AddLine(prev, first, color, thickness, glow);
        }

        void DrawGrid(LineBatcher* batcher,
            const Vector2& topLeft, const Vector2& size, float cellSize,
            const Color& color, float thickness, float glow)
        {
            if (!batcher || cellSize <= 0.0f) return;
            const float right = topLeft.x + size.x;
            const float bottom = topLeft.y + size.y;
            for (float x = topLeft.x; x <= right + 0.5f; x += cellSize)
                batcher->AddLine({ x, topLeft.y }, { x, bottom }, color, thickness, glow);
            for (float y = topLeft.y; y <= bottom + 0.5f; y += cellSize)
                batcher->AddLine({ topLeft.x, y }, { right, y }, color, thickness, glow);
        }

    } // namespace Graphics
} // namespace NeonVector
