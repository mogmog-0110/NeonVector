#pragma once

#include <cstdint>

namespace NeonVector
{

    // 基本型の定義
    using uint = uint32_t;
    using byte = uint8_t;

    /**
     * @struct Color
     * @brief RGBA色
     */
    struct Color
    {
        float r, g, b, a;

        Color() : r(1), g(1), b(1), a(1) {}
        Color(float r, float g, float b, float a = 1.0f)
            : r(r), g(g), b(b), a(a) {}

        // 定義済み色
        static const Color White;
        static const Color Black;
        static const Color Red;
        static const Color Green;
        static const Color Blue;
        static const Color Cyan;
        static const Color Magenta;
        static const Color Yellow;
    };

    // 定義済み色の実装
    inline const Color Color::White = {1.0f, 1.0f, 1.0f, 1.0f};
    inline const Color Color::Black = {0.0f, 0.0f, 0.0f, 1.0f};
    inline const Color Color::Red = {1.0f, 0.0f, 0.0f, 1.0f};
    inline const Color Color::Green = {0.0f, 1.0f, 0.0f, 1.0f};
    inline const Color Color::Blue = {0.3f, 0.7f, 1.0f, 1.0f};
    inline const Color Color::Cyan = {0.0f, 1.0f, 1.0f, 1.0f};
    inline const Color Color::Magenta = {1.0f, 0.0f, 1.0f, 1.0f};
    inline const Color Color::Yellow = {1.0f, 1.0f, 0.0f, 1.0f};

} // namespace NeonVector