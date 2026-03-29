#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
#include <vector>

#include "common/common.h"

namespace VisionCursor
{
namespace Utils
{

template <typename PointT>
inline float pointX(const PointT& p)
{
    return p.x;
}

template <typename PointT>
inline float pointY(const PointT& p)
{
    return p.y;
}

template <typename PointT>
inline float pointZ(const PointT& p)
{
    return p.z;
}

inline float pointX(const HandLandmark& p) { return p.normalized.x; }
inline float pointY(const HandLandmark& p) { return p.normalized.y; }
inline float pointZ(const HandLandmark& p) { return p.normalized.z; }

template <typename T>
inline T mean(const std::vector<T>& data)
{
    if (data.empty())
    {
        return static_cast<T>(0);
    }

    const T sum = std::accumulate(data.begin(), data.end(), static_cast<T>(0));
    return sum / static_cast<T>(data.size());
}

template <typename T>
inline T stddev(const std::vector<T>& data, T avg)
{
    if (data.size() < 2)
    {
        return static_cast<T>(0);
    }

    T accum = static_cast<T>(0);
    for (const auto& v : data)
    {
        const T diff = v - avg;
        accum += diff * diff;
    }

    return static_cast<T>(std::sqrt(accum / static_cast<T>(data.size())));
}

template <typename T>
inline T minValue(const std::vector<T>& data)
{
    if (data.empty())
    {
        return static_cast<T>(0);
    }

    return *std::min_element(data.begin(), data.end());
}

template <typename T>
inline T maxValue(const std::vector<T>& data)
{
    if (data.empty())
    {
        return static_cast<T>(0);
    }

    return *std::max_element(data.begin(), data.end());
}

template <typename T>
inline T coeffOfVariation(const std::vector<T>& data)
{
    const T avg = mean(data);
    if (std::fabs(avg) <= static_cast<T>(1e-6))
    {
        return std::numeric_limits<T>::infinity();
    }

    return stddev(data, avg) / avg;
}

template <typename PointT>
inline float distance3D(const PointT& a, const PointT& b)
{
    const float dx = pointX(a) - pointX(b);
    const float dy = pointY(a) - pointY(b);
    const float dz = pointZ(a) - pointZ(b);
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

template <typename T>
inline bool floatEqual(T a, T b, T eps = static_cast<T>(1e-6))
{
    return std::fabs(a - b) <= eps;
}

template <typename PointT>
inline float angle3D(const PointT& a, const PointT& b, const PointT& c)
{
    const float bax = pointX(a) - pointX(b);
    const float bay = pointY(a) - pointY(b);
    const float baz = pointZ(a) - pointZ(b);

    const float bcx = pointX(c) - pointX(b);
    const float bcy = pointY(c) - pointY(b);
    const float bcz = pointZ(c) - pointZ(b);

    const float dot = bax * bcx + bay * bcy + baz * bcz;
    const float norm_ba = std::sqrt(bax * bax + bay * bay + baz * baz);
    const float norm_bc = std::sqrt(bcx * bcx + bcy * bcy + bcz * bcz);

    constexpr float kEps = 1e-6f;
    if (norm_ba <= kEps || norm_bc <= kEps)
    {
        return 0.0f;
    }

    float cos_theta = dot / (norm_ba * norm_bc);
    cos_theta = std::max(-1.0f, std::min(1.0f, cos_theta));

    constexpr float kRadToDeg = 57.29577951308232f;
    return std::acos(cos_theta) * kRadToDeg;
}

template <typename PointT>
inline float triangleArea2D(const PointT& a, const PointT& b, const PointT& c)
{
    const float area2 = std::fabs((pointX(b) - pointX(a)) * (pointY(c) - pointY(a))
                                - (pointY(b) - pointY(a)) * (pointX(c) - pointX(a)));
    return 0.5f * area2;
}

template <typename PointT>
inline float triangleArea3D(const PointT& a, const PointT& b, const PointT& c)
{
    const float abx = pointX(b) - pointX(a);
    const float aby = pointY(b) - pointY(a);
    const float abz = pointZ(b) - pointZ(a);

    const float acx = pointX(c) - pointX(a);
    const float acy = pointY(c) - pointY(a);
    const float acz = pointZ(c) - pointZ(a);

    const float cx = aby * acz - abz * acy;
    const float cy = abz * acx - abx * acz;
    const float cz = abx * acy - aby * acx;

    return 0.5f * std::sqrt(cx * cx + cy * cy + cz * cz);
}

template <typename T>
inline T trimmedMean(std::vector<T> data, std::size_t trim_count = 1)
{
    if (data.empty())
    {
        return static_cast<T>(0);
    }

    std::sort(data.begin(), data.end());

    if (data.size() <= 2 * trim_count)
    {
        return mean(data);
    }

    const auto begin_it = data.begin() + static_cast<std::ptrdiff_t>(trim_count);
    const auto end_it = data.end() - static_cast<std::ptrdiff_t>(trim_count);

    const T sum = std::accumulate(begin_it, end_it, static_cast<T>(0));
    return sum / static_cast<T>(std::distance(begin_it, end_it));
}

template <typename PointT>
inline float palmAreaScale3D(const PointT& wrist,
                             const PointT& index_mcp,
                             const PointT& middle_mcp,
                             const PointT& pinky_mcp)
{
    const float area1 = triangleArea3D(wrist, index_mcp, middle_mcp);
    const float area2 = triangleArea3D(wrist, middle_mcp, pinky_mcp);
    return std::sqrt(std::max(area1 + area2, 0.0f));
}

template <typename PointT>
inline float palmPrincipalAxisScale3D(const PointT& wrist,
                                      const PointT& index_mcp,
                                      const PointT& middle_mcp,
                                      const PointT& pinky_mcp)
{
    const float width = distance3D(index_mcp, pinky_mcp);
    const float height = distance3D(wrist, middle_mcp);
    return std::sqrt(std::max(width * height, 0.0f));
}

template <typename PointT>
inline float palmRobustEdgeScale3D(const PointT& wrist,
                                   const PointT& index,
                                   const PointT& middle,
                                   const PointT& ring,
                                   const PointT& pinky)
{
    std::vector<float> edges = {
        distance3D(wrist, index),
        distance3D(wrist, middle),
        distance3D(wrist, ring),
        distance3D(wrist, pinky),
        distance3D(index, pinky)
    };

    return trimmedMean(edges, 1);
}

} // namespace Utils
} // namespace VisionCursor