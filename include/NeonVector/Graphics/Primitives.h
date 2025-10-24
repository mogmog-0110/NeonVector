#pragma once

#include <NeonVector/Math/Vector2.h>
#include <NeonVector/Core/Types.h>
#include <vector>

namespace NeonVector {
    namespace Graphics {

        class LineBatcher;

        /**
         * @brief ~`
         */
        void DrawCircle(LineBatcher* batcher,
            const Vector2& center,
            float radius,
            const Color& color,
            int segments = 32,
            float thickness = 1.0f);

        /**
         * @brief ``
         */
        void DrawRect(LineBatcher* batcher,
            const Vector2& topLeft,
            const Vector2& size,
            const Color& color,
            float thickness = 1.0f);

        /**
         * @brief p``
         */
        void DrawPolygon(LineBatcher* batcher,
            const std::vector<Vector2>& points,
            const Color& color,
            bool closed = true,
            float thickness = 1.0f);

    } // namespace Graphics
} // namespace NeonVector