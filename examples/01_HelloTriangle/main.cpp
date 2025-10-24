#include <NeonVector/NeonVector.h>
#include <iostream>

using namespace NeonVector;

/**
 * @brief 最小限のサンプル
 *
 * 黒い画面を表示するだけ。
 * DirectX12の初期化が成功すればOK。
 */
class HelloTriangleApp : public Application
{
public:
    HelloTriangleApp() : Application({.title = "NeonVector - Hello Triangle",
                                      .width = 800,
                                      .height = 600,
                                      .vsync = true}) {}

protected:
    void OnInit() override
    {
        std::cout << "=== NeonVector Engine ===" << std::endl;
        std::cout << "Version: " << Version::GetString() << std::endl;
        std::cout << "Press ESC to quit" << std::endl;
    }

    void OnUpdate(float deltaTime) override
    {
        // 将来的にゲームロジックを追加
    }

    void OnRender() override
    {
        // 将来的に描画処理を追加
    }

    void OnShutdown() override
    {
        std::cout << "Goodbye!" << std::endl;
    }
};

int main()
{
    try
    {
        HelloTriangleApp app;
        return app.Run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
}