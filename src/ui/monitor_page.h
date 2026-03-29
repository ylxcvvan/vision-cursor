#pragma once

#include <QTimer>
#include <QWidget>

#include <atomic>
#include <mutex>
#include <thread>

#include "core/vision_cursor_orchestrator.h"
#include "ui/video_opengl_widget.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
class MonitorPage;
}
QT_END_NAMESPACE

namespace VisionCursor
{

class MonitorPage : public QWidget
{
    Q_OBJECT

public:
    explicit MonitorPage(QWidget* parent = nullptr);
    ~MonitorPage() override;

    MonitorPage(const MonitorPage&) = delete;
    MonitorPage& operator=(const MonitorPage&) = delete;

public:
    enum class MonitorMode
    {
        Close = 0,
        Preview,
        Debug,
        Full
    };

public:
    bool init();

    bool startMonitoring();
    bool stopMonitoring();
    bool closeMonitoring();
    bool refreshRuntimeView();

public:
    void setCurrentModeText(const QString& mode_text);

    void setRunningStateText(const QString& text);
    void setPointerMovingStateText(const QString& text);
    void setCameraOpenedStateText(const QString& text);
    void setHandStateText(const QString& text);
    void setLastValidActionText(const QString& text);
    void setCurrentFrameActionText(const QString& text);
    void setVideoFpsText(const QString& text);
    void setSelectedCameraText(const QString& text);
    void setVideoResolutionText(const QString& text);
    void setSchemeText(const QString& text);
    void setCurrentTimeText(const QString& text);

    void appendLog(const QString& text);
    void clearLog();

signals:
    void videoFrameReady();

public slots:
    void onSettingApply();

private:
    void initUiState();
    void initConnections();
    void initTimer();

    void updateModeButtonState();
    void applyCurrentModeToCore();
    void setCurrentMode(MonitorMode mode);

    void applyRealtimeOutputToUi(const Core::OrchestratorOutput& output);
    void applySlowOutputToUi(const Core::OrchestratorOutput& output);

    QString actionToText(const Action& action, bool has_hand) const;

    void appendInfoLog(const QString& text);
    void appendErrorLog(const QString& text);
    void drainGlobalLogs();

    void refreshStatusView();
    bool refreshRuntimeStatusOnly();

    void startVideoThread();
    void stopVideoThread();
    void videoRefreshLoop();

    void drawDebugOverlay(FramePtr& frame, const HandLandmarkArray& arr);

private slots:
    void onPreviewModeClicked();
    void onDebugModeClicked();
    void onFullModeClicked();
    void onCloseModeClicked();

    void onStatusRefreshTimerTimeout();
    void onVideoFrameReady();

private:
    Ui::MonitorPage* ui = nullptr;
    VideoOpenGLWidget* video_widget_ = nullptr;

    QTimer* status_refresh_timer_ = nullptr;

    std::atomic<MonitorMode> current_mode_{MonitorMode::Close};
    QString last_valid_action_text_ = QStringLiteral("None");

    std::thread video_refresh_thread_;
    std::atomic<bool> video_thread_running_{false};

    std::atomic<int> current_fps_{0};

    std::mutex pending_frame_mutex_;
    FramePtr pending_frame_;
    HandLandmarkArray pending_landmarks_;
    Action pending_action_{};
    bool pending_has_hand_ = false;
};

} // namespace VisionCursor
