#pragma once

#include <cstddef>
#include <deque>

#include "algorithm/filter/filter.h"

namespace VisionCursor
{
namespace Algorithm
{

class SMAFilter : public HandFilter
{
public:
    explicit SMAFilter(std::size_t window_size = 5) : window_size_(window_size == 0 ? 1 : window_size) {}

    HandLandmarkArray process(const HandLandmarkArray& input) override
    {
        if (!input.isValid())
        {
            return input;
        }

        HandLandmarkArray data = prev_ ? prev_->process(input) : input;

        history_.push_back(data);
        if (history_.size() > window_size_)
        {
            history_.pop_front();
        }

        HandLandmarkArray output;

        for (std::size_t i = 0; i < HAND_LANDMARK_COUNT; ++i)
        {
            float norm_sum_x = 0.0f;
            float norm_sum_y = 0.0f;
            float norm_sum_z = 0.0f;
            std::size_t norm_count = 0;

            float world_sum_x = 0.0f;
            float world_sum_y = 0.0f;
            float world_sum_z = 0.0f;
            std::size_t world_count = 0;

            for (const auto& frame : history_)
            {
                if (frame[i].has_normalized)
                {
                    norm_sum_x += frame[i].normalized.x;
                    norm_sum_y += frame[i].normalized.y;
                    norm_sum_z += frame[i].normalized.z;
                    ++norm_count;
                }

                if (frame[i].has_world)
                {
                    world_sum_x += frame[i].world.x;
                    world_sum_y += frame[i].world.y;
                    world_sum_z += frame[i].world.z;
                    ++world_count;
                }
            }

            if (norm_count > 0)
            {
                output[i].normalized.x = norm_sum_x / static_cast<float>(norm_count);
                output[i].normalized.y = norm_sum_y / static_cast<float>(norm_count);
                output[i].normalized.z = norm_sum_z / static_cast<float>(norm_count);
                output[i].has_normalized = true;
            }

            if (world_count > 0)
            {
                output[i].world.x = world_sum_x / static_cast<float>(world_count);
                output[i].world.y = world_sum_y / static_cast<float>(world_count);
                output[i].world.z = world_sum_z / static_cast<float>(world_count);
                output[i].has_world = true;
            }
        }

        return output;
    }

    void reset() override { history_.clear(); }

private:
    std::size_t window_size_;
    std::deque<HandLandmarkArray> history_;
};

} // namespace Algorithm
} // namespace VisionCursor
