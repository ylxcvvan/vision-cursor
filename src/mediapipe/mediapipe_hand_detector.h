#pragma once

#include <string>

#include <mediapipe.h>

#include "common/frame.h"
#include "common/structs.h"

namespace VisionCursor
{
namespace MediaPipe
{

class MediaPipeHandDetector
{
public:
    MediaPipeHandDetector();
    MediaPipeHandDetector(const MediaPipeHandDetector&) = delete;
    MediaPipeHandDetector& operator=(const MediaPipeHandDetector&) = delete;
    MediaPipeHandDetector(MediaPipeHandDetector&&) = delete;
    MediaPipeHandDetector& operator=(MediaPipeHandDetector&&) = delete;
    ~MediaPipeHandDetector();

    bool open(const std::string& training_data_dir,
              float min_score_thresh = 0.6f,
              double threshold = 0.2f,
              int model_complexity = 1,
              bool use_prev_landmarks = true);

    bool close();
    bool process(FramePtr& frame, HandLandmarkArray& landmarks_out);

private:
    bool logLastMediapipeError(const char* context) const;
    void cleanup();
    static void fillNormalized(const mp_multi_face_landmark_list* src, HandLandmarkArray& out);
    static void fillWorld(const mp_multi_face_landmark_list* src, HandLandmarkArray& out);

private:
    bool is_open_ = false;
    mp_instance_builder* builder_ = nullptr;
    mp_instance* instance_ = nullptr;
    mp_poller* norm_landmarks_poller_ = nullptr;
    mp_poller* world_landmarks_poller_ = nullptr;
};

} // namespace MediaPipe
} // namespace VisionCursor