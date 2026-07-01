#pragma once

#include <NeonVector/Math/Vector2.h>
#include <NeonVector/Core/Types.h>
#include <vector>
#include <random>

namespace NeonVector {
    namespace Graphics { class LineBatcher; }

    namespace Effects {

        struct Particle {
            Vector2 pos;
            Vector2 vel;
            float life;      // 残り寿命（秒）
            float maxLife;   // 初期寿命
            float size;
            Color color;
        };

        /**
         * @class ParticleSystem
         * @brief ネオンの発光パーティクル。爆発・軌跡・スパークに使う。
         *
         * Emit で放射状にばら撒き、Update で移動・減衰・寿命処理、Draw で速度方向の
         * 短い発光ストリークとして描く（bloom で光る）。
         */
        class ParticleSystem {
        public:
            ParticleSystem();

            /** @brief pos から count 個を放射状に放出（速度は min..max、寿命 life 秒） */
            void Emit(const Vector2& pos, int count,
                float minSpeed, float maxSpeed,
                const Color& color, float life, float size = 3.0f);

            void Update(float dt);
            void Draw(Graphics::LineBatcher* batcher, float glow = 1.5f) const;
            void Clear();

            size_t Count() const { return m_particles.size(); }
            void SetGravity(float g) { m_gravity = g; }   // +で下方向(画面座標)
            void SetDrag(float d) { m_drag = d; }         // 毎秒残す速度割合(1=減衰なし)

        private:
            std::vector<Particle> m_particles;
            float m_gravity = 0.0f;
            float m_drag = 0.6f;
            std::mt19937 m_rng;
        };

    } // namespace Effects
} // namespace NeonVector
