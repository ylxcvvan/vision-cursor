#include "utils/visualizer.h"

#include <iomanip>
#include <iostream>
#include <unordered_map>

#include "common/constants.h"
#include "vision/opencv_frame.h"

namespace VisionCursor
{
namespace Utils
{

void draw_landmarks(FramePtr& frame, const HandLandmarkArray& arr)
{
    if (!frame)
    {
        std::cerr << "showFrame: frame is null\n";
        return;
    }

    auto cv_frame = std::dynamic_pointer_cast<Vision::OpenCVFrame>(frame);
    if (!cv_frame)
    {
        std::cerr << "showFrame: frame is not OpenCVFrame\n";
        return;
    }

    cv::Mat& mat = cv_frame->Mat();

    for (const auto& c : ARR_HAND_CONNECTIONS)
    {
        const auto& p1 = arr[c[0]];
        const auto& p2 = arr[c[1]];

        if (!p1.has_normalized || !p2.has_normalized)
        {
            continue;
        }

        const float x1 = mat.cols * p1.normalized.x;
        const float y1 = mat.rows * p1.normalized.y;
        const float x2 = mat.cols * p2.normalized.x;
        const float y2 = mat.rows * p2.normalized.y;

        cv::line(mat,
                 {static_cast<int>(x1), static_cast<int>(y1)},
                 {static_cast<int>(x2), static_cast<int>(y2)},
                 CV_RGB(0, 255, 0),
                 2);
    }

    for (std::size_t i = 0; i < HAND_LANDMARK_COUNT; ++i)
    {
        const auto& p = arr[i];
        if (!p.has_normalized)
        {
            continue;
        }

        const float x = mat.cols * p.normalized.x;
        const float y = mat.rows * p.normalized.y;

        cv::circle(mat, cv::Point(static_cast<int>(x), static_cast<int>(y)), 4, CV_RGB(255, 0, 0), -1);
    }
}

void drawControlJointHighlight(FramePtr& frame, const HandLandmarkArray& arr, HandJoint control_joint)
{
    if (!frame || !arr.isValid())
    {
        return;
    }

    auto cv_frame = std::dynamic_pointer_cast<Vision::OpenCVFrame>(frame);
    if (!cv_frame)
    {
        return;
    }

    cv::Mat& mat = cv_frame->Mat();
    if (mat.empty())
    {
        return;
    }

    const HandLandmark& p = arr[control_joint];
    if (!p.has_normalized)
    {
        return;
    }

    const int x = static_cast<int>(p.normalized.x * mat.cols);
    const int y = static_cast<int>(p.normalized.y * mat.rows);

    cv::circle(mat, cv::Point(x, y), 10, cv::Scalar(255, 255, 0), 2);
    cv::circle(mat, cv::Point(x, y), 4, cv::Scalar(255, 0, 255), -1);
}

void drawMapperRegion(FramePtr& frame, const MappingRegion& region)
{
    if (!frame)
    {
        return;
    }

    auto cv_frame = std::dynamic_pointer_cast<Vision::OpenCVFrame>(frame);
    if (!cv_frame)
    {
        return;
    }

    cv::Mat& mat = cv_frame->Mat();
    if (mat.empty())
    {
        return;
    }

    const int x1 = static_cast<int>(region.left * mat.cols);
    const int y1 = static_cast<int>(region.top * mat.rows);
    const int x2 = static_cast<int>(region.right * mat.cols);
    const int y2 = static_cast<int>(region.bottom * mat.rows);

    cv::rectangle(mat, cv::Point(x1, y1), cv::Point(x2, y2), cv::Scalar(255, 255, 0), 2);
}

void cout_landmarks(const HandLandmarkArray& landmarks)
{
    using std::cout;
    using std::endl;
    using std::fixed;
    using std::setprecision;
    using std::setw;

    cout << fixed << setprecision(4);

    auto print_joint = [&](std::size_t idx, const std::string& name) {
        const auto& p = landmarks[idx];

        cout << setw(15) << name << " | "
             << "N(" << setw(8) << p.normalized.x << ", " << setw(8) << p.normalized.y << ", " << setw(8)
             << p.normalized.z << ") "
             << "W(" << setw(8) << p.world.x << ", " << setw(8) << p.world.y << ", " << setw(8)
             << p.world.z << ") "
             << "flags[n=" << (p.has_normalized ? 1 : 0) << ",w=" << (p.has_world ? 1 : 0) << "]"
             << endl;
    };

    cout << "\n================ Hand Landmarks ================\n";
    print_joint(0, "WRIST");
    print_joint(1, "THUMB_CMC");
    print_joint(2, "THUMB_MCP");
    print_joint(3, "THUMB_IP");
    print_joint(4, "THUMB_TIP");
    print_joint(5, "INDEX_MCP");
    print_joint(6, "INDEX_PIP");
    print_joint(7, "INDEX_DIP");
    print_joint(8, "INDEX_TIP");
    print_joint(9, "MIDDLE_MCP");
    print_joint(10, "MIDDLE_PIP");
    print_joint(11, "MIDDLE_DIP");
    print_joint(12, "MIDDLE_TIP");
    print_joint(13, "RING_MCP");
    print_joint(14, "RING_PIP");
    print_joint(15, "RING_DIP");
    print_joint(16, "RING_TIP");
    print_joint(17, "PINKY_MCP");
    print_joint(18, "PINKY_PIP");
    print_joint(19, "PINKY_DIP");
    print_joint(20, "PINKY_TIP");
    cout << "================================================\n";
}

void show_frame(const FramePtr& frame, const std::string& window_name)
{
    static std::unordered_map<std::string, bool> window_created;
    static int window_index = 0;

    const int window_width = 640;
    const int window_height = 480;
    const int cols = 3;

    if (!frame)
    {
        std::cerr << "showFrame: frame is null\n";
        return;
    }

    auto cv_frame = std::dynamic_pointer_cast<Vision::OpenCVFrame>(frame);
    if (!cv_frame)
    {
        std::cerr << "showFrame: frame is not OpenCVFrame\n";
        return;
    }

    cv::Mat rawMat = cv_frame->CloneMat();
    if (rawMat.empty())
    {
        std::cerr << "showFrame: rawMat is empty\n";
        return;
    }

    cv::Mat mat;
    cv::cvtColor(rawMat, mat, cv::COLOR_BGR2RGB);

    if (window_created.find(window_name) == window_created.end())
    {
        cv::namedWindow(window_name, cv::WINDOW_NORMAL);

        const int row = window_index / cols;
        const int col = window_index % cols;

        const int x = col * window_width;
        const int y = row * window_height;

        cv::moveWindow(window_name, x, y);

        window_created[window_name] = true;
        window_index++;
    }

    cv::imshow(window_name, mat);
    cv::waitKey(1);
}

} // namespace Utils
} // namespace VisionCursor