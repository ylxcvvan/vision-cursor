#include "monitor_page.h"
#include "ui_monitor_page.h"

#include <QDateTime>
#include <QTextDocument>
#include <QVBoxLayout>

#include <chrono>
#include <thread>

#include "single_instance.hpp"
#include "log/logging.h"
#include "utils/visualizer.h"

namespace VisionCursor
{
namespace
{

QString nowText()
{
    return QDateTime::currentDateTime().toString("HH:mm:ss");
}

} // namespace

MonitorPage::MonitorPage(QWidget* parent) : QWidget(parent), ui(new Ui::MonitorPage)
{
    ui->setupUi(this);

    video_widget_ = new VideoOpenGLWidget(this);
    video_widget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout* layout = qobject_cast<QVBoxLayout*>(ui->groupBoxVideo->layout());
    if (layout == nullptr)
    {
        layout = new QVBoxLayout(ui->groupBoxVideo);
        layout->setContentsMargins(8, 8, 8, 8);
        layout->setSpacing(0);
    }

    layout->addWidget(video_widget_);
}

MonitorPage::~MonitorPage()
{
    stopVideoThread();

    if (status_refresh_timer_ != nullptr)
    {
        status_refresh_timer_->stop();
        delete status_refresh_timer_;
        status_refresh_timer_ = nullptr;
    }

    delete video_widget_;
    delete ui;
}

bool MonitorPage::init()
{
    initUiState();
    initConnections();
    initTimer();
    return true;
}

bool MonitorPage::startMonitoring()
{
    auto& mgr = CoreManager::instance();
    auto& core = mgr.core();

    if (!core.isRunning())
    {
        if (!mgr.applyConfig())
        {
            appendErrorLog("Failed to apply config before start.");
            return false;
        }

        if (!core.isInited() && !mgr.initCore())
        {
            appendErrorLog("Failed to init core.");
            return false;
        }

        if (!core.isRunning() && !mgr.startCore())
        {
            appendErrorLog("Failed to start core.");
            return false;
        }

        current_fps_.store(0);
        setVideoFpsText(QStringLiteral("0 FPS"));

        startVideoThread();

        if (status_refresh_timer_ != nullptr && !status_refresh_timer_->isActive())
        {
            status_refresh_timer_->start();
        }

        appendInfoLog("Monitoring started.");
    }

    applyCurrentModeToCore();

    setRunningStateText(QStringLiteral("Running"));
    setCameraOpenedStateText(QStringLiteral("Opened"));
    return true;
}

bool MonitorPage::stopMonitoring()
{
    auto& mgr = CoreManager::instance();
    auto& core = mgr.core();

    stopVideoThread();
    if (video_widget_ != nullptr)
    {
        video_widget_->clearFrame();
    }

    if (status_refresh_timer_ != nullptr && status_refresh_timer_->isActive())
    {
        status_refresh_timer_->stop();
    }

    if (core.isRunning() && !mgr.stopCore())
    {
        appendErrorLog("Failed to stop core.");
        return false;
    }

    current_fps_.store(0);

    setRunningStateText(QStringLiteral("Stopped"));
    setPointerMovingStateText(QStringLiteral("No"));
    setCameraOpenedStateText(QStringLiteral("Closed"));
    setHandStateText(QStringLiteral("Not detected"));
    setCurrentFrameActionText(QStringLiteral("None"));
    setVideoFpsText(QStringLiteral("0 FPS"));

    appendInfoLog("Monitoring stopped.");
    return true;
}

bool MonitorPage::closeMonitoring()
{
    const bool stop_ok = stopMonitoring();
    const bool clear_ok = CoreManager::instance().clearCore();

    current_fps_.store(0);
    last_valid_action_text_ = QStringLiteral("None");

    setRunningStateText(QStringLiteral("Stopped"));
    setCameraOpenedStateText(QStringLiteral("Closed"));
    setPointerMovingStateText(QStringLiteral("No"));
    setHandStateText(QStringLiteral("Not detected"));
    setCurrentFrameActionText(QStringLiteral("None"));
    setLastValidActionText(QStringLiteral("None"));
    setVideoFpsText(QStringLiteral("0 FPS"));
    setSelectedCameraText(QStringLiteral("Not selected"));
    setVideoResolutionText(QStringLiteral("0 x 0"));
    setCurrentTimeText(QStringLiteral("--:--:--"));

    if (!stop_ok)
    {
        appendErrorLog("closeMonitoring: stop failed.");
    }
    if (!clear_ok)
    {
        appendErrorLog("closeMonitoring: clear failed.");
    }

    if (stop_ok && clear_ok)
    {
        appendInfoLog("Monitoring closed.");
    }

    return stop_ok && clear_ok;
}

bool MonitorPage::refreshRuntimeView()
{
    return refreshRuntimeStatusOnly();
}

void MonitorPage::setCurrentModeText(const QString& mode_text)
{
    ui->labelPageHint->setText(QStringLiteral("Current mode: %1").arg(mode_text));
    updateModeButtonState();
}

void MonitorPage::setRunningStateText(const QString& text)
{
    ui->labelSystemRunningValue->setText(text);
}

void MonitorPage::setPointerMovingStateText(const QString& text)
{
    ui->labelPointerMovingValue->setText(text);
}

void MonitorPage::setCameraOpenedStateText(const QString& text)
{
    ui->labelCameraOpenedValue->setText(text);
}

void MonitorPage::setHandStateText(const QString& text)
{
    ui->labelHasHandValue->setText(text);
}

void MonitorPage::setLastValidActionText(const QString& text)
{
    ui->labelLastValidActionValue->setText(text);
}

void MonitorPage::setCurrentFrameActionText(const QString& text)
{
    ui->labelCurrentFrameActionValue->setText(text);
}

void MonitorPage::setVideoFpsText(const QString& text)
{
    ui->labelVideoFpsValue->setText(text);
}

void MonitorPage::setSelectedCameraText(const QString& text)
{
    if (text.trimmed().isEmpty())
    {
        ui->labelSelectedCameraValue->setText(QStringLiteral("Not selected"));
        return;
    }
    ui->labelSelectedCameraValue->setText(text);
}

void MonitorPage::setVideoResolutionText(const QString& text)
{
    ui->labelVideoResolutionValue->setText(text);
}

void MonitorPage::setSchemeText(const QString& text)
{
    if (text.trimmed().isEmpty())
    {
        ui->labelFsmSchemeValue->setText(QStringLiteral("Not set"));
        return;
    }
    ui->labelFsmSchemeValue->setText(text);
}

void MonitorPage::setCurrentTimeText(const QString& text)
{
    ui->labelCurrentTimeValue->setText(text);
}

void MonitorPage::appendLog(const QString& text)
{
    ui->plainTextLog->appendPlainText(text);
}

void MonitorPage::clearLog()
{
    ui->plainTextLog->clear();
}

void MonitorPage::initUiState()
{
    setCurrentMode(MonitorMode::Close);

    setRunningStateText(QStringLiteral("Stopped"));
    setPointerMovingStateText(QStringLiteral("No"));
    setCameraOpenedStateText(QStringLiteral("Closed"));
    setHandStateText(QStringLiteral("Not detected"));
    setLastValidActionText(QStringLiteral("None"));
    setCurrentFrameActionText(QStringLiteral("None"));
    setVideoFpsText(QStringLiteral("0 FPS"));
    setSelectedCameraText(QStringLiteral("Not selected"));
    setVideoResolutionText(QStringLiteral("0 x 0"));
    setSchemeText(QStringLiteral("easy"));
    setCurrentTimeText(QStringLiteral("--:--:--"));

    ui->plainTextLog->document()->setMaximumBlockCount(2000);
    clearLog();
    appendInfoLog("MonitorPage initialized.");
}

void MonitorPage::initConnections()
{
    connect(ui->btnPreviewMode, &QPushButton::clicked, this, &MonitorPage::onPreviewModeClicked);
    connect(ui->btnDebugMode, &QPushButton::clicked, this, &MonitorPage::onDebugModeClicked);
    connect(ui->btnFullMode, &QPushButton::clicked, this, &MonitorPage::onFullModeClicked);
    connect(ui->btnCloseMode, &QPushButton::clicked, this, &MonitorPage::onCloseModeClicked);

    connect(this, &MonitorPage::videoFrameReady, this, &MonitorPage::onVideoFrameReady, Qt::QueuedConnection);
}

void MonitorPage::initTimer()
{
    status_refresh_timer_ = new QTimer(this);
    status_refresh_timer_->setInterval(1000);
    connect(status_refresh_timer_, &QTimer::timeout, this, &MonitorPage::onStatusRefreshTimerTimeout);
    status_refresh_timer_->start();
}

void MonitorPage::updateModeButtonState()
{
    const QString active_style = QStringLiteral("background-color: #dbeafe;"
                                                "border: 1px solid #93c5fd;"
                                                "color: #1e3a8a;"
                                                "font-weight: 700;");

    const QString normal_style = QString();

    ui->btnPreviewMode->setStyleSheet(normal_style);
    ui->btnDebugMode->setStyleSheet(normal_style);
    ui->btnFullMode->setStyleSheet(normal_style);
    ui->btnCloseMode->setStyleSheet(normal_style);

    switch (current_mode_.load())
    {
        case MonitorMode::Preview: ui->btnPreviewMode->setStyleSheet(active_style); break;
        case MonitorMode::Debug: ui->btnDebugMode->setStyleSheet(active_style); break;
        case MonitorMode::Full: ui->btnFullMode->setStyleSheet(active_style); break;
        case MonitorMode::Close: ui->btnCloseMode->setStyleSheet(active_style); break;
    }
}

void MonitorPage::setCurrentMode(MonitorMode mode)
{
    current_mode_.store(mode);

    switch (current_mode_.load())
    {
        case MonitorMode::Preview: setCurrentModeText(QStringLiteral("Preview")); break;
        case MonitorMode::Debug: setCurrentModeText(QStringLiteral("Debug")); break;
        case MonitorMode::Full: setCurrentModeText(QStringLiteral("Full")); break;
        case MonitorMode::Close: setCurrentModeText(QStringLiteral("Close")); break;
    }
}

void MonitorPage::applyCurrentModeToCore()
{
    auto& core = CoreManager::instance().core();

    switch (current_mode_.load())
    {
        case MonitorMode::Preview:
            core.setMediapipeEnabled(false);
            core.setMouseControlEnabled(false);
            break;
        case MonitorMode::Debug:
            core.setMediapipeEnabled(true);
            core.setMouseControlEnabled(false);
            break;
        case MonitorMode::Full:
            core.setMediapipeEnabled(true);
            core.setMouseControlEnabled(true);
            break;
        case MonitorMode::Close:
            core.setMediapipeEnabled(false);
            core.setMouseControlEnabled(false);
            break;
    }
}

void MonitorPage::applyRealtimeOutputToUi(const Core::OrchestratorOutput& output)
{
    const bool has_hand = output.has_hand;
    const Action& action = output.action;

    setHandStateText(has_hand ? QStringLiteral("Detected") : QStringLiteral("Not detected"));
    setPointerMovingStateText(action.isMoving ? QStringLiteral("Yes") : QStringLiteral("No"));

    QString extra_info;
    if (action.type == ActionType::ScrollVertical)
    {
        extra_info = QStringLiteral(" (dy=%1)").arg(action.dy, 0, 'f', 3);
    }
    else if (action.type == ActionType::ScrollHorizontal)
    {
        extra_info = QStringLiteral(" (dx=%1)").arg(action.dx, 0, 'f', 3);
    }

    const QString current_action_text = actionToText(action, has_hand) + extra_info;
    setCurrentFrameActionText(current_action_text);

    if (has_hand && current_action_text != QStringLiteral("None"))
    {
        last_valid_action_text_ = current_action_text;
    }
}

void MonitorPage::applySlowOutputToUi(const Core::OrchestratorOutput& output)
{
    Q_UNUSED(output);
    setLastValidActionText(last_valid_action_text_);
}

QString MonitorPage::actionToText(const Action& action, bool has_hand) const
{
    if (!has_hand)
    {
        return QStringLiteral("None");
    }

    switch (action.type)
    {
        case ActionType::None: return QStringLiteral("None");
        case ActionType::LeftDown: return QStringLiteral("LeftDown");
        case ActionType::LeftUp: return QStringLiteral("LeftUp");
        case ActionType::LeftClick: return QStringLiteral("LeftClick");
        case ActionType::RightDown: return QStringLiteral("RightDown");
        case ActionType::RightUp: return QStringLiteral("RightUp");
        case ActionType::RightClick: return QStringLiteral("RightClick");
        case ActionType::ScrollVertical: return QStringLiteral("ScrollVertical");
        case ActionType::ScrollHorizontal: return QStringLiteral("ScrollHorizontal");
        default: return QStringLiteral("Unknown");
    }
}

void MonitorPage::appendInfoLog(const QString& text)
{
    appendLog(QStringLiteral("[%1] [INFO] %2").arg(nowText(), text));
}

void MonitorPage::appendErrorLog(const QString& text)
{
    appendLog(QStringLiteral("[%1] [ERROR] %2").arg(nowText(), text));
}

void MonitorPage::drainGlobalLogs()
{
    const auto lines = Log::ConsumeLines(256);
    for (const auto& line : lines)
    {
        appendLog(QString::fromStdString(line));
    }
}

bool MonitorPage::refreshRuntimeStatusOnly()
{
    refreshStatusView();
    return true;
}

void MonitorPage::refreshStatusView()
{
    auto& mgr = CoreManager::instance();
    auto& core = mgr.core();

    setCurrentTimeText(nowText());
    setRunningStateText(core.isRunning() ? QStringLiteral("Running") : QStringLiteral("Stopped"));

    const Config::OrchestratorConfig config = mgr.config();
    setSelectedCameraText(QString::fromStdString(config.camera.camera_device_name));
    setVideoResolutionText(QStringLiteral("%1 x %2")
                               .arg(config.camera.camera_resolution.width)
                               .arg(config.camera.camera_resolution.height));
    setSchemeText(QString::fromStdString(config.interaction.scheme_name));

    const int fps = current_fps_.exchange(0);
    setVideoFpsText(QStringLiteral("%1 FPS").arg(fps));

    const Core::OrchestratorOutput output = core.lastOutput();
    applySlowOutputToUi(output);

    setCameraOpenedStateText(core.isInited() ? QStringLiteral("Opened") : QStringLiteral("Closed"));
}

void MonitorPage::startVideoThread()
{
    if (video_thread_running_.load())
    {
        return;
    }

    video_thread_running_.store(true);
    video_refresh_thread_ = std::thread(&MonitorPage::videoRefreshLoop, this);
}

void MonitorPage::stopVideoThread()
{
    if (!video_thread_running_.load())
    {
        if (video_refresh_thread_.joinable())
        {
            video_refresh_thread_.join();
        }
        return;
    }

    video_thread_running_.store(false);

    if (video_refresh_thread_.joinable())
    {
        video_refresh_thread_.join();
    }
}

void MonitorPage::drawDebugOverlay(FramePtr& frame, const HandLandmarkArray& arr)
{
    if (!frame)
    {
        return;
    }

    const Config::OrchestratorConfig config = CoreManager::instance().config();
    Utils::drawMapperRegion(frame, config.mapper.region);

    if (!arr.isValid())
    {
        return;
    }

    Utils::draw_landmarks(frame, arr);
    Utils::drawControlJointHighlight(frame, arr, config.interaction.control_joint);
}

void MonitorPage::videoRefreshLoop()
{
    auto& core = CoreManager::instance().core();

    while (video_thread_running_.load())
    {
        if (!core.isRunning())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        const Core::OrchestratorOutput output = core.getOutput(300);
        if (!video_thread_running_.load())
        {
            break;
        }

        FramePtr frame = output.frame;
        if (!frame || frame->getData() == nullptr || frame->getWidth() <= 0 || frame->getHeight() <= 0)
        {
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(pending_frame_mutex_);
            pending_frame_ = frame;
            pending_landmarks_ = output.processed_landmarks;
            pending_action_ = output.action;
            pending_has_hand_ = output.has_hand;
        }

        emit videoFrameReady();
    }
}

void MonitorPage::onPreviewModeClicked()
{
    if (current_mode_.load() == MonitorMode::Preview)
    {
        return;
    }

    setCurrentMode(MonitorMode::Preview);
    appendInfoLog("Switched to Preview mode.");

    auto& core = CoreManager::instance().core();
    const bool ok = core.isRunning() ? true : startMonitoring();
    if (!ok)
    {
        return;
    }

    applyCurrentModeToCore();
    refreshStatusView();
}

void MonitorPage::onDebugModeClicked()
{
    if (current_mode_.load() == MonitorMode::Debug)
    {
        return;
    }

    setCurrentMode(MonitorMode::Debug);
    appendInfoLog("Switched to Debug mode.");

    auto& core = CoreManager::instance().core();
    const bool ok = core.isRunning() ? true : startMonitoring();
    if (!ok)
    {
        return;
    }

    applyCurrentModeToCore();
    refreshStatusView();
}

void MonitorPage::onFullModeClicked()
{
    if (current_mode_.load() == MonitorMode::Full)
    {
        return;
    }

    setCurrentMode(MonitorMode::Full);
    appendInfoLog("Switched to Full mode.");

    auto& core = CoreManager::instance().core();
    const bool ok = core.isRunning() ? true : startMonitoring();
    if (!ok)
    {
        return;
    }

    applyCurrentModeToCore();
    refreshStatusView();
}

void MonitorPage::onCloseModeClicked()
{
    if (current_mode_.load() == MonitorMode::Close)
    {
        return;
    }

    setCurrentMode(MonitorMode::Close);
    appendInfoLog("Switched to Close mode.");
    closeMonitoring();
}

void MonitorPage::onStatusRefreshTimerTimeout()
{
    drainGlobalLogs();
    if (CoreManager::instance().core().isRunning())
    {
        refreshStatusView();
    }
}

void MonitorPage::onVideoFrameReady()
{
    if (current_mode_.load() == MonitorMode::Close)
    {
        return;
    }

    FramePtr frame;
    HandLandmarkArray landmarks;
    Action action;
    bool has_hand = false;

    {
        std::lock_guard<std::mutex> lock(pending_frame_mutex_);
        frame = pending_frame_;
        landmarks = pending_landmarks_;
        action = pending_action_;
        has_hand = pending_has_hand_;
    }

    if (!frame || video_widget_ == nullptr)
    {
        return;
    }

    FramePtr render_frame = frame;

    if (current_mode_.load() == MonitorMode::Debug || current_mode_.load() == MonitorMode::Full)
    {
        render_frame = frame->clone();
        drawDebugOverlay(render_frame, landmarks);
    }

    video_widget_->setFrame(render_frame);

    Core::OrchestratorOutput output{};
    output.action = action;
    output.has_hand = has_hand;
    applyRealtimeOutputToUi(output);

    current_fps_.fetch_add(1);
}

void MonitorPage::onSettingApply()
{
    const MonitorMode mode = current_mode_.load();

    onCloseModeClicked();
    if (mode == MonitorMode::Preview)
    {
        onPreviewModeClicked();
    }
    else if (mode == MonitorMode::Debug)
    {
        onDebugModeClicked();
    }
    else if (mode == MonitorMode::Full)
    {
        onFullModeClicked();
    }
}

} // namespace VisionCursor
