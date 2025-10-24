#include <NeonVector/Graphics/Primitives.h>
#include <NeonVector/Graphics/LineBatcher.h>
#include <cmath>

namespace NeonVector {
    namespace Graphics {

        void DrawCircle(LineBatcher* batcher,
            const Vector2& center,
            float radius,
            const Color& color,
            int segments,
            float thickness)
        {
            if (!batcher || segments < 3) {
                return;
            }

            const float angleStep = (2.0f * 3.14159265f) / segments;

            for (int i = 0; i < segments; i++) {
                float angle1 = i * angleStep;
                float angle2 = (i + 1) * angleStep;

                Vector2 p1 = {
                    center.x + std::cos(angle1) * radius,
                    center.y + std::sin(angle1) * radius
                };

                Vector2 p2 = {
                    center.x + std::cos(angle2) * radius,
                    center.y + std::sin(angle2) * radius
                };

                batcher->AddLine(p1, p2, color, thickness);
            }
        }

        void DrawRect(LineBatcher* batcher,
            const Vector2& topLeft,
            const Vector2& size,
            const Color& color,
            float thickness)
        {
            if (!batcher) {
                return;
            }

            Vector2 topRight = { topLeft.x + size.x, topLeft.y };
            Vector2 bottomRight = { topLeft.x + size.x, topLeft.y + size.y };
            Vector2 bottomLeft = { topLeft.x, topLeft.y + size.y };

            batcher->AddLine(topLeft, topRight, color, thickness);
            batcher->AddLine(topRight, bottomRight, color, thickness);
            batcher->AddLine(bottomRight, bottomLeft, color, thickness);
            batcher->AddLine(bottomLeft, topLeft, color, thickness);
        }

        void DrawPolygon(LineBatcher* batcher,
            const std::vector<Vector2>& points,
            const Color& color,
            bool closed,
            float thickness)
        {
            if (!batcher || points.size() < 2) {
                return;
            }

            for (size_t i = 0; i < points.size() - 1; i++) {
                batcher->AddLine(points[i], points[i + 1], color, thickness);
            }

            if (closed && points.size() > 2) {
                batcher->AddLine(points.back(), points.front(), color, thickness);
            }
        }

    } // namespace Graphics
} // namespace NeonVector