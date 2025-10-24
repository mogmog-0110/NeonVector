#pragma once

#include <cmath>

namespace NeonVector
{

    /**
     * @struct Vector2
     * @brief 2次元ベクトル
     */
    struct Vector2
    {
        float x, y;

        // コンストラクタ
        Vector2() : x(0), y(0) {}
        Vector2(float x, float y) : x(x), y(y) {}

        // 算術演算子
        Vector2 operator+(const Vector2 &other) const
        {
            return {x + other.x, y + other.y};
        }

        Vector2 operator-(const Vector2 &other) const
        {
            return {x - other.x, y - other.y};
        }

        Vector2 operator*(float scalar) const
        {
            return {x * scalar, y * scalar};
        }

        // ベクトル演算
        float Length() const
        {
            return std::sqrt(x * x + y * y);
        }

        float LengthSquared() const
        {
            return x * x + y * y;
        }

        void Normalize()
        {
            float len = Length();
            if (len > 0.0f)
            {
                x /= len;
                y /= len;
            }
        }

        // 静的メソッド
        static Vector2 Zero() { return {0, 0}; }
        static Vector2 One() { return {1, 1}; }
    };

} // namespace NeonVector