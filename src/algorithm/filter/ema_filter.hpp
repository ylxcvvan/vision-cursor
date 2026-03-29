#pragma once

#include <cstddef>

#include "algorithm/filter/filter.h"

namespace VisionCursor
{
namespace Algorithm
{

class EMAFilter : public HandFilter
{
public:
    explicit EMAFilter(float alpha = 0.5f) : alpha_(clampAlpha(alpha)), initialized_(false) {}

    HandLandmarkArray process(const HandLandmarkArray& input) override
    {
        if (!input.isValid())
        {
            return input;
        }

        HandLandmarkArray data = prev_ ? prev_->process(input) : input;

        if (!initialized_)
        {
            last_output_ = data;
            initialized_ = true;
            return last_output_;
        }

        HandLandmarkArray output = data;

        for (std::size_t i = 0; i < HAND_LANDMARK_COUNT; ++i)
        {
            if (data[i].has_normalized && last_output_[i].has_normalized)
            {
                output[i].normalized.x = alpha_ * data[i].normalized.x + (1.0f - alpha_) * last_output_[i].normalized.x;
                output[i].normalized.y = alpha_ * data[i].normalized.y + (1.0f - alpha_) * last_output_[i].normalized.y;
                output[i].normalized.z = alpha_ * data[i].normalized.z + (1.0f - alpha_) * last_output_[i].normalized.z;
            }

            if (data[i].has_world && last_output_[i].has_world)
            {
                output[i].world.x = alpha_ * data[i].world.x + (1.0f - alpha_) * last_output_[i].world.x;
                output[i].world.y = alpha_ * data[i].world.y + (1.0f - alpha_) * last_output_[i].world.y;
                output[i].world.z = alpha_ * data[i].world.z + (1.0f - alpha_) * last_output_[i].world.z;
            }
        }

        last_output_ = output;
        return output;
    }

    void reset() override
    {
        initialized_ = false;
        last_output_.reset();
    }

private:
    static float clampAlpha(float alpha)
    {
        if (alpha < 0.0f)
        {
            return 0.0f;
        }
        if (alpha > 1.0f)
        {
            return 1.0f;
        }
        return alpha;
    }

private:
    float alpha_;
    bool initialized_;
    HandLandmarkArray last_output_;
};

} // namespace Algorithm
} // namespace VisionCursor
