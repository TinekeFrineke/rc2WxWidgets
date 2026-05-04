#pragma once

namespace wxConvert {

struct RcRectDU
{
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;

    constexpr int right() const noexcept { return left + width; }
    constexpr int bottom() const noexcept  { return top + height; }
    constexpr int surface() const noexcept  { return width * height; }
};

inline bool isInside(const RcRectDU& outline, int x, int y)
{
    return outline.left <= x && outline.right() >= x
        && outline.top <= y && outline.bottom() >= y;
}

inline bool isInside(const RcRectDU& outline, const RcRectDU& candidate)
{
    return outline.left <= candidate.left && outline.right() >= candidate.right()
        && outline.top <= candidate.top && outline.bottom() >= candidate.bottom();
}

} // namespace wxConvert