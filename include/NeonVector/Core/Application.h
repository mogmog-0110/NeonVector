#pragma once

#include <string>
#include <memory>
#include <Windows.h>

namespace NeonVector
{

    // 前方宣言
    class DX12Context;


    namespace Graphics {
        class LineBatcher; 
    }

    /**
     * @struct ApplicationConfig
     * @brief アプリケーション設定
     */
    struct ApplicationConfig
    {
        std::string title = "NeonVector App";
        int width = 800;
        int height = 600;
        bool vsync = true;
        bool fullscreen = false;
    };

    /**
     * @class Application
     * @brief アプリケーションの基底クラス
     */
    class Application
    {
    public:
        Application(const ApplicationConfig &config = {});
        virtual ~Application();

        /**
         * @brief アプリケーションを実行
         * @return 終了コード
         */
        int Run();

        /**
         * @brief 実行中かどうか
         */
        bool IsRunning() const { return m_isRunning; }

        /**
         * @brief アプリケーションを終了
         */
        void Quit() { m_isRunning = false; }

    protected:
        /**
         * @brief 初期化時に呼ばれる
         */
        virtual void OnInit() {}

        /**
         * @brief 毎フレーム更新時に呼ばれる
         * @param deltaTime 前フレームからの経過時間（秒）
         */
        virtual void OnUpdate(float deltaTime) {}

        /**
         * @brief 描画時に呼ばれる
         */
        virtual void OnRender() {}

        /**
         * @brief 終了時に呼ばれる
         */
        virtual void OnShutdown() {}

        Graphics::LineBatcher* GetLineBatcher() const;

    protected:
        ApplicationConfig m_config;
        std::unique_ptr<DX12Context> m_context;
        HWND m_hwnd;
        bool m_isRunning;

    private:
        static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
        void ProcessMessages();
    };

} // namespace NeonVector