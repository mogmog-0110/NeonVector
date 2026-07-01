// 05_NeonPlayground/main.cpp
// エンジン機能の実演: 拡張プリミティブ + Trail(マウス追従コメット) +
// ParticleSystem(クリックで発光噴射) + 入力 API + Bloom。
//
// 操作: マウス移動=コメット / 左ドラッグ=パーティクル噴射 / 1-4=色切替 /
//       Space=中央でバースト / Esc=終了

#include <NeonVector/NeonVector.h>
#include <NeonVector/Effects/BloomEffect.h>

#include <cmath>
#include <memory>
#include <iostream>

using namespace NeonVector;

class NeonPlayground : public Application
{
public:
    NeonPlayground() : Application(ApplicationConfig{ "NeonVector - Neon Playground", 1280, 720, true, false }) {}

    void OnInit() override
    {
        Application::OnInit();
        m_bloom = std::make_unique<Effects::BloomEffect>();
        if (!m_bloom->Initialize(GetDevice(), m_config.width, m_config.height)) {
            std::cerr << "Bloom init failed; running without bloom" << std::endl;
            m_bloom.reset();
        } else {
            m_bloom->SetThreshold(0.7f);
            m_bloom->SetIntensity(1.6f);
            m_bloom->SetBloomStrength(1.3f);
            m_bloom->SetBlurRadius(2.5f);
        }
        m_particles.SetDrag(0.4f);   // すぐ減速して火花っぽく
        m_color = Color::Cyan;
    }

    void OnUpdate(float dt) override
    {
        m_time += dt;
        m_rot += dt * 0.6f;

        // 色切替
        if (WasKeyPressed('1')) m_color = Color::Cyan;
        if (WasKeyPressed('2')) m_color = Color::Magenta;
        if (WasKeyPressed('3')) m_color = Color{ 0.4f, 1.0f, 0.5f, 1.0f };
        if (WasKeyPressed('4')) m_color = Color{ 1.0f, 0.6f, 0.2f, 1.0f };

        // マウス追従コメット
        Vector2 mouse = GetMousePosition();
        m_trail.Push(mouse);

        // 左ドラッグでパーティクル噴射
        if (IsMouseButtonDown(0))
            m_particles.Emit(mouse, 5, 60.0f, 280.0f, m_color, 0.7f, 3.0f);

        // Space で中央から大きめのバースト
        if (WasKeyPressed(VK_SPACE))
            m_particles.Emit({ m_config.width / 2.0f, m_config.height / 2.0f },
                120, 120.0f, 520.0f, m_color, 1.1f, 3.5f);

        m_particles.Update(dt);
    }

    void OnRender() override
    {
        auto* b = GetLineBatcher();
        if (!b) return;

        const float cx = m_config.width / 2.0f;
        const float cy = m_config.height / 2.0f;
        Color dim{ m_color.r, m_color.g, m_color.b, 0.15f };

        // 背景グリッド（薄い）
        Graphics::DrawGrid(b, { 0, 0 }, { (float)m_config.width, (float)m_config.height }, 64.0f, dim, 1.0f, 0.3f);

        // 中央: 回転する星・多角形・円弧（拡張プリミティブの実演）
        Graphics::DrawStar(b, { cx, cy }, 150.0f, 62.0f, 5, m_color, m_rot, 2.5f, 1.6f);
        Graphics::DrawRegularPolygon(b, { cx, cy }, 210.0f, 6, m_color, -m_rot * 0.5f, 2.0f, 1.2f);
        Graphics::DrawCircle(b, { cx, cy }, 40.0f + std::sin(m_time * 3.0f) * 10.0f, m_color, 32, 2.0f, 1.4f);
        float a0 = m_rot * 1.5f;
        Graphics::DrawArc(b, { cx, cy }, 260.0f, a0, a0 + 2.2f, m_color, 24, 3.0f, 1.5f);

        // マウスコメット + パーティクル
        m_trail.Draw(b, m_color, 3.0f, 2.0f);
        m_particles.Draw(b, 1.6f);

        b->Flush();

        if (m_bloom) {
            auto* rt = GetCurrentRenderTarget();
            if (rt) m_bloom->Apply(GetCommandList(), rt, rt);
        }
    }

private:
    std::unique_ptr<Effects::BloomEffect> m_bloom;
    Effects::Trail m_trail{ 40 };
    Effects::ParticleSystem m_particles;
    Color m_color;
    float m_time = 0.0f;
    float m_rot = 0.0f;
};

int main()
{
    NeonPlayground app;
    return app.Run();
}
