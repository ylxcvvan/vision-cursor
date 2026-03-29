#pragma once

#include <array>
#include <cmath>
#include <cstddef>

#include "algorithm/filter/filter.h"

namespace VisionCursor
{
namespace Algorithm
{

class OneEuroFilter : public HandFilter
{
public:
    explicit OneEuroFilter(float min_cutoff = 12.0f,
                           float beta = 30.0f,
                           float d_cutoff = 1.0f,
                           float frequency = 60.0f)
        : min_cutoff_(min_cutoff > 0.0f ? min_cutoff : 1.0f),
          beta_(beta >= 0.0f ? beta : 0.0f),
          d_cutoff_(d_cutoff > 0.0f ? d_cutoff : 1.0f),
          frequency_(frequency > 0.0f ? frequency : 60.0f),
          initialized_(false)
    {
    }

    HandLandmarkArray process(const HandLandmarkArray& input) override
    {
        if (!input.isValid())
        {
            return input;
        }

        HandLandmarkArray data = prev_ ? prev_->process(input) : input;

        if (!initialized_)
        {
            prev_raw_ = data;
            prev_filtered_ = data;
            clearDerivatives();
            initialized_ = true;
            return data;
        }

        HandLandmarkArray output = data;
        const float dt = 1.0f / frequency_;

        auto filter_space = [this, dt](bool has_curr,
                                       const Landmark& curr,
                                       bool has_prev_raw,
                                       const Landmark& prev_raw,
                                       bool has_prev_filtered,
                                       const Landmark& prev_filtered,
                                       Landmark& out,
                                       Landmark& prev_d,
                                       bool& has_prev_d) {
            if (!has_curr)
            {
                out = {};
                has_prev_d = false;
                prev_d = {};
                return false;
            }

            if (!has_prev_raw || !has_prev_filtered)
            {
                out = curr;
                has_prev_d = false;
                prev_d = {};
                return true;
            }

            const float dx = (curr.x - prev_raw.x) / dt;
            const float dy = (curr.y - prev_raw.y) / dt;
            const float dz = (curr.z - prev_raw.z) / dt;

            const float alpha_d = smoothingFactor(dt, d_cutoff_);

            const float prev_dx = has_prev_d ? prev_d.x : 0.0f;
            const float prev_dy = has_prev_d ? prev_d.y : 0.0f;
            const float prev_dz = has_prev_d ? prev_d.z : 0.0f;

            const float dx_hat = exponentialSmoothing(alpha_d, dx, prev_dx);
            const float dy_hat = exponentialSmoothing(alpha_d, dy, prev_dy);
            const float dz_hat = exponentialSmoothing(alpha_d, dz, prev_dz);

            prev_d.x = dx_hat;
            prev_d.y = dy_hat;
            prev_d.z = dz_hat;
            has_prev_d = true;

            const float cutoff_x = min_cutoff_ + beta_ * std::fabs(dx_hat);
            const float cutoff_y = min_cutoff_ + beta_ * std::fabs(dy_hat);
            const float cutoff_z = min_cutoff_ + beta_ * std::fabs(dz_hat);

            const float alpha_x = smoothingFactor(dt, cutoff_x);
            const float alpha_y = smoothingFactor(dt, cutoff_y);
            const float alpha_z = smoothingFactor(dt, cutoff_z);

            out.x = exponentialSmoothing(alpha_x, curr.x, prev_filtered.x);
            out.y = exponentialSmoothing(alpha_y, curr.y, prev_filtered.y);
            out.z = exponentialSmoothing(alpha_z, curr.z, prev_filtered.z);
            return true;
        };

        for (std::size_t i = 0; i < HAND_LANDMARK_COUNT; ++i)
        {
            output[i].has_normalized = filter_space(data[i].has_normalized,
                                                    data[i].normalized,
                                                    prev_raw_[i].has_normalized,
                                                    prev_raw_[i].normalized,
                                                    prev_filtered_[i].has_normalized,
                                                    prev_filtered_[i].normalized,
                                                    output[i].normalized,
                                                    prev_d_norm_[i],
                                                    has_prev_d_norm_[i]);

            output[i].has_world = filter_space(data[i].has_world,
                                               data[i].world,
                                               prev_raw_[i].has_world,
                                               prev_raw_[i].world,
                                               prev_filtered_[i].has_world,
                                               prev_filtered_[i].world,
                                               output[i].world,
                                               prev_d_world_[i],
                                               has_prev_d_world_[i]);
        }

        prev_raw_ = data;
        prev_filtered_ = output;

        return output;
    }

    void reset() override
    {
        initialized_ = false;
        prev_raw_.reset();
        prev_filtered_.reset();
        clearDerivatives();
    }

private:
    static float exponentialSmoothing(float alpha, float value, float prev_value)
    {
        return alpha * value + (1.0f - alpha) * prev_value;
    }

    static float smoothingFactor(float dt, float cutoff)
    {
        constexpr float kPi = 3.14159265358979323846f;
        const float r = 2.0f * kPi * cutoff * dt;
        return r / (r + 1.0f);
    }

    void clearDerivatives()
    {
        for (std::size_t i = 0; i < HAND_LANDMARK_COUNT; ++i)
        {
            prev_d_norm_[i] = {};
            prev_d_world_[i] = {};
            has_prev_d_norm_[i] = false;
            has_prev_d_world_[i] = false;
        }
    }

private:
    float min_cutoff_;
    float beta_;
    float d_cutoff_;
    float frequency_;

    bool initialized_;

    HandLandmarkArray prev_raw_;
    HandLandmarkArray prev_filtered_;

    std::array<Landmark, HAND_LANDMARK_COUNT> prev_d_norm_{};
    std::array<Landmark, HAND_LANDMARK_COUNT> prev_d_world_{};
    std::array<bool, HAND_LANDMARK_COUNT> has_prev_d_norm_{};
    std::array<bool, HAND_LANDMARK_COUNT> has_prev_d_world_{};
};

} // namespace Algorithm
} // namespace VisionCursor
