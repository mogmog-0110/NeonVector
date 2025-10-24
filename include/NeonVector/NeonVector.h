/**
 * @file NeonVector.h
 * @brief NeonVector Engine メインヘッダー
 * @version 0.1.0
 */

#pragma once

// Core
#include "Core/Application.h"
#include "Core/Types.h"

// Math
#include "Math/Vector2.h"

// Graphics
#include "Graphics/LineBatcher.h"

/**
 * @namespace NeonVector
 * @brief NeonVector Engine の全機能を含む名前空間
 */
namespace NeonVector
{

    /**
     * @brief エンジンのバージョン情報
     */
    struct Version
    {
        static constexpr int Major = 0;
        static constexpr int Minor = 1;
        static constexpr int Patch = 0;

        static const char *GetString()
        {
            return "0.1.0";
        }
    };

    namespace Simple
    {
        bool Init(int width, int height, const char *title);
        bool Update();
        void Shutdown();
        void ClearScreen(const Color &color = Color::Black);

        // 新しい描画関数
        void DrawLine(const Vector2 &start, const Vector2 &end,
                      const Color &color, float thickness = 1.0f);
        void FlushLines();
    }

} // namespace NeonVector

// エイリアス（短く書けるように）
namespace NV = NeonVector;