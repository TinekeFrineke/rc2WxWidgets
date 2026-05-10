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
    ////int minFrom = std::min(a.from, b.from);
    ////int maxFrom = std::max(a.from, b.from);
    ////int minTo = std::min(a.to, b.to);
    ////int maxTo = std::min(a.to, b.to);
    ////return std::max(0, minTo - maxFrom);

    //const int minFrom = std::min(a.from, b.from);
    //const int maxFrom = std::max(a.from, b.from);
    //const int minTo = std::min(a.to, b.to);
    //const int maxTo = std::max(a.to, b.to);
    //const int largest = std::max(a.length(), b.length());
    //const int actualOverlap(Interval(maxFrom, minTo).length());
    //const double outcome = actualOverlap / static_cast<double>(largest);
    //return outcome;
}

inline bool overlaps(const Interval& a, const Interval& b) noexcept
{
    int maxFrom = std::max(a.from, b.from);
    int minTo = std::min(a.to, b.to);
    return maxFrom < minTo;
}

//// return percentage of overlap with smallest interval as 100% (e.g. 0.5 for half overlap, 1.0 for full overlap, >1.0 for more than full)
//inline double overlap(std::vector<Interval>& intervals)
//{
//    std::sort(intervals.begin(), intervals.end(), [](const Interval& a, const Interval& b) {
//        return a.length() < b.length();
//    });
//
//    double totalOverlap = 0.0;
//    for (size_t i = 0; i < intervals.size(); ++i) {
//        for (size_t j = i + 1; j < intervals.size(); ++j) {
//            if (overlaps(intervals[i], intervals[j])) {
//                int maxFrom = std::max(intervals[i].from, intervals[j].from);
//                int minTo = std::min(intervals[i].to, intervals[j].to);
//                totalOverlap += static_cast<double>(minTo - maxFrom);
//            }
//        }
//    }
//    return totalOverlap;
//}

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