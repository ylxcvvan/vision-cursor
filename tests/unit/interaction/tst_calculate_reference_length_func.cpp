#include "vision_cursor_test_fixture.h"

#include <algorithm>
#include <cmath>
#include <functional>
#include <iomanip>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

#include <opencv2/opencv.hpp>

#include "algorithm/filter/one_euro_filter.hpp"
#include "utils/math.hpp"
#include "utils/visualizer.h"
#include "vision/opencv_frame.h"

using namespace VisionCursor;

namespace
{

struct ReferenceFunctionEntry
{
    std::string name;
    std::function<float(const HandLandmarkArray&)> func;
};

bool hasJoint(const HandLandmarkArray& landmarks, HandJoint joint)
{
    return landmarks[joint].isValid();
}

bool hasJoints(const HandLandmarkArray& landmarks, std::initializer_list<HandJoint> joints)
{
    for (const HandJoint joint : joints)
    {
        if (!hasJoint(landmarks, joint))
        {
            return false;
        }
    }
    return true;
}

Landmark pickPoint(const HandLandmark& point)
{
    if (point.has_normalized)
    {
        return point.normalized;
    }

    if (point.has_world)
    {
        return point.world;
    }

    return {};
}

float distance3DByAvailableSpace(const HandLandmark& a, const HandLandmark& b)
{
    const Landmark pa = pickPoint(a);
    const Landmark pb = pickPoint(b);
    return Utils::distance3D(pa, pb);
}

float distance2DByAvailableSpace(const HandLandmark& a, const HandLandmark& b)
{
    const Landmark pa = pickPoint(a);
    const Landmark pb = pickPoint(b);
    const float dx = pa.x - pb.x;
    const float dy = pa.y - pb.y;
    return std::sqrt(dx * dx + dy * dy);
}

float triangleArea3DByAvailableSpace(const HandLandmark& a, const HandLandmark& b, const HandLandmark& c)
{
    const Landmark pa = pickPoint(a);
    const Landmark pb = pickPoint(b);
    const Landmark pc = pickPoint(c);
    return Utils::triangleArea3D(pa, pb, pc);
}

float triangleArea2DByAvailableSpace(const HandLandmark& a, const HandLandmark& b, const HandLandmark& c)
{
    const Landmark pa = pickPoint(a);
    const Landmark pb = pickPoint(b);
    const Landmark pc = pickPoint(c);
    return Utils::triangleArea2D(pa, pb, pc);
}

float refPalmWidth3D(const HandLandmarkArray& landmarks)
{
    if (!hasJoints(landmarks, {HandJoint::IndexMCP, HandJoint::PinkyMCP}))
    {
        return 0.0f;
    }

    return distance3DByAvailableSpace(landmarks[HandJoint::IndexMCP], landmarks[HandJoint::PinkyMCP]);
}

float refPalmWidth2D(const HandLandmarkArray& landmarks)
{
    if (!hasJoints(landmarks, {HandJoint::IndexMCP, HandJoint::PinkyMCP}))
    {
        return 0.0f;
    }

    return distance2DByAvailableSpace(landmarks[HandJoint::IndexMCP], landmarks[HandJoint::PinkyMCP]);
}

float refTriangleMean3D(const HandLandmarkArray& landmarks)
{
    if (!hasJoints(landmarks, {HandJoint::Wrist, HandJoint::IndexMCP, HandJoint::PinkyMCP}))
    {
        return 0.0f;
    }

    const auto& wrist = landmarks[HandJoint::Wrist];
    const auto& index_mcp = landmarks[HandJoint::IndexMCP];
    const auto& pinky_mcp = landmarks[HandJoint::PinkyMCP];

    const float d1 = distance3DByAvailableSpace(wrist, index_mcp);
    const float d2 = distance3DByAvailableSpace(wrist, pinky_mcp);
    const float d3 = distance3DByAvailableSpace(index_mcp, pinky_mcp);

    return (d1 + d2 + d3) / 3.0f;
}

float refTriangleMean2D(const HandLandmarkArray& landmarks)
{
    if (!hasJoints(landmarks, {HandJoint::Wrist, HandJoint::IndexMCP, HandJoint::PinkyMCP}))
    {
        return 0.0f;
    }

    const auto& wrist = landmarks[HandJoint::Wrist];
    const auto& index_mcp = landmarks[HandJoint::IndexMCP];
    const auto& pinky_mcp = landmarks[HandJoint::PinkyMCP];

    const float d1 = distance2DByAvailableSpace(wrist, index_mcp);
    const float d2 = distance2DByAvailableSpace(wrist, pinky_mcp);
    const float d3 = distance2DByAvailableSpace(index_mcp, pinky_mcp);

    return (d1 + d2 + d3) / 3.0f;
}

float refPalmMultiEdge3D(const HandLandmarkArray& landmarks)
{
    if (!hasJoints(
            landmarks, {HandJoint::Wrist, HandJoint::IndexMCP, HandJoint::MiddleMCP, HandJoint::RingMCP, HandJoint::PinkyMCP}))
    {
        return 0.0f;
    }

    const auto& wrist = landmarks[HandJoint::Wrist];
    const auto& index = landmarks[HandJoint::IndexMCP];
    const auto& middle = landmarks[HandJoint::MiddleMCP];
    const auto& ring = landmarks[HandJoint::RingMCP];
    const auto& pinky = landmarks[HandJoint::PinkyMCP];

    const float d1 = distance3DByAvailableSpace(wrist, index);
    const float d2 = distance3DByAvailableSpace(wrist, middle);
    const float d3 = distance3DByAvailableSpace(wrist, ring);
    const float d4 = distance3DByAvailableSpace(wrist, pinky);
    const float d5 = distance3DByAvailableSpace(index, pinky);

    return (d1 + d2 + d3 + d4 + d5) / 5.0f;
}

float refPalmMultiEdge2D(const HandLandmarkArray& landmarks)
{
    if (!hasJoints(
            landmarks, {HandJoint::Wrist, HandJoint::IndexMCP, HandJoint::MiddleMCP, HandJoint::RingMCP, HandJoint::PinkyMCP}))
    {
        return 0.0f;
    }

    const auto& wrist = landmarks[HandJoint::Wrist];
    const auto& index = landmarks[HandJoint::IndexMCP];
    const auto& middle = landmarks[HandJoint::MiddleMCP];
    const auto& ring = landmarks[HandJoint::RingMCP];
    const auto& pinky = landmarks[HandJoint::PinkyMCP];

    const float d1 = distance2DByAvailableSpace(wrist, index);
    const float d2 = distance2DByAvailableSpace(wrist, middle);
    const float d3 = distance2DByAvailableSpace(wrist, ring);
    const float d4 = distance2DByAvailableSpace(wrist, pinky);
    const float d5 = distance2DByAvailableSpace(index, pinky);

    return (d1 + d2 + d3 + d4 + d5) / 5.0f;
}

float refTriangleAreaSqrt3D(const HandLandmarkArray& landmarks)
{
    if (!hasJoints(landmarks, {HandJoint::Wrist, HandJoint::IndexMCP, HandJoint::PinkyMCP}))
    {
        return 0.0f;
    }

    const auto& wrist = landmarks[HandJoint::Wrist];
    const auto& index_mcp = landmarks[HandJoint::IndexMCP];
    const auto& pinky_mcp = landmarks[HandJoint::PinkyMCP];

    return std::sqrt(std::max(triangleArea3DByAvailableSpace(wrist, index_mcp, pinky_mcp), 0.0f));
}

float refTriangleAreaSqrt2D(const HandLandmarkArray& landmarks)
{
    if (!hasJoints(landmarks, {HandJoint::Wrist, HandJoint::IndexMCP, HandJoint::PinkyMCP}))
    {
        return 0.0f;
    }

    const auto& wrist = landmarks[HandJoint::Wrist];
    const auto& index_mcp = landmarks[HandJoint::IndexMCP];
    const auto& pinky_mcp = landmarks[HandJoint::PinkyMCP];

    return std::sqrt(std::max(triangleArea2DByAvailableSpace(wrist, index_mcp, pinky_mcp), 0.0f));
}

float refPalmDoubleTriangleAreaSqrt3D(const HandLandmarkArray& landmarks)
{
    if (!hasJoints(landmarks, {HandJoint::Wrist, HandJoint::IndexMCP, HandJoint::MiddleMCP, HandJoint::PinkyMCP}))
    {
        return 0.0f;
    }

    return Utils::palmAreaScale3D(pickPoint(landmarks[HandJoint::Wrist]),
                                  pickPoint(landmarks[HandJoint::IndexMCP]),
                                  pickPoint(landmarks[HandJoint::MiddleMCP]),
                                  pickPoint(landmarks[HandJoint::PinkyMCP]));
}

float refPalmPrincipalAxisScale3D(const HandLandmarkArray& landmarks)
{
    if (!hasJoints(landmarks, {HandJoint::Wrist, HandJoint::IndexMCP, HandJoint::MiddleMCP, HandJoint::PinkyMCP}))
    {
        return 0.0f;
    }

    return Utils::palmPrincipalAxisScale3D(pickPoint(landmarks[HandJoint::Wrist]),
                                           pickPoint(landmarks[HandJoint::IndexMCP]),
                                           pickPoint(landmarks[HandJoint::MiddleMCP]),
                                           pickPoint(landmarks[HandJoint::PinkyMCP]));
}

float refPalmRobustEdgeScale3D(const HandLandmarkArray& landmarks)
{
    if (!hasJoints(
            landmarks, {HandJoint::Wrist, HandJoint::IndexMCP, HandJoint::MiddleMCP, HandJoint::RingMCP, HandJoint::PinkyMCP}))
    {
        return 0.0f;
    }

    return Utils::palmRobustEdgeScale3D(pickPoint(landmarks[HandJoint::Wrist]),
                                        pickPoint(landmarks[HandJoint::IndexMCP]),
                                        pickPoint(landmarks[HandJoint::MiddleMCP]),
                                        pickPoint(landmarks[HandJoint::RingMCP]),
                                        pickPoint(landmarks[HandJoint::PinkyMCP]));
}

void printPrettySummary(const std::vector<ReferenceFunctionEntry>& ref_functions,
                        const std::vector<std::vector<float>>& all_values)
{
    std::cout << "\n\n==================== Reference Length Stability Summary ====================\n";
    std::cout << std::left << std::setw(42) << "Function" << std::setw(10) << "Count" << std::setw(14) << "Mean"
              << std::setw(14) << "StdDev" << std::setw(14) << "CV" << std::setw(14) << "Min" << std::setw(14) << "Max"
              << "\n";

    std::cout << std::string(108, '-') << "\n";

    for (size_t i = 0; i < ref_functions.size(); ++i)
    {
        const auto& values = all_values[i];
        const float avg = Utils::mean(values);
        const float sd = Utils::stddev(values, avg);
        const float cv = Utils::coeffOfVariation(values);
        const float minv = Utils::minValue(values);
        const float maxv = Utils::maxValue(values);

        std::cout << std::left << std::setw(42) << ref_functions[i].name << std::setw(10) << values.size() << std::setw(14)
                  << std::fixed << std::setprecision(6) << avg << std::setw(14) << std::fixed << std::setprecision(6) << sd
                  << std::setw(14) << std::fixed << std::setprecision(6) << cv << std::setw(14) << std::fixed
                  << std::setprecision(6) << minv << std::setw(14) << std::fixed << std::setprecision(6) << maxv << "\n";
    }

    std::cout << std::string(108, '-') << "\n";

    float best_cv = std::numeric_limits<float>::infinity();
    std::string best_name;

    for (size_t i = 0; i < ref_functions.size(); ++i)
    {
        const float cv = Utils::coeffOfVariation(all_values[i]);
        if (cv < best_cv)
        {
            best_cv = cv;
            best_name = ref_functions[i].name;
        }
    }

    std::cout << "[Best Stability] " << best_name << " | CV = " << std::fixed << std::setprecision(6) << best_cv << std::endl;
}

} // namespace

// 实时采集 600 帧，使用 OneEuroFilter 后比较不同 reference length 函数的稳定性。
TEST_F(VisionCursorTestFixture, LandmarkReferenceLengthRealtimeStability600Frames)
{
    Algorithm::OneEuroFilter filter;

    std::vector<ReferenceFunctionEntry> ref_functions = {
        {"PalmWidth3D_IndexMCP_PinkyMCP", refPalmWidth3D},
        {"PalmWidth2D_IndexMCP_PinkyMCP", refPalmWidth2D},
        {"TriangleMean3D_Wrist_IndexMCP_PinkyMCP", refTriangleMean3D},
        {"TriangleMean2D_Wrist_IndexMCP_PinkyMCP", refTriangleMean2D},
        {"PalmMultiEdge3D", refPalmMultiEdge3D},
        {"PalmMultiEdge2D", refPalmMultiEdge2D},
        {"TriangleAreaSqrt3D", refTriangleAreaSqrt3D},
        {"TriangleAreaSqrt2D", refTriangleAreaSqrt2D},
        {"PalmDoubleTriangleAreaSqrt3D", refPalmDoubleTriangleAreaSqrt3D},
        {"PalmPrincipalAxisScale3D", refPalmPrincipalAxisScale3D},
        {"PalmRobustEdgeScale3D", refPalmRobustEdgeScale3D},
    };

    std::vector<std::vector<float>> all_values(ref_functions.size());

    int valid_capture_count = 0;
    int valid_detect_count = 0;

    for (int i = 0; i < 600; ++i)
    {
        FramePtr frame;
        const bool captured = p_camera_device->capture(frame);
        ASSERT_TRUE(captured);
        ++valid_capture_count;

        HandLandmarkArray raw_landmarks;
        const bool detected = mp_hand_detector->process(frame, raw_landmarks);

        if (!detected || !raw_landmarks.isValid())
        {
            GTEST_LOG_(WARNING) << "[Frame " << i << "] invalid hand detection.";
            Utils::show_frame(frame, "Reference Length Stability Test");
            if (cv::waitKey(2) == 27)
            {
                break;
            }
            continue;
        }

        ++valid_detect_count;

        HandLandmarkArray filtered_landmarks = filter.process(raw_landmarks);
        if (!filtered_landmarks.isValid())
        {
            GTEST_LOG_(WARNING) << "[Frame " << i << "] filtered landmarks invalid.";
            Utils::show_frame(frame, "Reference Length Stability Test");
            if (cv::waitKey(2) == 27)
            {
                break;
            }
            continue;
        }

        for (size_t j = 0; j < ref_functions.size(); ++j)
        {
            all_values[j].push_back(ref_functions[j].func(filtered_landmarks));
        }

        Utils::draw_landmarks(frame, filtered_landmarks);

        auto opencv_frame = std::dynamic_pointer_cast<Vision::OpenCVFrame>(frame);
        ASSERT_NE(opencv_frame, nullptr);

        const std::string line1 = "Frame: " + std::to_string(i);
        const std::string line2 = "Captured: " + std::to_string(valid_capture_count);
        const std::string line3 = "Detected: " + std::to_string(valid_detect_count);

        cv::putText(
            opencv_frame->Mat(), line1, cv::Point(20, 50), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(0, 255, 0), 2);
        cv::putText(
            opencv_frame->Mat(), line2, cv::Point(20, 90), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(0, 255, 0), 2);
        cv::putText(
            opencv_frame->Mat(), line3, cv::Point(20, 130), cv::FONT_HERSHEY_SIMPLEX, 1.2, cv::Scalar(0, 255, 0), 2);

        Utils::show_frame(frame, "Reference Length Stability Test");
        if (cv::waitKey(2) == 27)
        {
            break;
        }
    }

    GTEST_LOG_(INFO) << "Captured frames: " << valid_capture_count << ", valid detected frames: " << valid_detect_count;
    EXPECT_GT(valid_capture_count, 0);

    printPrettySummary(ref_functions, all_values);
}
