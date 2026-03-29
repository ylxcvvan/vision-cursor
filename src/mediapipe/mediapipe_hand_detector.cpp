#include "mediapipe/mediapipe_hand_detector.h"

#include "log/logging.h"

namespace VisionCursor
{
namespace MediaPipe
{

MediaPipeHandDetector::MediaPipeHandDetector() = default;

MediaPipeHandDetector::~MediaPipeHandDetector()
{
    if (is_open_)
    {
        cleanup();
        is_open_ = false;
    }
}

bool MediaPipeHandDetector::open(const std::string& training_data_dir,
                                 float min_score_thresh,
                                 double threshold,
                                 int model_complexity,
                                 bool use_prev_landmarks)
{
    if (is_open_)
    {
        LOG(ERROR) << "Cannot open MediapipeHandDetector: already opened.";
        return false;
    }

    mp_set_resource_dir(training_data_dir.c_str());

    const std::string graph_path =
        training_data_dir + "/mediapipe/modules/hand_landmark/hand_landmark_tracking_cpu.binarypb";
    builder_ = mp_create_instance_builder(graph_path.c_str(), "image");

    mp_add_option_float(builder_,
                        "palmdetectioncpu__TensorsToDetectionsCalculator",
                        "min_score_thresh",
                        min_score_thresh);
    mp_add_option_double(builder_, "handlandmarkcpu__ThresholdingCalculator", "threshold", threshold);
    mp_add_side_packet(builder_, "num_hands", mp_create_packet_int(1));
    mp_add_side_packet(builder_, "model_complexity", mp_create_packet_int(model_complexity));
    mp_add_side_packet(builder_, "use_prev_landmarks", mp_create_packet_bool(use_prev_landmarks));

    instance_ = mp_create_instance(builder_);
    if (!instance_)
    {
        cleanup();
        return logLastMediapipeError("mp_create_instance failed");
    }

    norm_landmarks_poller_ = mp_create_poller(instance_, "multi_hand_landmarks");
    if (!norm_landmarks_poller_)
    {
        cleanup();
        return logLastMediapipeError("mp_create_poller(multi_hand_landmarks) failed");
    }

    world_landmarks_poller_ = mp_create_poller(instance_, "multi_hand_world_landmarks");

    if (!mp_start(instance_))
    {
        cleanup();
        return logLastMediapipeError("mp_start failed");
    }

    is_open_ = true;
    LOG(INFO) << "Mediapipe hand detector opened.";
    return true;
}

bool MediaPipeHandDetector::close()
{
    if (!is_open_)
    {
        LOG(ERROR) << "Cannot close MediapipeHandDetector: detector is not open.";
        return false;
    }

    cleanup();
    is_open_ = false;
    LOG(INFO) << "Mediapipe hand detector closed.";
    return true;
}

bool MediaPipeHandDetector::process(FramePtr& frame, HandLandmarkArray& landmarks_out)
{
    if (!is_open_)
    {
        LOG(ERROR) << "Cannot process frame: detector is not open.";
        return false;
    }

    if (!frame)
    {
        LOG(ERROR) << "Cannot process frame: input frame is null.";
        return false;
    }

    landmarks_out.reset();

    mp_image image;
    image.data = frame->getData();
    image.width = frame->getWidth();
    image.height = frame->getHeight();
    image.format = mp_image_format_srgb;

    if (!mp_process(instance_, mp_create_packet_image(image)))
    {
        return logLastMediapipeError("mp_process failed");
    }

    if (!mp_wait_until_idle(instance_))
    {
        return logLastMediapipeError("mp_wait_until_idle failed");
    }

    if (norm_landmarks_poller_ && mp_get_queue_size(norm_landmarks_poller_) > 0)
    {
        mp_packet* packet = mp_poll_packet(norm_landmarks_poller_);
        if (!packet)
        {
            LOG(ERROR) << "Failed to poll normalized landmarks packet.";
            return false;
        }

        mp_multi_face_landmark_list* norm = mp_get_norm_multi_face_landmarks(packet);
        if (norm)
        {
            fillNormalized(norm, landmarks_out);
            mp_destroy_multi_face_landmarks(norm);
        }

        mp_destroy_packet(packet);
    }

    if (world_landmarks_poller_ && mp_get_queue_size(world_landmarks_poller_) > 0)
    {
        mp_packet* packet = mp_poll_packet(world_landmarks_poller_);
        if (!packet)
        {
            LOG(ERROR) << "Failed to poll world landmarks packet.";
            return false;
        }

        mp_multi_face_landmark_list* world = mp_get_multi_face_landmarks(packet);
        if (world)
        {
            fillWorld(world, landmarks_out);
            mp_destroy_multi_face_landmarks(world);
        }

        mp_destroy_packet(packet);
    }

    return true;
}

bool MediaPipeHandDetector::logLastMediapipeError(const char* context) const
{
    const char* error = mp_get_last_error();
    LOG(ERROR) << context << ": " << (error ? error : "Unknown Mediapipe error.");
    if (error)
    {
        mp_free_error(error);
    }
    return false;
}

void MediaPipeHandDetector::cleanup()
{
    if (world_landmarks_poller_)
    {
        mp_destroy_poller(world_landmarks_poller_);
        world_landmarks_poller_ = nullptr;
    }

    if (norm_landmarks_poller_)
    {
        mp_destroy_poller(norm_landmarks_poller_);
        norm_landmarks_poller_ = nullptr;
    }

    if (instance_)
    {
        mp_destroy_instance(instance_);
        instance_ = nullptr;
    }

    builder_ = nullptr;
}

void MediaPipeHandDetector::fillNormalized(const mp_multi_face_landmark_list* src, HandLandmarkArray& out)
{
    if (!src || src->length <= 0 || !src->elements || src->elements[0].length <= 0)
    {
        return;
    }

    const mp_landmark_list& first_hand = src->elements[0];
    const int count = first_hand.length < HAND_LANDMARK_COUNT ? first_hand.length : HAND_LANDMARK_COUNT;

    for (int i = 0; i < count; ++i)
    {
        const mp_landmark& in = first_hand.elements[i];
        HandLandmark& out_lm = out[static_cast<std::size_t>(i)];

        out_lm.normalized.x = in.x;
        out_lm.normalized.y = in.y;
        out_lm.normalized.z = in.z;
        out_lm.has_normalized = true;
    }
}

void MediaPipeHandDetector::fillWorld(const mp_multi_face_landmark_list* src, HandLandmarkArray& out)
{
    if (!src || src->length <= 0 || !src->elements || src->elements[0].length <= 0)
    {
        return;
    }

    const mp_landmark_list& first_hand = src->elements[0];
    const int count = first_hand.length < HAND_LANDMARK_COUNT ? first_hand.length : HAND_LANDMARK_COUNT;

    for (int i = 0; i < count; ++i)
    {
        const mp_landmark& in = first_hand.elements[i];
        HandLandmark& out_lm = out[static_cast<std::size_t>(i)];

        out_lm.world.x = in.x;
        out_lm.world.y = in.y;
        out_lm.world.z = in.z;
        out_lm.has_world = true;
    }
}

} // namespace MediaPipe
} // namespace VisionCursor
