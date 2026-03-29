#pragma once


#include <memory>

#include "common/common.h"

namespace VisionCursor
{
namespace Interaction
{
namespace Rule
{

class Rule
{
public:
    virtual ~Rule() = default;

    // 判断当前规则是否成立
    virtual bool evaluate(const HandLandmarkArray& landmarks) = 0;

    // 重置规则内部状态
    virtual void reset() = 0;
};

using RulePtr = std::shared_ptr<Rule>;

} // namespace Rule
} // namespace Interaction
} // namespace VisionCursor