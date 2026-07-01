#include <NeonVector/Effects/ParticleSystem.h>
#include <NeonVector/Graphics/LineBatcher.h>
#include <cmath>

namespace NeonVector {
    namespace Effects {

        namespace { constexpr float kTwoPi = 6.28318530718f; }

        ParticleSystem::ParticleSystem()
            : m_rng(std::random_device{}())
        {
        }

        void ParticleSystem::Emit(const Vector2& pos, int count,
            float minSpeed, float maxSpeed,
            const Color& color, float life, float size)
        {
            std::uniform_real_distribution<float> angleDist(0.0f, kTwoPi);
            std::uniform_real_distribution<float> speedDist(minSpeed, maxSpeed);
            std::uniform_real_distribution<float> lifeDist(0.7f, 1.0f);

            m_particles.reserve(m_particles.size() + count);
            for (int i = 0; i < count; ++i) {
                float a = angleDist(m_rng);
                float s = speedDist(m_rng);
                float lf = life * lifeDist(m_rng);
                Particle p;
                p.pos = pos;
                p.vel = { std::cos(a) * s, std::sin(a) * s };
                p.life = lf;
                p.maxLife = lf;
                p.size = size;
                p.color = color;
                m_particles.push_back(p);
            }
        }

        void ParticleSystem::Update(float dt)
        {
            const float velScale = std::pow(m_drag, dt);
            for (auto& p : m_particles) {
                p.pos = p.pos + p.vel * dt;
                p.vel = p.vel * velScale;
                p.vel.y += m_gravity * dt;
                p.life -= dt;
            }
            // 寿命切れを除去
            size_t w = 0;
            for (size_t r = 0; r < m_particles.size(); ++r)
                if (m_particles[r].life > 0.0f)
                    m_particles[w++] = m_particles[r];
            m_particles.resize(w);
        }

        void ParticleSystem::Draw(Graphics::LineBatcher* batcher, float glow) const
        {
            if (!batcher) return;
            for (const auto& p : m_particles) {
                float t = (p.maxLife > 0.0f) ? (p.life / p.maxLife) : 0.0f;  // 1→0 で消える
                Color c = p.color;
                c.a = p.color.a * t;

                float speed = p.vel.Length();
                Vector2 half;
                if (speed > 1e-3f)
                    half = p.vel * (p.size * 0.5f / speed);   // 速度方向に長さ size
                else
                    half = { p.size * 0.5f, 0.0f };
                batcher->AddLine(p.pos - half, p.pos + half, c, p.size * 0.6f, glow);
            }
        }

        void ParticleSystem::Clear()
        {
            m_particles.clear();
        }

    } // namespace Effects
} // namespace NeonVector
