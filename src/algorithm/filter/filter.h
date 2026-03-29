#pragma once

#include <memory>

#include "common/common.h"

namespace VisionCursor
{
namespace Algorithm
{

/// 滤波器基类
// 抽象组件：滤波器接口
class HandFilter
{
public:
    virtual ~HandFilter()                                             = default;
    virtual HandLandmarkArray process(const HandLandmarkArray& input) = 0;
    void decorate(std::shared_ptr<HandFilter>& prev) { prev_ = prev; };
    virtual void reset() {};

protected:
    std::shared_ptr<HandFilter> prev_;
};

}      // namespace Algorithm
}      // namespace VisionCursor
