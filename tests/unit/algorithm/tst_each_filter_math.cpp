#include <gtest/gtest.h>

#include <cstdint>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <opencv2/core/utils/logger.hpp>
#include <opencv2/opencv.hpp>

#include "algorithm/filter/ema_filter.hpp"
#include "algorithm/filter/one_euro_filter.hpp"
#include "algorithm/filter/sma_filter.hpp"
#include "test_helpers.h"
#include "utils/utils.h"
#include "vision/opencv_frame.h"
#include "vision_cursor_test_fixture.h"

using namespace VisionCursor::Algorithm;

namespace
{

struct FilterMetrics
{
    std::string name;
    int valid_frame_count = 0;

    std::vector<float> step_distances;
    std::vector<float> lag_errors;
};

void updateFilterMetrics(HandFilter* filter,
                         const VisionCursor::HandLandmarkArray& raw_landmarks,
                         FilterMetrics& metrics,
                         VisionCursor::HandLandmark& prev_filtered_point,
                         std::uint8_t& has_prev_filtered_point,
                         std::size_t landmark_index)
{
    auto filtered_landmarks = filter->process(raw_landmarks);

    const auto& raw_point = raw_landmarks[landmark_index];
    const auto& filtered_point = filtered_landmarks[landmark_index];

    if (!raw_point.has_normalized || !filtered_point.has_normalized)
    {
        return;
    }

    metrics.valid_frame_count++;

    if (has_prev_filtered_point != 0)
    {
        metrics.step_distances.push_back(VisionCursor::Tests::distance2D(filtered_point, prev_filtered_point));
    }

    metrics.lag_errors.push_back(VisionCursor::Tests::distance2D(filtered_point, raw_point));

    prev_filtered_point = filtered_point;
    has_prev_filtered_point = 1;
}

void printFilterMetrics(const FilterMetrics& metrics)
{
    const float step_mean = VisionCursor::Utils::mean(metrics.step_distances);
    const float step_std = VisionCursor::Utils::stddev(metrics.step_distances, step_mean);
    const float lag_mean = VisionCursor::Utils::mean(metrics.lag_errors);
    const float lag_std = VisionCursor::Utils::stddev(metrics.lag_errors, lag_mean);

    std::cout << std::left << std::setw(24) << metrics.name << " | valid frames: " << std::setw(4)
              << metrics.valid_frame_count << " | mean step: " << std::setw(12) << std::fixed << std::setprecision(6)
              << step_mean << " | step stddev: " << std::setw(12) << std::fixed << std::setprecision(6) << step_std
              << " | mean lag error: " << std::setw(12) << std::fixed << std::setprecision(6) << lag_mean
              << " | lag stddev: " << std::setw(12) << std::fixed << std::setprecision(6) << lag_std << '\n';
}

std::vector<FilterMetrics> createMetricsList(const std::vector<std::string>& filter_names)
{
    std::vector<FilterMetrics> metrics_list;
    metrics_list.reserve(filter_names.size());

    for (const auto& name : filter_names)
    {
        FilterMetrics metrics;
        metrics.name = name;
        metrics_list.push_back(metrics);
    }

    return metrics_list;
}

void mergeMetrics(std::vector<FilterMetrics>& total_metrics, const std::vector<FilterMetrics>& current_metrics)
{
    ASSERT_EQ(total_metrics.size(), current_metrics.size());

    for (size_t i = 0; i < total_metrics.size(); ++i)
    {
        total_metrics[i].valid_frame_count += current_metrics[i].valid_frame_count;

        total_metrics[i].step_distances.insert(total_metrics[i].step_distances.end(),
                                               current_metrics[i].step_distances.begin(),
                                               current_metrics[i].step_distances.end());

        total_metrics[i].lag_errors.insert(total_metrics[i].lag_errors.end(),
                                           current_metrics[i].lag_errors.begin(),
                                           current_metrics[i].lag_errors.end());
    }
}

std::vector<FilterMetrics>
runSingleVideoBenchmark(std::shared_ptr<VisionCursor::MediaPipe::MediaPipeHandDetector> detector,
                        const std::vector<HandFilter*>& filters,
                        const std::vector<std::string>& filter_names,
                        const std::string& video_path,
                        std::size_t landmark_index)
{
    using namespace VisionCursor;

    if (filters.size() != filter_names.size())
    {
        std::cerr << "filters.size() != filter_names.size()" << std::endl;
        return {};
    }

    cv::VideoCapture cap(video_path);
    if (!cap.isOpened())
    {
        std::cerr << "Failed to open video: " << video_path << std::endl;
        return {};
    }

    std::vector<FilterMetrics> metrics_list = createMetricsList(filter_names);
    std::vector<HandLandmark> prev_points(filters.size());
    std::vector<std::uint8_t> has_prev_points(filters.size(), 0);

    while (true)
    {
        cv::Mat mat;
        if (!cap.read(mat))
        {
            break;
        }

        if (mat.empty())
        {
            continue;
        }

        cv::Mat rgb;
        cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);

        FramePtr frame = std::make_shared<Vision::OpenCVFrame>(rgb);

        HandLandmarkArray raw_landmarks;
        if (!detector->process(frame, raw_landmarks) || !raw_landmarks.isValid())
        {
            continue;
        }

        for (size_t i = 0; i < filters.size(); ++i)
        {
            updateFilterMetrics(filters[i],
                                raw_landmarks,
                                metrics_list[i],
                                prev_points[i],
                                has_prev_points[i],
                                landmark_index);
        }
    }

    return metrics_list;
}

void printMetricsGroup(const std::string& title,
                       const std::string& video_label,
                       const std::string& video_path,
                       std::size_t landmark_index,
                       const std::vector<FilterMetrics>& metrics_list)
{
    std::cout << "\n========== " << title << " ==========\n";
    if (!video_label.empty())
    {
        std::cout << "Case: " << video_label << "\n";
    }
    if (!video_path.empty())
    {
        std::cout << "Video: " << video_path << "\n";
    }
    std::cout << "Landmark index: " << landmark_index << "\n\n";

    for (const auto& metrics : metrics_list)
    {
        printFilterMetrics(metrics);
    }

    std::cout << "===============================================\n";
}

void runMultiVideoBenchmark(std::shared_ptr<VisionCursor::MediaPipe::MediaPipeHandDetector> detector,
                            const std::vector<HandFilter*>& filters,
                            const std::vector<std::string>& filter_names,
                            const std::vector<VisionCursor::Tests::VideoTestCase>& video_cases,
                            const std::string& benchmark_title,
                            std::size_t landmark_index)
{
    ASSERT_FALSE(filters.empty());
    ASSERT_EQ(filters.size(), filter_names.size());

    std::vector<FilterMetrics> total_metrics = createMetricsList(filter_names);

    for (const auto& video_case : video_cases)
    {
        for (auto* filter : filters)
        {
            filter->reset();
        }

        std::vector<FilterMetrics> current_metrics =
            runSingleVideoBenchmark(detector, filters, filter_names, video_case.path, landmark_index);

        ASSERT_FALSE(current_metrics.empty()) << "Failed benchmark case: " << video_case.path;

        printMetricsGroup(benchmark_title + " - Single Video",
                          video_case.label,
                          video_case.path,
                          landmark_index,
                          current_metrics);

        mergeMetrics(total_metrics, current_metrics);
    }

    printMetricsGroup(benchmark_title + " - Overall Summary", "", "", landmark_index, total_metrics);
}

} // namespace

TEST_F(VisionCursorTestFixture, SMAFilterMultiVideoQuantitativeTest)
{
    constexpr std::size_t LANDMARK_INDEX = 8;
    constexpr std::size_t MAX_WINDOW_SIZE = 5;

    std::vector<SMAFilter> filters;
    std::vector<HandFilter*> filter_ptrs;
    std::vector<std::string> filter_names;

    filters.reserve(MAX_WINDOW_SIZE);
    filter_ptrs.reserve(MAX_WINDOW_SIZE);
    filter_names.reserve(MAX_WINDOW_SIZE);

    for (std::size_t i = 0; i < MAX_WINDOW_SIZE; ++i)
    {
        filters.emplace_back(i + 1);
    }

    for (std::size_t i = 0; i < MAX_WINDOW_SIZE; ++i)
    {
        filter_ptrs.push_back(&filters[i]);
        filter_names.push_back("SMA(window=" + std::to_string(i + 1) + ")");
    }

    runMultiVideoBenchmark(mp_hand_detector,
                           filter_ptrs,
                           filter_names,
                           VisionCursor::Tests::trajectoryVideoCases(),
                           "SMA Filter Quantitative Result",
                           LANDMARK_INDEX);
}

TEST_F(VisionCursorTestFixture, EMAFilterMultiVideoQuantitativeTest)
{
    constexpr std::size_t LANDMARK_INDEX = 8;
    const std::vector<float> alphas = {0.2f, 0.4f, 0.6f, 0.8f};

    std::vector<EMAFilter> filters;
    std::vector<HandFilter*> filter_ptrs;
    std::vector<std::string> filter_names;

    filters.reserve(alphas.size());
    filter_ptrs.reserve(alphas.size());
    filter_names.reserve(alphas.size());

    for (float alpha : alphas)
    {
        filters.emplace_back(alpha);
    }

    for (std::size_t i = 0; i < alphas.size(); ++i)
    {
        filter_ptrs.push_back(&filters[i]);
        filter_names.push_back("EMA(alpha=" + std::to_string(alphas[i]) + ")");
    }

    runMultiVideoBenchmark(mp_hand_detector,
                           filter_ptrs,
                           filter_names,
                           VisionCursor::Tests::trajectoryVideoCases(),
                           "EMA Filter Quantitative Result",
                           LANDMARK_INDEX);
}

TEST_F(VisionCursorTestFixture, OneEuroFilterMinCutoffMultiVideoQuantitativeTest)
{
    constexpr std::size_t LANDMARK_INDEX = 8;

    constexpr float BETA = 20.0f;
    constexpr float D_CUTOFF = 1.0f;
    constexpr float FREQUENCY = 60.0f;

    const std::vector<float> min_cutoffs = {5.0f, 8.0f, 12.0f, 16.0f};

    std::vector<OneEuroFilter> filters;
    std::vector<HandFilter*> filter_ptrs;
    std::vector<std::string> filter_names;

    filters.reserve(min_cutoffs.size());
    filter_ptrs.reserve(min_cutoffs.size());
    filter_names.reserve(min_cutoffs.size());

    for (float min_cutoff : min_cutoffs)
    {
        filters.emplace_back(min_cutoff, BETA, D_CUTOFF, FREQUENCY);
    }

    for (std::size_t i = 0; i < min_cutoffs.size(); ++i)
    {
        filter_ptrs.push_back(&filters[i]);
        filter_names.push_back("OneEuro(minCutoff=" + std::to_string(min_cutoffs[i]) +
                              ", beta=" + std::to_string(BETA) +
                              ", dCutoff=" + std::to_string(D_CUTOFF) + ")");
    }

    runMultiVideoBenchmark(mp_hand_detector,
                           filter_ptrs,
                           filter_names,
                           VisionCursor::Tests::trajectoryVideoCases(),
                           "OneEuro Filter MinCutoff Quantitative Result",
                           LANDMARK_INDEX);
}

TEST_F(VisionCursorTestFixture, OneEuroFilterBetaMultiVideoQuantitativeTest)
{
    constexpr std::size_t LANDMARK_INDEX = 8;

    constexpr float MIN_CUTOFF = 12.0f;
    constexpr float D_CUTOFF = 1.0f;
    constexpr float FREQUENCY = 60.0f;

    const std::vector<float> betas = {5.0f, 10.0f, 20.0f, 30.0f, 40.0f};

    std::vector<OneEuroFilter> filters;
    std::vector<HandFilter*> filter_ptrs;
    std::vector<std::string> filter_names;

    filters.reserve(betas.size());
    filter_ptrs.reserve(betas.size());
    filter_names.reserve(betas.size());

    for (float beta : betas)
    {
        filters.emplace_back(MIN_CUTOFF, beta, D_CUTOFF, FREQUENCY);
    }

    for (std::size_t i = 0; i < betas.size(); ++i)
    {
        filter_ptrs.push_back(&filters[i]);
        filter_names.push_back("OneEuro(minCutoff=" + std::to_string(MIN_CUTOFF) +
                              ", beta=" + std::to_string(betas[i]) +
                              ", dCutoff=" + std::to_string(D_CUTOFF) + ")");
    }

    runMultiVideoBenchmark(mp_hand_detector,
                           filter_ptrs,
                           filter_names,
                           VisionCursor::Tests::trajectoryVideoCases(),
                           "OneEuro Filter Beta Quantitative Result",
                           LANDMARK_INDEX);
}
