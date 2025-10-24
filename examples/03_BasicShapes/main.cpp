#include <NeonVector/NeonVector.h>
#include <NeonVector/Graphics/Primitives.h>
#include <iostream>
#include <cmath>

using namespace NeonVector;
using namespace NeonVector::Graphics;

/**
 * @brief 基本図形のテスト
 */
class BasicShapesApp : public Application
{
public:
    BasicShapesApp()
        : Application({.title = "NeonVector - Basic Shapes",
                       .width = 800,
                       .height = 600,
                       .vsync = true})
    {
    }

protected:
    void OnInit() override
    {
        std::cout << "=== Basic Shapes Test ===" << std::endl;
        std::cout << "Press ESC to quit" << std::endl;
        std::cout << "Press SPACE to cycle through demos" << std::endl;
    }

    void OnUpdate(float deltaTime) override
    {
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            Quit();
        }

        // スペースキーでデモ切り替え
        static bool spaceWasPressed = false;
        bool spaceIsPressed = (GetAsyncKeyState(VK_SPACE) & 0x8000) != 0;

        if (spaceIsPressed && !spaceWasPressed)
        {
            m_currentDemo = (m_currentDemo + 1) % 4;
            std::cout << "Demo: " << m_currentDemo << std::endl;
        }
        spaceWasPressed = spaceIsPressed;

        m_time += deltaTime;
    }

    void OnRender() override
    {
        auto *batcher = GetLineBatcher();
        if (!batcher)
            return;

        switch (m_currentDemo)
        {
        case 0:
            RenderCircles(batcher);
            break;
        case 1:
            RenderRectangles(batcher);
            break;
        case 2:
            RenderPolygons(batcher);
            break;
        case 3:
            RenderAll(batcher);
            break;
        }

        batcher->Flush();
    }

    void OnShutdown() override
    {
        std::cout << "Basic Shapes Test finished" << std::endl;
    }

private:
    void RenderCircles(LineBatcher *batcher)
    {
        // 同心円
        for (int i = 1; i <= 5; i++)
        {
            float radius = i * 30.0f;
            Color color = Color::White;
            color.r = (i % 3 == 0) ? 1.0f : 0.0f;
            color.g = (i % 3 == 1) ? 1.0f : 0.0f;
            color.b = (i % 3 == 2) ? 1.0f : 0.0f;
            DrawCircle(batcher, {400, 300}, radius, color, 64, 2.0f);
        }

        // アニメーション円
        float x = 400 + std::cos(m_time) * 100;
        float y = 300 + std::sin(m_time) * 100;
        DrawCircle(batcher, {x, y}, 20, Color::Yellow, 32, 3.0f);
    }

    void RenderRectangles(LineBatcher *batcher)
    {
        // 回転する矩形
        for (int i = 0; i < 5; i++)
        {
            float angle = m_time + i * 0.3f;
            float size = 50 + i * 30.0f;

            std::vector<Vector2> rect;
            float halfSize = size / 2.0f;

            for (int j = 0; j < 4; j++)
            {
                float a = angle + j * 1.57079632f; // 90度ずつ
                rect.push_back({400 + std::cos(a) * halfSize,
                                300 + std::sin(a) * halfSize});
            }

            Color color = Color::White;
            color.r = (i % 2 == 0) ? 1.0f : 0.0f;
            color.g = 1.0f;
            color.b = (i % 2 == 1) ? 1.0f : 0.0f;

            DrawPolygon(batcher, rect, color, true, 2.0f);
        }
    }

    void RenderPolygons(LineBatcher *batcher)
    {
        // 様々な正多角形
        for (int sides = 3; sides <= 8; sides++)
        {
            std::vector<Vector2> polygon;
            float angle_offset = m_time * 0.5f;
            float radius = 50 + (sides - 3) * 15.0f;

            for (int i = 0; i < sides; i++)
            {
                float angle = (i / (float)sides) * 2.0f * 3.14159f + angle_offset;
                polygon.push_back({400 + std::cos(angle) * radius,
                                   300 + std::sin(angle) * radius});
            }

            Color color = Color::White;
            float t = (sides - 3) / 5.0f;
            color.r = t;
            color.g = 1.0f - t;
            color.b = 0.5f;

            DrawPolygon(batcher, polygon, color, true, 2.0f);
        }
    }

    void RenderAll(LineBatcher *batcher)
    {
        // 円
        DrawCircle(batcher, {400, 300}, 100, Color::Cyan, 32, 2.0f);
        DrawCircle(batcher, {400, 300}, 150, Color::Magenta, 64, 2.0f);

        // 矩形
        DrawRect(batcher, {200, 200}, {400, 200}, Color::Yellow, 2.0f);

        // 五角形
        std::vector<Vector2> pentagon;
        for (int i = 0; i < 5; i++)
        {
            float angle = (i / 5.0f) * 2.0f * 3.14159f - 3.14159f / 2.0f + m_time;
            pentagon.push_back({400 + std::cos(angle) * 50,
                                300 + std::sin(angle) * 50});
        }
        DrawPolygon(batcher, pentagon, Color::Green, true, 3.0f);
    }

private:
    float m_time = 0.0f;
    int m_currentDemo = 3; // 0:円, 1:矩形, 2:多角形, 3:全部
};

int main()
{
    try
    {
        BasicShapesApp app;
        return app.Run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}