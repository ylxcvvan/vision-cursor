#include <gtest/gtest.h>

#include <algorithm>
#include <filesystem>
#include <iostream>
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

using namespace VisionCursor;
using namespace VisionCursor::Algorithm;

TEST_F(VisionCursorTestFixture, CaptureAndMark)
{
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);

    for (int i = 0; i < 180; ++i)
    {
        FramePtr frame;
        if (!p_camera_device->capture(frame) || frame == nullptr)
        {
            GTEST_LOG_(WARNING) << "capture failed at frame " << i;
            break;
        }

        HandLandmarkArray arr_raw_landmark;
        if (!mp_hand_detector->process(frame, arr_raw_landmark))
        {
            GTEST_LOG_(WARNING) << "mediapipe process failed at frame " << i;
            continue;
        }

        Utils::draw_landmarks(frame, arr_raw_landmark);
        Utils::show_frame(frame, "capture frame");

        if (cv::waitKey(1) == 27)
        {
            break;
        }
    }
}

TEST_F(VisionCursorTestFixture, SMAFilterWindowCompare)
{
    std::vector<SMAFilter> vec_filter;

    constexpr size_t MAX_WINDOW_SIZE = 5;
    for (size_t i = 0; i < MAX_WINDOW_SIZE; ++i)
    {
        vec_filter.emplace_back(i + 1);
    }

    for (int frame_idx = 0; frame_idx < 600; ++frame_idx)
    {
        FramePtr frame;
        if (!p_camera_device->capture(frame) || frame == nullptr)
        {
            GTEST_LOG_(WARNING) << "capture failed at frame " << frame_idx;
            break;
        }

        HandLandmarkArray arr_raw_landmark;
        if (!mp_hand_detector->process(frame, arr_raw_landmark))
        {
            GTEST_LOG_(WARNING) << "mediapipe process failed at frame " << frame_idx;
            continue;
        }

        for (size_t i = 0; i < MAX_WINDOW_SIZE; ++i)
        {
            auto arr_sma_landmark = vec_filter[i].process(arr_raw_landmark);
            auto sma_frame = frame->clone();

            const std::string window_name = "SMA raw landmarks, window size: " + std::to_string(i + 1);
            Utils::draw_landmarks(sma_frame, arr_sma_landmark);
            Utils::show_frame(sma_frame, window_name);
        }

        Utils::draw_landmarks(frame, arr_raw_landmark);
        Utils::show_frame(frame, "raw landmarks");

        if (cv::waitKey(1) == 27)
        {
            break;
        }
    }
}

TEST_F(VisionCursorTestFixture, EMAFilterAlphaCompare)
{
    std::vector<EMAFilter> vec_filter;

    const std::vector<float> alphas = {0.2f, 0.4f, 0.6f, 0.8f};
    for (float alpha : alphas)
    {
        vec_filter.emplace_back(alpha);
    }

    for (int frame_idx = 0; frame_idx < 600; ++frame_idx)
    {
        FramePtr frame;
        if (!p_camera_device->capture(frame) || frame == nullptr)
        {
            GTEST_LOG_(WARNING) << "capture failed at frame " << frame_idx;
            break;
        }

        HandLandmarkArray arr_raw_landmark;
        if (!mp_hand_detector->process(frame, arr_raw_landmark))
        {
            GTEST_LOG_(WARNING) << "mediapipe process failed at frame " << frame_idx;
            continue;
        }

        for (size_t i = 0; i < vec_filter.size(); ++i)
        {
            auto arr_ema_landmark = vec_filter[i].process(arr_raw_landmark);
            auto ema_frame = frame->clone();

            const std::string window_name = "EMA raw landmarks, alpha: " + std::to_string(alphas[i]);
            Utils::draw_landmarks(ema_frame, arr_ema_landmark);
            Utils::show_frame(ema_frame, window_name);
        }

        Utils::draw_landmarks(frame, arr_raw_landmark);
        Utils::show_frame(frame, "raw landmarks");

        if (cv::waitKey(1) == 27)
        {
            break;
        }
    }
}

TEST_F(VisionCursorTestFixture, OneEuroFilterMinCutoffCompare)
{
    constexpr float BETA = 20.0f;
    constexpr float D_CUTOFF = 1.0f;
    constexpr float FREQUENCY = 60.0f;

    const std::vector<float> min_cutoffs = {5.0f, 8.0f, 12.0f, 16.0f};
    std::vector<OneEuroFilter> vec_filter;

    for (float min_cutoff : min_cutoffs)
    {
        vec_filter.emplace_back(min_cutoff, BETA, D_CUTOFF, FREQUENCY);
    }

    for (int frame_idx = 0; frame_idx < 600; ++frame_idx)
    {
        FramePtr frame;
        if (!p_camera_device->capture(frame) || frame == nullptr)
        {
            GTEST_LOG_(WARNING) << "capture failed at frame " << frame_idx;
            break;
        }

        HandLandmarkArray arr_raw_landmark;
        if (!mp_hand_detector->process(frame, arr_raw_landmark))
        {
            GTEST_LOG_(WARNING) << "mediapipe process failed at frame " << frame_idx;
            continue;
        }

        for (size_t i = 0; i < vec_filter.size(); ++i)
        {
            auto arr_one_euro_landmark = vec_filter[i].process(arr_raw_landmark);
            auto one_euro_frame = frame->clone();

            const std::string window_name = "OneEuro minCutoff: " + std::to_string(min_cutoffs[i]);
            Utils::draw_landmarks(one_euro_frame, arr_one_euro_landmark);
            Utils::show_frame(one_euro_frame, window_name);
        }

        Utils::draw_landmarks(frame, arr_raw_landmark);
        Utils::show_frame(frame, "raw landmarks");

        if (cv::waitKey(1) == 27)
        {
            break;
        }
    }
}

TEST_F(VisionCursorTestFixture, OneEuroFilterBetaCompare)
{
    constexpr float MIN_CUTOFF = 12.0f;
    constexpr float D_CUTOFF = 1.0f;
    constexpr float FREQUENCY = 60.0f;

    const std::vector<float> betas = {5.0f, 10.0f, 20.0f, 30.0f, 40.0f};
    std::vector<OneEuroFilter> vec_filter;

    for (float beta : betas)
    {
        vec_filter.emplace_back(MIN_CUTOFF, beta, D_CUTOFF, FREQUENCY);
    }

    for (int frame_idx = 0; frame_idx < 600; ++frame_idx)
    {
        FramePtr frame;
        if (!p_camera_device->capture(frame) || frame == nullptr)
        {
            GTEST_LOG_(WARNING) << "capture failed at frame " << frame_idx;
            break;
        }

        HandLandmarkArray arr_raw_landmark;
        if (!mp_hand_detector->process(frame, arr_raw_landmark))
        {
            GTEST_LOG_(WARNING) << "mediapipe process failed at frame " << frame_idx;
            continue;
        }

        for (size_t i = 0; i < vec_filter.size(); ++i)
        {
            auto arr_one_euro_landmark = vec_filter[i].process(arr_raw_landmark);
            auto one_euro_frame = frame->clone();

            const std::string window_name = "OneEuro raw landmarks, beta: " + std::to_string(betas[i]);
            Utils::draw_landmarks(one_euro_frame, arr_one_euro_landmark);
            Utils::show_frame(one_euro_frame, window_name);
        }

        Utils::draw_landmarks(frame, arr_raw_landmark);
        Utils::show_frame(frame, "raw landmarks");

        if (cv::waitKey(1) == 27)
        {
            break;
        }
    }
}

namespace
{

cv::Point landmarkToPixel(const VisionCursor::HandLandmark& landmark, int canvas_width, int canvas_height)
{
    const int x = static_cast<int>(landmark.normalized.x * canvas_width);
    const int y = static_cast<int>(landmark.normalized.y * canvas_height);
    return cv::Point(x, y);
}

void drawLegend(cv::Mat& img)
{
    cv::circle(img, cv::Point(25, 25), 6, cv::Scalar(0, 255, 0), -1);
    cv::putText(img, "Raw", cv::Point(40, 30), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 128, 0), 2);

    cv::circle(img, cv::Point(120, 25), 6, cv::Scalar(0, 0, 255), -1);
    cv::putText(img, "Filtered", cv::Point(135, 30), cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(0, 0, 255), 2);
}

void runIndexTrajectoryTest(HandFilter* filter,
                            const std::string& window_name,
                            const std::string& video_path,
                            std::shared_ptr<VisionCursor::MediaPipe::MediaPipeHandDetector> detector)
{
    using namespace VisionCursor;

    constexpr size_t LANDMARK_INDEX = 8; // Index TIP
    constexpr int PANEL_WIDTH = 640;
    constexpr int PANEL_HEIGHT = 480;

    cv::VideoCapture cap(video_path);
    ASSERT_TRUE(cap.isOpened()) << "Failed to open video: " << video_path;

    filter->reset();

    cv::namedWindow(window_name.c_str(), cv::WINDOW_NORMAL);
    cv::resizeWindow(window_name.c_str(), PANEL_WIDTH * 2, PANEL_HEIGHT);

    cv::Mat trajectory_canvas(PANEL_HEIGHT, PANEL_WIDTH, CV_8UC3, cv::Scalar(255, 255, 255));

    cv::Point prev_raw_pt(-1, -1);
    cv::Point prev_filtered_pt(-1, -1);

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
        if (!detector->process(frame, raw_landmarks))
        {
            continue;
        }

        if (!raw_landmarks[LANDMARK_INDEX].has_normalized)
        {
            continue;
        }

        HandLandmarkArray filtered_landmarks = filter->process(raw_landmarks);
        if (!filtered_landmarks[LANDMARK_INDEX].has_normalized)
        {
            continue;
        }

        const auto& raw_landmark = raw_landmarks[LANDMARK_INDEX];
        const auto& filtered_landmark = filtered_landmarks[LANDMARK_INDEX];

        cv::Point raw_traj_pt = landmarkToPixel(raw_landmark, trajectory_canvas.cols, trajectory_canvas.rows);
        cv::Point filtered_traj_pt = landmarkToPixel(filtered_landmark, trajectory_canvas.cols, trajectory_canvas.rows);

        if (prev_raw_pt.x >= 0 && prev_raw_pt.y >= 0)
        {
            cv::line(trajectory_canvas, prev_raw_pt, raw_traj_pt, cv::Scalar(0, 255, 0), 2);
        }

        if (prev_filtered_pt.x >= 0 && prev_filtered_pt.y >= 0)
        {
            cv::line(trajectory_canvas, prev_filtered_pt, filtered_traj_pt, cv::Scalar(0, 0, 255), 2);
        }

        prev_raw_pt = raw_traj_pt;
        prev_filtered_pt = filtered_traj_pt;

        cv::Mat display = mat.clone();

        cv::Point raw_video_pt = landmarkToPixel(raw_landmark, display.cols, display.rows);
        cv::Point filtered_video_pt = landmarkToPixel(filtered_landmark, display.cols, display.rows);

        cv::circle(display, raw_video_pt, 5, cv::Scalar(0, 255, 0), -1);
        cv::circle(display, filtered_video_pt, 5, cv::Scalar(0, 0, 255), -1);

        cv::Mat resized_display;
        cv::resize(display, resized_display, cv::Size(PANEL_WIDTH, PANEL_HEIGHT));

        cv::Mat trajectory_view = trajectory_canvas.clone();

        cv::putText(resized_display, "Video", cv::Point(20, 60), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(255, 255, 255), 2);
        cv::putText(trajectory_view, "Trajectory", cv::Point(20, 60), cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0, 0, 0), 2);

        drawLegend(resized_display);
        drawLegend(trajectory_view);

        cv::Mat combined;
        cv::hconcat(resized_display, trajectory_view, combined);

        cv::imshow(window_name.c_str(), combined);

        const int key = cv::waitKey(16);
        if (key == 27)
        {
            break;
        }
    }

    drawLegend(trajectory_canvas);
    cv::putText(trajectory_canvas, window_name, cv::Point(60, 60), cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 0, 0), 2);

    std::filesystem::create_directories("trajectory_output");

    std::string safe_name = window_name;
    std::replace(safe_name.begin(), safe_name.end(), '(', '_');
    std::replace(safe_name.begin(), safe_name.end(), ')', '_');
    std::replace(safe_name.begin(), safe_name.end(), '.', '_');
    std::replace(safe_name.begin(), safe_name.end(), ' ', '_');

    const std::string output_path = "trajectory_output/" + safe_name + "_trajectory.jpg";
    cv::imwrite(output_path, trajectory_canvas);

    std::cout << "Saved trajectory image: " << output_path << std::endl;

    cv::destroyWindow(window_name.c_str());
}

} // namespace

TEST_F(VisionCursorTestFixture, SMAIndexTrajectoryCircleVideo)
{
    SMAFilter filter(3);
    runIndexTrajectoryTest(&filter, "SMA Circle (window=3)", "test_data/tst_each_filter_circle.mp4", mp_hand_detector);
}

TEST_F(VisionCursorTestFixture, SMAIndexTrajectoryRectangleVideo)
{
    SMAFilter filter(3);
    runIndexTrajectoryTest(&filter, "SMA Rectangle (window=3)", "test_data/tst_each_filter_rectangle.mp4", mp_hand_detector);
}

TEST_F(VisionCursorTestFixture, SMAIndexTrajectoryHorizontalVideo)
{
    SMAFilter filter(3);
    runIndexTrajectoryTest(&filter, "SMA Horizontal (window=3)", "test_data/tst_each_filter_horizontal.mp4", mp_hand_detector);
}

TEST_F(VisionCursorTestFixture, EMAIndexTrajectoryCircleVideo)
{
    EMAFilter filter(0.6f);
    runIndexTrajectoryTest(&filter, "EMA Circle (alpha=0.6)", "test_data/tst_each_filter_circle.mp4", mp_hand_detector);
}

TEST_F(VisionCursorTestFixture, EMAIndexTrajectoryRectangleVideo)
{
    EMAFilter filter(0.6f);
    runIndexTrajectoryTest(&filter, "EMA Rectangle (alpha=0.6)", "test_data/tst_each_filter_rectangle.mp4", mp_hand_detector);
}

TEST_F(VisionCursorTestFixture, EMAIndexTrajectoryHorizontalVideo)
{
    EMAFilter filter(0.6f);
    runIndexTrajectoryTest(&filter, "EMA Horizontal (alpha=0.6)", "test_data/tst_each_filter_horizontal.mp4", mp_hand_detector);
}
