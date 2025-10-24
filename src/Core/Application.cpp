#include "NeonVector/Core/Application.h"
#include "../DX12Context.h"
#include <chrono>
#include <iostream>

namespace NeonVector
{

    Application::Application(const ApplicationConfig &config)
        : m_config(config), m_hwnd(nullptr), m_isRunning(false), m_context(nullptr)
    {
    }

    Application::~Application()
    {
    }

    int Application::Run()
    {
        // ウィンドウクラス登録
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.style = CS_HREDRAW | CS_VREDRAW;
        wc.lpfnWndProc = WindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
        wc.lpszClassName = L"NeonVectorWindowClass";
        RegisterClassExW(&wc);

        // ウィンドウサイズ計算
        RECT rect = {0, 0, m_config.width, m_config.height};
        AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, FALSE);

        // ウィンドウ作成
        std::wstring title(m_config.title.begin(), m_config.title.end());
        m_hwnd = CreateWindowExW(
            0,
            L"NeonVectorWindowClass",
            title.c_str(),
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT, CW_USEDEFAULT,
            rect.right - rect.left,
            rect.bottom - rect.top,
            nullptr,
            nullptr,
            GetModuleHandle(nullptr),
            this);

        if (!m_hwnd)
        {
            std::cerr << "Failed to create window" << std::endl;
            return -1;
        }

        ShowWindow(m_hwnd, SW_SHOW);
        UpdateWindow(m_hwnd);

        // DirectX12初期化
        m_context = std::make_unique<DX12Context>();
        if (!m_context->Initialize(m_hwnd, m_config.width, m_config.height))
        {
            std::cerr << "Failed to initialize DirectX12" << std::endl;
            return -1;
        }

        std::cout << "NeonVector Engine initialized successfully!" << std::endl;

        // ユーザー初期化
        OnInit();

        // メインループ
        m_isRunning = true;
        auto lastTime = std::chrono::high_resolution_clock::now();

        while (m_isRunning)
        {
            // メッセージ処理
            ProcessMessages();

            if (!m_isRunning)
                break;

            // deltaTime計算
            auto currentTime = std::chrono::high_resolution_clock::now();
            float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
            lastTime = currentTime;

            // 更新
            OnUpdate(deltaTime);

            // 描画
            m_context->BeginFrame();
            m_context->ClearRenderTarget(0.0f, 0.0f, 0.0f, 1.0f);
            OnRender();
            m_context->EndFrame();
            m_context->Present(m_config.vsync);
        }

        // 終了処理
        OnShutdown();
        m_context->Shutdown();

        DestroyWindow(m_hwnd);
        UnregisterClassW(L"NeonVectorWindowClass", GetModuleHandle(nullptr));

        return 0;
    }

    void Application::ProcessMessages()
    {
        MSG msg = {};
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                m_isRunning = false;
            }
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    LRESULT CALLBACK Application::WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        Application *app = nullptr;

        if (msg == WM_CREATE)
        {
            CREATESTRUCT *pCreate = reinterpret_cast<CREATESTRUCT *>(lParam);
            app = reinterpret_cast<Application *>(pCreate->lpCreateParams);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(app));
        }
        else
        {
            app = reinterpret_cast<Application *>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
        }

        if (app)
        {
            switch (msg)
            {
            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            case WM_KEYDOWN:
                if (wParam == VK_ESCAPE)
                {
                    app->Quit();
                }
                return 0;
            }
        }

        return DefWindowProcW(hwnd, msg, wParam, lParam);
    }

} // namespace NeonVector