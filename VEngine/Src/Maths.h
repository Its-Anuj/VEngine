#pragma once

namespace VEngine
{
    struct Vec2
    {
        float x, y;

        Vec2(float px = 0.0f, float py = 0.0f) : x(px), y(py) {}

        Vec2 &operator=(const Vec2 &other)
        {
            this->x = other.x;
            this->y = other.y;
            return *this;
        }

        // Arithmetic assignment operators
        Vec2 &operator+=(const Vec2 &rhs)
        {
            x += rhs.x;
            y += rhs.y;
            return *this;
        }
        Vec2 &operator-=(const Vec2 &rhs)
        {
            x -= rhs.x;
            y -= rhs.y;
            return *this;
        }
        Vec2 &operator*=(const Vec2 &rhs)
        {
            x *= rhs.x;
            y *= rhs.y;
            return *this;
        }
        Vec2 &operator/=(const Vec2 &rhs)
        {
            x /= rhs.x;
            y /= rhs.y;
            return *this;
        }

        // Comparison operators
        bool operator==(const Vec2 &rhs) const
        {
            return x == rhs.x && y == rhs.y;
        }
        bool operator!=(const Vec2 &rhs) const
        {
            return !(*this == rhs);
        }
        bool operator<(const Vec2 &rhs) const
        {
            return (x < rhs.x) || (x == rhs.x && y < rhs.y);
        }
        bool operator>(const Vec2 &rhs) const
        {
            return (x > rhs.x) || (x == rhs.x && y > rhs.y);
        }
    };
} // namespace VEngine
