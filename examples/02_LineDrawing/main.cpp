#include <NeonVector/NeonVector.h>
#include <iostream>
#include <cmath>

using namespace NeonVector;

/**
 * @brief 線描画のテストプログラム
 */
class LineDrawingApp : public Application
{
public:
    LineDrawingApp() : Application({.title = "NeonVector - Line Drawing Test",
                                    .width = 800,
                                    .height = 600,
                                    .vsync = true}) {}

protected:
    void OnInit() override
    {
        std::cout << "=== Line Drawing Test ===" << std::endl;
        std::cout << "Press ESC to quit" << std::endl;
    }

    void OnUpdate(float deltaTime) override
    {
        // ESCで終了
        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000)
        {
            Quit();
        }

        m_time += deltaTime;
    }

    void OnRender() override
    {
        auto* batcher = GetLineBatcher();
        if (!batcher)
            return;

        // テスト1: 静的な線
        batcher->AddLine({100, 100}, {700, 100}, Color::Cyan, 2.0f);
        batcher->AddLine({100, 100}, {100, 500}, Color::Magenta, 2.0f);
        batcher->AddLine({700, 100}, {700, 500}, Color::Yellow, 2.0f);
        batcher->AddLine({100, 500}, {700, 500}, Color::Green, 2.0f);

        // テスト2: 対角線
        batcher->AddLine({100, 100}, {700, 500}, Color::Red, 3.0f);
        batcher->AddLine({700, 100}, {100, 500}, Color::Blue, 3.0f);

        // テスト3: アニメーションする線
        float centerX = 400.0f;
        float centerY = 300.0f;
        float radius = 200.0f;

        for (int i = 0; i < 12; i++)
        {
            float angle = (i / 12.0f) * 3.14159f * 2.0f + m_time;
            float x = centerX + std::cos(angle) * radius;
            float y = centerY + std::sin(angle) * radius;

            Color color = Color::White;
            color.r = (std::sin(angle) + 1.0f) * 0.5f;
            color.g = (std::cos(angle) + 1.0f) * 0.5f;
            color.b = (std::sin(angle * 2.0f) + 1.0f) * 0.5f;

            batcher->AddLine({centerX, centerY}, {x, y}, color, 2.0f);
        }

        // すべての線を描画
        batcher->Flush();
    }

    void OnShutdown() override
    {
        std::cout << "Line Drawing Test finished" << std::endl;
    }

private:
    float m_time = 0.0f;
};

int main()
{
    try
    {
        LineDrawingApp app;
        return app.Run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}