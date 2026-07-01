#include <NeonVector/Effects/Trail.h>
#include <NeonVector/Graphics/LineBatcher.h>

namespace NeonVector {
    namespace Effects {

        Trail::Trail(int maxPoints)
            : m_max(maxPoints < 2 ? 2 : maxPoints)
        {
        }

        void Trail::Push(const Vector2& p)
        {
            m_points.push_back(p);
            while (static_cast<int>(m_points.size()) > m_max)
                m_points.pop_front();
        }

        void Trail::Clear()
        {
            m_points.clear();
        }

        void Trail::SetMaxPoints(int n)
        {
            m_max = (n < 2) ? 2 : n;
            while (static_cast<int>(m_points.size()) > m_max)
                m_points.pop_front();
        }

        void Trail::Draw(Graphics::LineBatcher* batcher, const Color& color,
            float thickness, float glow) const
        {
            if (!batcher || m_points.size() < 2)
                return;

            const size_t n = m_points.size();
            for (size_t i = 0; i + 1 < n; ++i) {
                // t: 0 = 最も古い端, 1 = 最新端。新しいほど濃く太く。
                float t = static_cast<float>(i + 1) / static_cast<float>(n - 1);
                Color c = color;
                c.a = color.a * t;
                float th = thickness * (0.25f + 0.75f * t);
                batcher->AddLine(m_points[i], m_points[i + 1], c, th, glow * t);
            }
        }

    } // namespace Effects
} // namespace NeonVector
