#include <gtest/gtest.h>

#include <chrono>
#include <iostream>

#include <mediapipe.h>

#include <opencv2/core/utils/logger.hpp>
#include <opencv2/opencv.hpp>

#include "test_helpers.h"

using namespace VisionCursor;

namespace
{

const mp_hand_landmark CONNECTIONS[][2] = {
    {mp_hand_landmark_wrist, mp_hand_landmark_thumb_cmc},
    {mp_hand_landmark_thumb_cmc, mp_hand_landmark_thumb_mcp},
    {mp_hand_landmark_thumb_mcp, mp_hand_landmark_thumb_ip},
    {mp_hand_landmark_thumb_ip, mp_hand_landmark_thumb_tip},
    {mp_hand_landmark_wrist, mp_hand_landmark_index_finger_mcp},
    {mp_hand_landmark_index_finger_mcp, mp_hand_landmark_index_finger_pip},
    {mp_hand_landmark_index_finger_pip, mp_hand_landmark_index_finger_dip},
    {mp_hand_landmark_index_finger_dip, mp_hand_landmark_index_finger_tip},
    {mp_hand_landmark_index_finger_mcp, mp_hand_landmark_middle_finger_mcp},
    {mp_hand_landmark_middle_finger_mcp, mp_hand_landmark_middle_finger_pip},
    {mp_hand_landmark_middle_finger_pip, mp_hand_landmark_middle_finger_dip},
    {mp_hand_landmark_middle_finger_dip, mp_hand_landmark_middle_finger_tip},
    {mp_hand_landmark_middle_finger_mcp, mp_hand_landmark_ring_finger_mcp},
    {mp_hand_landmark_ring_finger_mcp, mp_hand_landmark_ring_finger_pip},
    {mp_hand_landmark_ring_finger_pip, mp_hand_landmark_ring_finger_dip},
    {mp_hand_landmark_ring_finger_dip, mp_hand_landmark_ring_finger_tip},
    {mp_hand_landmark_ring_finger_mcp, mp_hand_landmark_pinky_mcp},
    {mp_hand_landmark_wrist, mp_hand_landmark_pinky_mcp},
    {mp_hand_landmark_pinky_mcp, mp_hand_landmark_pinky_pip},
    {mp_hand_landmark_pinky_pip, mp_hand_landmark_pinky_dip},
    {mp_hand_landmark_pinky_dip, mp_hand_landmark_pinky_tip},
};

void drawLandmarks(cv::Mat& frame, mp_multi_face_landmark_list* landmarks)
{
    if (landmarks == nullptr)
    {
        return;
    }

    for (int i = 0; i < landmarks->length; i++)
    {
        const mp_landmark_list& hand = landmarks->elements[i];

        for (const auto& connection : CONNECTIONS)
        {
            const mp_landmark& p1 = hand.elements[connection[0]];
            const mp_landmark& p2 = hand.elements[connection[1]];
            float x1 = static_cast<float>(frame.cols) * p1.x;
            float y1 = static_cast<float>(frame.rows) * p1.y;
            float x2 = static_cast<float>(frame.cols) * p2.x;
            float y2 = static_cast<float>(frame.rows) * p2.y;

            cv::line(frame, {static_cast<int>(x1), static_cast<int>(y1)}, {static_cast<int>(x2), static_cast<int>(y2)}, CV_RGB(0, 255, 0), 2);
        }

        for (int j = 0; j < hand.length; j++)
        {
            const mp_landmark& p = hand.elements[j];
            float x = static_cast<float>(frame.cols) * p.x;
            float y = static_cast<float>(frame.rows) * p.y;
            cv::circle(frame, cv::Point(static_cast<int>(x), static_cast<int>(y)), 4, CV_RGB(255, 0, 0), -1);
        }
    }
}

void drawRects(cv::Mat& frame, mp_rect_list* rects)
{
    if (rects == nullptr)
    {
        return;
    }

    for (int i = 0; i < rects->length; i++)
    {
        const mp_rect& rect = rects->elements[i];

        cv::Point2f center(static_cast<float>(frame.cols) * rect.x_center, static_cast<float>(frame.rows) * rect.y_center);
        cv::Point2f size(static_cast<float>(frame.cols) * rect.width, static_cast<float>(frame.rows) * rect.height);
        float rotation = static_cast<float>(rect.rotation) * (180.0f / static_cast<float>(CV_PI));

        cv::Point2f vertices[4];
        cv::RotatedRect(center, size, rotation).points(vertices);
        for (int j = 0; j < 4; j++)
        {
            cv::line(frame, vertices[j], vertices[(j + 1) % 4], CV_RGB(0, 0, 255), 2);
        }
    }
}

} // namespace

TEST(IntegrationTest, BasicHandLandMarkExample)
{
    cv::utils::logging::setLogLevel(cv::utils::logging::LOG_LEVEL_ERROR);

    const std::string mediapipe_data_dir = Tests::findTrainingDataDir();
    if (mediapipe_data_dir.empty())
    {
        GTEST_SKIP() << "training_data not found";
    }

    cv::VideoCapture capture(0);
    if (!capture.isOpened())
    {
        GTEST_SKIP() << "camera(0) not available";
    }

    EXPECT_TRUE(capture.set(cv::CAP_PROP_FRAME_WIDTH, 640));
    EXPECT_TRUE(capture.set(cv::CAP_PROP_FRAME_HEIGHT, 480));
    EXPECT_TRUE(capture.set(cv::CAP_PROP_FPS, 60));

    mp_set_resource_dir(mediapipe_data_dir.c_str());

    const std::string graph_path = mediapipe_data_dir + "/mediapipe/modules/hand_landmark/hand_landmark_tracking_cpu.binarypb";

    mp_instance_builder* builder = mp_create_instance_builder(graph_path.c_str(), "image");
    ASSERT_NE(builder, nullptr);

    mp_add_option_float(builder, "palmdetectioncpu__TensorsToDetectionsCalculator", "min_score_thresh", 0.6f);
    mp_add_option_double(builder, "handlandmarkcpu__ThresholdingCalculator", "threshold", 0.2);
    mp_add_side_packet(builder, "num_hands", mp_create_packet_int(2));
    mp_add_side_packet(builder, "model_complexity", mp_create_packet_int(1));
    mp_add_side_packet(builder, "use_prev_landmarks", mp_create_packet_bool(true));

    mp_instance* instance = mp_create_instance(builder);
    ASSERT_NE(instance, nullptr);

    mp_poller* landmarks_poller = mp_create_poller(instance, "multi_hand_landmarks");
    ASSERT_NE(landmarks_poller, nullptr);

    mp_poller* rects_poller = mp_create_poller(instance, "hand_rects");
    ASSERT_NE(rects_poller, nullptr);

    ASSERT_TRUE(mp_start(instance));

    auto cleanup = [&]() {
        cv::destroyAllWindows();
        if (rects_poller != nullptr)
        {
            mp_destroy_poller(rects_poller);
            rects_poller = nullptr;
        }
        if (landmarks_poller != nullptr)
        {
            mp_destroy_poller(landmarks_poller);
            landmarks_poller = nullptr;
        }
        if (instance != nullptr)
        {
            mp_destroy_instance(instance);
            instance = nullptr;
        }
    };

    using namespace std::chrono;
    const auto start = steady_clock::now();
    const auto duration = 10s;

    cv::Mat raw_frame;
    while (true)
    {
        if (!capture.read(raw_frame))
        {
            break;
        }

        cv::Mat frame;
        cv::cvtColor(raw_frame, frame, cv::COLOR_BGR2RGB);

        mp_image image;
        image.data = frame.data;
        image.width = frame.cols;
        image.height = frame.rows;
        image.format = mp_image_format_srgb;

        if (!mp_process(instance, mp_create_packet_image(image)))
        {
            GTEST_LOG_(WARNING) << "mp_process failed";
            break;
        }

        if (!mp_wait_until_idle(instance))
        {
            GTEST_LOG_(WARNING) << "mp_wait_until_idle failed";
            break;
        }

        if (mp_get_queue_size(landmarks_poller) > 0)
        {
            mp_packet* packet = mp_poll_packet(landmarks_poller);
            if (packet != nullptr)
            {
                mp_multi_face_landmark_list* landmarks = mp_get_norm_multi_face_landmarks(packet);
                drawLandmarks(frame, landmarks);
                if (landmarks != nullptr)
                {
                    mp_destroy_multi_face_landmarks(landmarks);
                }
                mp_destroy_packet(packet);
            }
        }

        if (mp_get_queue_size(rects_poller) > 0)
        {
            mp_packet* packet = mp_poll_packet(rects_poller);
            if (packet != nullptr)
            {
                mp_rect_list* rects = mp_get_norm_rects(packet);
                drawRects(frame, rects);
                if (rects != nullptr)
                {
                    mp_destroy_rects(rects);
                }
                mp_destroy_packet(packet);
            }
        }

        cv::Mat preview;
        cv::resize(frame, preview, cv::Size(1600, 900));
        cv::imshow("MediaPipe", preview);

        if (cv::waitKey(1) == 27)
        {
            break;
        }

        if (steady_clock::now() - start >= duration)
        {
            break;
        }
    }

    cleanup();
}
