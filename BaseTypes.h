#pragma once

namespace wxConvert {

struct RcRectDU
{
    int left = 0;
    int top = 0;
    int width = 0;
    int height = 0;

    int right() const { return left + width; }
    int bottom() const { return top + height; }
    int surface() const { return width * height; }
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