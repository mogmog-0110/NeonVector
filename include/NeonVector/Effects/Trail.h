#pragma once

#include <NeonVector/Math/Vector2.h>
#include <NeonVector/Core/Types.h>
#include <deque>

namespace NeonVector {
    namespace Graphics { class LineBatcher; }

    namespace Effects {

        /**
         * @class Trail
         * @brief 動く物体の残光（コメット状のトレイル）。
         *
         * 直近の位置を貯め、新しい端ほど明るく・太く、古い端ほど薄く・細く描く。
         * 毎フレーム Push(現在位置) を呼び、Draw で線として描画する。bloom で発光する。
         */
        class Trail {
        public:
            explicit Trail(int maxPoints = 48);

            void Push(const Vector2& p);
            void Clear();

            void Draw(Graphics::LineBatcher* batcher, const Color& color,
                float thickness = 2.0f, float glow = 1.5f) const;

            void SetMaxPoints(int n);
            int  MaxPoints() const { return m_max; }
            size_t Size() const { return m_points.size(); }

        private:
            int m_max;
            std::deque<Vector2> m_points;   // front = 古い / back = 新しい
        };

    } // namespace Effects
} // namespace NeonVector
