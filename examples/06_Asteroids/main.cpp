// 06_Asteroids/main.cpp
// NeonVector で作る「ネオン Asteroids」。エンジンが実ゲームを作れることの実証:
// 入力・拡張プリミティブ・Trail・ParticleSystem・Bloom を総動員。
//
// 操作: ←/→ or A/D=旋回, ↑ or W=推進, Space=射撃, R=リスタート, Esc=終了

#include <NeonVector/NeonVector.h>
#include <NeonVector/Effects/BloomEffect.h>

#include <cmath>
#include <memory>
#include <vector>
#include <random>
#include <iostream>

using namespace NeonVector;
using Graphics::LineBatcher;

namespace {
    constexpr float kPi = 3.14159265f;
    constexpr float kTwoPi = 6.28318530718f;
    int   g_W = 1280, g_H = 720;

    Vector2 fromAngle(float a) { return { std::cos(a), std::sin(a) }; }
    Vector2 rotate(const Vector2& v, float a) {
        float c = std::cos(a), s = std::sin(a);
        return { v.x * c - v.y * s, v.x * s + v.y * c };
    }
    float dist(const Vector2& a, const Vector2& b) { return (a - b).Length(); }
    void wrap(Vector2& p) {
        if (p.x < 0) p.x += g_W; else if (p.x >= g_W) p.x -= g_W;
        if (p.y < 0) p.y += g_H; else if (p.y >= g_H) p.y -= g_H;
    }
}

struct Ship {
    Vector2 pos, vel;
    float angle = -kPi / 2.0f;
    bool alive = true;
    float invuln = 0.0f;     // 無敵残り秒（点滅＋当たり無効）
    float respawn = 0.0f;    // >0 の間は復活待ち
};

struct Bullet { Vector2 pos, vel; float life; };

struct Asteroid {
    Vector2 pos, vel;
    float radius, angle, spin;
    int tier;                    // 3=大 2=中 1=小
    std::vector<float> shape;    // 各頂点の半径倍率（ゴツゴツ感）
};

class NeonAsteroids : public Application
{
public:
    NeonAsteroids() : Application(ApplicationConfig{ "NeonVector - Asteroids", 1280, 720, true, false }),
        m_rng(std::random_device{}()) {}

    void OnInit() override
    {
        Application::OnInit();
        g_W = m_config.width; g_H = m_config.height;
        m_bloom = std::make_unique<Effects::BloomEffect>();
        if (!m_bloom->Initialize(GetDevice(), m_config.width, m_config.height)) {
            std::cerr << "Bloom init failed; running without bloom" << std::endl;
            m_bloom.reset();
        } else {
            m_bloom->SetThreshold(0.65f);
            m_bloom->SetIntensity(1.6f);
            m_bloom->SetBloomStrength(1.3f);
            m_bloom->SetBlurRadius(2.5f);
        }
        m_particles.SetDrag(0.5f);
        startGame();
    }

    void OnUpdate(float dt) override
    {
        if (dt > 0.05f) dt = 0.05f;   // スパイク抑制
        m_time += dt;

        if (m_gameOver) {
            if (WasKeyPressed('R') || WasKeyPressed(VK_SPACE)) startGame();
            m_particles.Update(dt);
            return;
        }

        updateShip(dt);
        updateBullets(dt);
        updateAsteroids(dt);
        m_particles.Update(dt);
        checkCollisions();

        if (m_asteroids.empty()) nextWave();
    }

    void OnRender() override
    {
        auto* b = GetLineBatcher();
        if (!b) return;

        // 背景グリッド（ごく薄く）
        Graphics::DrawGrid(b, { 0,0 }, { (float)g_W,(float)g_H }, 80.0f, Color{ 0.15f,0.35f,0.5f,0.12f }, 1.0f, 0.25f);

        drawAsteroids(b);
        drawBullets(b);
        if (m_ship.alive) drawShip(b);
        m_trail.Draw(b, Color{ 0.4f,0.9f,1.0f,1.0f }, 3.0f, 2.0f);
        m_particles.Draw(b, 1.6f);
        drawHud(b);

        b->Flush();
        if (m_bloom) { auto* rt = GetCurrentRenderTarget(); if (rt) m_bloom->Apply(GetCommandList(), rt, rt); }
    }

private:
    // ── ゲーム進行 ──
    void startGame()
    {
        m_score = 0; m_lives = 3; m_wave = 0; m_gameOver = false;
        m_bullets.clear(); m_asteroids.clear(); m_particles.Clear(); m_trail.Clear();
        resetShip();
        nextWave();
    }
    void resetShip()
    {
        m_ship = Ship{};
        m_ship.pos = { g_W / 2.0f, g_H / 2.0f };
        m_ship.vel = { 0,0 };
        m_ship.angle = -kPi / 2.0f;
        m_ship.alive = true;
        m_ship.invuln = 2.0f;
    }
    void nextWave()
    {
        ++m_wave;
        int count = 3 + m_wave;
        for (int i = 0; i < count; ++i) {
            // 中央（自機付近）を避けて出す
            Vector2 p;
            do { p = { randf(0, (float)g_W), randf(0, (float)g_H) }; } while (dist(p, m_ship.pos) < 220.0f);
            spawnAsteroid(p, 3);
        }
    }
    void spawnAsteroid(const Vector2& pos, int tier)
    {
        Asteroid a;
        a.pos = pos;
        float speed = randf(30.0f, 70.0f) + m_wave * 5.0f + (3 - tier) * 25.0f;
        float dir = randf(0, kTwoPi);
        a.vel = fromAngle(dir) * speed;
        a.tier = tier;
        a.radius = (tier == 3) ? 56.0f : (tier == 2) ? 32.0f : 17.0f;
        a.angle = randf(0, kTwoPi);
        a.spin = randf(-1.2f, 1.2f);
        int verts = 10 + (m_rng() % 3);
        a.shape.reserve(verts);
        for (int i = 0; i < verts; ++i) a.shape.push_back(randf(0.72f, 1.18f));
        m_asteroids.push_back(std::move(a));
    }

    // ── 更新 ──
    void updateShip(float dt)
    {
        if (!m_ship.alive) {
            m_ship.respawn -= dt;
            if (m_ship.respawn <= 0.0f) resetShip();
            return;
        }
        if (m_ship.invuln > 0.0f) m_ship.invuln -= dt;

        if (IsKeyDown(VK_LEFT) || IsKeyDown('A'))  m_ship.angle -= 3.6f * dt;
        if (IsKeyDown(VK_RIGHT) || IsKeyDown('D')) m_ship.angle += 3.6f * dt;

        bool thrusting = IsKeyDown(VK_UP) || IsKeyDown('W');
        if (thrusting) {
            m_ship.vel = m_ship.vel + fromAngle(m_ship.angle) * (330.0f * dt);
            // 尾から炎パーティクル＋トレイル
            Vector2 tail = m_ship.pos - fromAngle(m_ship.angle) * 16.0f;
            m_trail.Push(tail);
            if (m_thrustEmit <= 0.0f) {
                m_particles.Emit(tail, 3, 40.0f, 160.0f, Color{ 1.0f,0.6f,0.2f,1.0f }, 0.4f, 2.5f);
                m_thrustEmit = 0.02f;
            }
        } else {
            m_trail.Push(m_ship.pos);   // 慣性の細い尾
        }
        m_thrustEmit -= dt;

        m_ship.vel = m_ship.vel * std::pow(0.6f, dt);   // 慣性減衰
        m_ship.pos = m_ship.pos + m_ship.vel * dt;
        wrap(m_ship.pos);

        // 射撃（クールダウン付き）
        m_fireCd -= dt;
        if (IsKeyDown(VK_SPACE) && m_fireCd <= 0.0f) {
            Vector2 dir = fromAngle(m_ship.angle);
            Bullet bt; bt.pos = m_ship.pos + dir * 18.0f;
            bt.vel = dir * 640.0f + m_ship.vel;
            bt.life = 1.05f;
            m_bullets.push_back(bt);
            m_fireCd = 0.16f;
        }
    }
    void updateBullets(float dt)
    {
        for (auto& b : m_bullets) { b.pos = b.pos + b.vel * dt; wrap(b.pos); b.life -= dt; }
        eraseDead(m_bullets, [](const Bullet& b) { return b.life <= 0.0f; });
    }
    void updateAsteroids(float dt)
    {
        for (auto& a : m_asteroids) { a.pos = a.pos + a.vel * dt; wrap(a.pos); a.angle += a.spin * dt; }
    }
    void checkCollisions()
    {
        // 弾 vs 小惑星
        for (auto& bt : m_bullets) {
            for (size_t i = 0; i < m_asteroids.size(); ++i) {
                if (dist(bt.pos, m_asteroids[i].pos) < m_asteroids[i].radius) {
                    bt.life = 0.0f;
                    splitAsteroid(i);
                    break;
                }
            }
        }
        eraseDead(m_bullets, [](const Bullet& b) { return b.life <= 0.0f; });

        // 自機 vs 小惑星
        if (m_ship.alive && m_ship.invuln <= 0.0f) {
            for (auto& a : m_asteroids) {
                if (dist(m_ship.pos, a.pos) < a.radius + 12.0f) { killShip(); break; }
            }
        }
    }
    void splitAsteroid(size_t idx)
    {
        Asteroid a = m_asteroids[idx];   // copy
        m_asteroids.erase(m_asteroids.begin() + idx);
        m_score += (a.tier == 3) ? 20 : (a.tier == 2) ? 50 : 100;
        m_particles.Emit(a.pos, 18 + (3 - a.tier) * 6, 60.0f, 320.0f,
            Color{ 0.7f,0.9f,1.0f,1.0f }, 0.7f, 3.0f);
        if (a.tier > 1) {
            spawnAsteroid(a.pos, a.tier - 1);
            spawnAsteroid(a.pos, a.tier - 1);
        }
    }
    void killShip()
    {
        m_particles.Emit(m_ship.pos, 60, 80.0f, 380.0f, Color{ 0.4f,0.9f,1.0f,1.0f }, 1.0f, 3.5f);
        m_ship.alive = false;
        m_trail.Clear();
        --m_lives;
        if (m_lives <= 0) m_gameOver = true;
        else m_ship.respawn = 1.4f;
    }

    // ── 描画 ──
    void drawShip(LineBatcher* b)
    {
        // 無敵中は点滅
        if (m_ship.invuln > 0.0f && std::fmod(m_time, 0.2f) < 0.1f) return;
        Color c{ 0.4f, 0.95f, 1.0f, 1.0f };
        Vector2 p0 = m_ship.pos + rotate({ 18.0f, 0.0f }, m_ship.angle);
        Vector2 p1 = m_ship.pos + rotate({ -12.0f, 11.0f }, m_ship.angle);
        Vector2 p2 = m_ship.pos + rotate({ -6.0f, 0.0f }, m_ship.angle);
        Vector2 p3 = m_ship.pos + rotate({ -12.0f, -11.0f }, m_ship.angle);
        b->AddLine(p0, p1, c, 2.5f, 1.5f);
        b->AddLine(p1, p2, c, 2.5f, 1.5f);
        b->AddLine(p2, p3, c, 2.5f, 1.5f);
        b->AddLine(p3, p0, c, 2.5f, 1.5f);
    }
    void drawAsteroids(LineBatcher* b)
    {
        Color c{ 0.75f, 0.85f, 1.0f, 1.0f };
        for (const auto& a : m_asteroids) {
            const int n = (int)a.shape.size();
            const float step = kTwoPi / n;
            Vector2 prev{};
            for (int i = 0; i <= n; ++i) {
                float ang = a.angle + step * (i % n);
                Vector2 pt = a.pos + fromAngle(ang) * (a.radius * a.shape[i % n]);
                if (i > 0) b->AddLine(prev, pt, c, 2.0f, 1.2f);
                prev = pt;
            }
        }
    }
    void drawBullets(LineBatcher* b)
    {
        Color c{ 1.0f, 1.0f, 0.5f, 1.0f };
        for (const auto& bt : m_bullets) {
            Vector2 d = bt.vel; float len = d.Length();
            Vector2 h = (len > 1e-3f) ? d * (6.0f / len) : Vector2{ 6.0f,0.0f };
            b->AddLine(bt.pos - h, bt.pos + h, c, 2.5f, 2.0f);
        }
    }
    void drawHud(LineBatcher* b)
    {
        Color c{ 0.5f, 1.0f, 0.9f, 1.0f };
        // スコア（右上・7セグ風）
        drawNumber(b, m_score, g_W - 30.0f, 24.0f, 22.0f, 36.0f, 6.0f, c);
        // 残機（左上・小さな自機アイコン）
        for (int i = 0; i < m_lives; ++i) {
            Vector2 o{ 30.0f + i * 34.0f, 40.0f };
            b->AddLine(o + Vector2{ 0,-12 }, o + Vector2{ -9,9 }, c, 2.0f, 1.2f);
            b->AddLine(o + Vector2{ -9,9 }, o + Vector2{ 9,9 }, c, 2.0f, 1.2f);
            b->AddLine(o + Vector2{ 9,9 }, o + Vector2{ 0,-12 }, c, 2.0f, 1.2f);
        }
        // ウェーブ表示（下）
        drawNumber(b, m_wave, 52.0f, (float)g_H - 40.0f, 14.0f, 22.0f, 4.0f, Color{ 0.4f,0.7f,1.0f,0.8f });

        if (m_gameOver) {
            // 中央に大きく X 印っぽいバツ＋スコアで「終了」を示す（フォント無しの割り切り）
            Color r{ 1.0f, 0.35f, 0.45f, 1.0f };
            float cx = g_W / 2.0f, cy = g_H / 2.0f;
            b->AddLine({ cx - 60, cy - 60 }, { cx + 60, cy + 60 }, r, 4.0f, 2.0f);
            b->AddLine({ cx + 60, cy - 60 }, { cx - 60, cy + 60 }, r, 4.0f, 2.0f);
            drawNumber(b, m_score, cx + 130.0f, cy - 24.0f, 30.0f, 48.0f, 8.0f, r);
        }
    }

    // 7 セグメント数字（フォント代わり）。x は右端、右詰めで描く。
    void drawDigit(LineBatcher* b, int d, float x, float y, float w, float h, const Color& c)
    {
        // segs: a b c d e f g
        static const bool tbl[10][7] = {
            {1,1,1,1,1,1,0},{0,1,1,0,0,0,0},{1,1,0,1,1,0,1},{1,1,1,1,0,0,1},
            {0,1,1,0,0,1,1},{1,0,1,1,0,1,1},{1,0,1,1,1,1,1},{1,1,1,0,0,0,0},
            {1,1,1,1,1,1,1},{1,1,1,1,0,1,1}
        };
        if (d < 0 || d > 9) return;
        const bool* s = tbl[d];
        float x0 = x, x1 = x + w, y0 = y, ym = y + h / 2, y1 = y + h;
        float th = 2.5f, gl = 1.3f;
        if (s[0]) b->AddLine({ x0,y0 }, { x1,y0 }, c, th, gl);
        if (s[1]) b->AddLine({ x1,y0 }, { x1,ym }, c, th, gl);
        if (s[2]) b->AddLine({ x1,ym }, { x1,y1 }, c, th, gl);
        if (s[3]) b->AddLine({ x0,y1 }, { x1,y1 }, c, th, gl);
        if (s[4]) b->AddLine({ x0,ym }, { x0,y1 }, c, th, gl);
        if (s[5]) b->AddLine({ x0,y0 }, { x0,ym }, c, th, gl);
        if (s[6]) b->AddLine({ x0,ym }, { x1,ym }, c, th, gl);
    }
    void drawNumber(LineBatcher* b, int value, float rightX, float y, float w, float h, float gap, const Color& c)
    {
        if (value < 0) value = 0;
        float x = rightX - w;
        int v = value;
        do {
            drawDigit(b, v % 10, x, y, w, h, c);
            x -= (w + gap);
            v /= 10;
        } while (v > 0);
    }

    template<class T, class Pred>
    static void eraseDead(std::vector<T>& v, Pred dead)
    {
        size_t w = 0;
        for (size_t r = 0; r < v.size(); ++r) if (!dead(v[r])) v[w++] = v[r];
        v.resize(w);
    }
    float randf(float lo, float hi) { std::uniform_real_distribution<float> d(lo, hi); return d(m_rng); }

private:
    std::unique_ptr<Effects::BloomEffect> m_bloom;
    Effects::ParticleSystem m_particles;
    Effects::Trail m_trail{ 24 };
    Ship m_ship;
    std::vector<Bullet> m_bullets;
    std::vector<Asteroid> m_asteroids;
    std::mt19937 m_rng;

    int m_score = 0, m_lives = 3, m_wave = 0;
    bool m_gameOver = false;
    float m_time = 0.0f, m_fireCd = 0.0f, m_thrustEmit = 0.0f;
};

int main()
{
    NeonAsteroids app;
    return app.Run();
}
