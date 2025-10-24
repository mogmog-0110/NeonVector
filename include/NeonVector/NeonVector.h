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

} // namespace NeonVector

// エイリアス（短く書けるように）
namespace NV = NeonVector;