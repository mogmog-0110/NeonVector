// 04_BloomDemo/main.cpp

#include <NeonVector/Core/Application.h>
#include <NeonVector/Core/Types.h>
#include <NeonVector/Graphics/LineBatcher.h>
#include <NeonVector/Graphics/Primitives.h>
#include <NeonVector/Math/Vector2.h>

#include <NeonVector/Effects/BloomEffect.h>
#include <NeonVector/Graphics/PostProcessManager.h>

#include <cmath>
#include <memory>
#include <iostream>

using namespace NeonVector;

class BloomDemo : public Application
{
private:
    float m_rotation = 0.0f;
    float m_time = 0.0f;

    std::unique_ptr<Graphics::PostProcessManager> m_postProcessManager;
    std::unique_ptr<Effects::BloomEffect> m_bloomEffect;
    bool m_bloomEnabled = true;

public:
    BloomDemo() : Application(ApplicationConfig{
        "NeonVector - Bloom Demo",
        1280,
        720,
        true,
        false
        })
    {
    }

    void OnInit() override
    {
        // 親クラスの初期化
        Application::OnInit();

        // PostProcessManagerの初期化
        m_postProcessManager = std::make_unique<Graphics::PostProcessManager>();
        if (!m_postProcessManager->Initialize(
            GetDevice(),
            m_config.width,
            m_config.height))
        {
            std::cerr << "Failed to initialize PostProcessManager" << std::endl;
            m_postProcessManager.reset();  // 初期化失敗時はリセット
        }

        // BloomEffectの初期化
        m_bloomEffect = std::make_unique<Effects::BloomEffect>();
        if (!m_bloomEffect->Initialize(
            GetDevice(),
            m_config.width,
            m_config.height))
        {
            std::cerr << "Failed to initialize BloomEffect" << std::endl;
            m_bloomEffect.reset();  // 初期化失敗時はリセット
            return;  // Bloomなしで続行
        }

        // Bloomパラメータの設定（初期化成功時のみ）
        if (m_bloomEffect) {
            m_bloomEffect->SetThreshold(0.8f);
            m_bloomEffect->SetIntensity(1.5f);
            m_bloomEffect->SetBloomStrength(1.2f);
            m_bloomEffect->SetBlurRadius(2.5f);
        }
    }

    void OnUpdate(float deltaTime) override
    {
        m_rotation += 30.0f * deltaTime;
        if (m_rotation >= 360.0f)
            m_rotation -= 360.0f;

        m_time += deltaTime;

        // SpaceキーでBloom ON/OFF
        if (GetKeyDown(VK_SPACE))
        {
            m_bloomEnabled = !m_bloomEnabled;
        }
    }

    void OnRender() override
    {
        auto* batcher = GetLineBatcher();
        if (!batcher) return;

        const float centerX = m_config.width / 2.0f;
        const float centerY = m_config.height / 2.0f;

        // ========================================
        // 1. 放射状の線（24本）
        // ========================================
        const int lineCount = 24;
        const float innerRadius = 50.0f;
        const float outerRadius = 300.0f;

        for (int i = 0; i < lineCount; ++i)
        {
            float angle = (m_rotation + i * 360.0f / lineCount) * 3.14159f / 180.0f;

            float startX = centerX + std::cos(angle) * innerRadius;
            float startY = centerY + std::sin(angle) * innerRadius;
            float endX = centerX + std::cos(angle) * outerRadius;
            float endY = centerY + std::sin(angle) * outerRadius;

            // 虹色（HSV→RGB変換）
            float hue = (float)i / lineCount;
            float r, g, b;
            HSVtoRGB(hue, 1.0f, 1.0f, r, g, b);

            Vector2 start{ startX, startY };
            Vector2 end{ endX, endY };
            Color color{ r, g, b, 1.0f };

            batcher->AddLine(start, end, color, 3.0f, 1.0f);
        }

        // ========================================
        // 2. 中心の脈動する円
        // ========================================
        float pulse = std::sin(m_time * 3.0f) * 0.3f + 1.0f;
        float radius = 30.0f * pulse;

        Graphics::DrawCircle(
            batcher,
            Vector2{ centerX, centerY },
            radius,
            Color::Cyan,
            32,
            2.0f
        );

        // ========================================
        // 3. 回転する六角形
        // ========================================
        const int sides = 6;
        const float hexRadius = 150.0f;
        Vector2 prevPoint;

        for (int i = 0; i <= sides; ++i)
        {
            float angle = (-m_rotation * 0.5f + i * 360.0f / sides) * 3.14159f / 180.0f;
            float x = centerX + std::cos(angle) * hexRadius;
            float y = centerY + std::sin(angle) * hexRadius;

            Vector2 point{ x, y };

            if (i > 0)
            {
                batcher->AddLine(prevPoint, point, Color::Magenta, 2.0f, 1.0f);
            }

            prevPoint = point;
        }

        // 描画を実行
        batcher->Flush();

        // ========================================
        // 4. Bloomエフェクトを適用
        // ========================================
        if (m_bloomEnabled && m_bloomEffect)
        {
            // 現在のレンダーターゲットを取得
            auto* renderTarget = GetCurrentRenderTarget();

            // Bloomエフェクトを適用
            if (renderTarget) {
                m_bloomEffect->Apply(
                    GetCommandList(),
                    renderTarget,
                    renderTarget
                );
            }
        }
    }

private:
    // HSV→RGB変換
    void HSVtoRGB(float h, float s, float v, float& r, float& g, float& b)
    {
        float c = v * s;
        float x = c * (1.0f - std::abs(std::fmod(h * 6.0f, 2.0f) - 1.0f));
        float m = v - c;

        if (h < 1.0f / 6.0f) {
            r = c; g = x; b = 0;
        }
        else if (h < 2.0f / 6.0f) {
            r = x; g = c; b = 0;
        }
        else if (h < 3.0f / 6.0f) {
            r = 0; g = c; b = x;
        }
        else if (h < 4.0f / 6.0f) {
            r = 0; g = x; b = c;
        }
        else if (h < 5.0f / 6.0f) {
            r = x; g = 0; b = c;
        }
        else {
            r = c; g = 0; b = x;
        }

        r += m;
        g += m;
        b += m;
    }

    // キー入力取得（仮実装）
    bool GetKeyDown(int vkey)
    {
        // 実際のApplication.hに実装されているはず
        return (GetAsyncKeyState(vkey) & 0x8000) != 0;
    }
};

int main()
{
    BloomDemo app;
    return app.Run();
}
