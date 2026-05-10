#pragma once

#include <algorithm>

namespace wxConvert {

struct Interval
{
    int from = 0;
    int to = 0;
    constexpr int length() const noexcept { return to - from; }
};

inline double overlapRatio(const Interval& a, const Interval& b)
{
    const int overlap = std::max(0,
                                 std::min(a.to, b.to) - std::max(a.from, b.from));

    const int denom = std::min(a.length(), b.length());

    if (denom == 0)
        return 0.0;

    const double outcome{ overlap / static_cast<double>(denom) };
    return outcome;
}

inline bool overlaps(const Interval& a, const Interval& b) noexcept
{
    int maxFrom = std::max(a.from, b.from);
    int minTo = std::min(a.to, b.to);
    return maxFrom < minTo;
}

struct RcRectDU
{
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;

    constexpr int right() const noexcept { return left + width; }
    constexpr int bottom() const noexcept  { return top + height; }
    constexpr int surface() const noexcept  { return width * height; }
        bool operator==(const RcRectDU& other) const noexcept { return left == other.left && top == other.top && width == other.width && height == other.height; }
    void add(const RcRectDU& other)
    {
        left = std::min(left, other.left);
        top = std::min(top, other.top);
        width = std::max(right(), other.right()) - left;
        height = std::max(bottom(), other.bottom()) - top;
    }
};

inline bool operator!=(const RcRectDU& a, const RcRectDU& b) noexcept { return !(a == b); }
inline bool isInside(const RcRectDU& outline, int x, int y)
{
    return outline.left <= x && outline.right() >= x
        && outline.top <= y && outline.bottom() >= y;
}

inline bool isInside(const RcRectDU& outline, const RcRectDU& candidate)
{
    return outline != candidate && outline.left <= candidate.left && outline.right() >= candidate.right()
        && outline.top <= candidate.top && outline.bottom() >= candidate.bottom();
}

inline Interval horizontalInterval(const RcRectDU& rect) noexcept
{
    return { rect.left, rect.right() };
}

inline Interval verticalInterval(const RcRectDU& rect) noexcept
{
    return { rect.top, rect.bottom() };
}

} // namespace wxConvert