#pragma once

#include <QStringList>
#include <QShowEvent>
#include <QWidget>

#include <vector>

#include "config/core_config.h"
#include "config/structs.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
class SettingPage;
}
QT_END_NAMESPACE

namespace VisionCursor
{

class SettingPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingPage(QWidget* parent = nullptr);
    ~SettingPage() override;

    SettingPage(const SettingPage&) = delete;
    SettingPage& operator=(const SettingPage&) = delete;

public:
    bool init();

    bool loadConfig(const QString& file_path = QString());
    bool saveConfig(const QString& file_path = QString());
    bool applyConfig();
    bool resetConfig();
    bool refreshCameraDevices();

public:
    Config::OrchestratorConfig collectConfigFromUi() const;
    void applyConfigToUi(const Config::OrchestratorConfig& config);

    void setCameraDeviceList(const QStringList& device_names);
    QString selectedCameraDeviceName() const;

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void showEvent(QShowEvent* event) override;

private:
    bool collectConfigFromUiChecked(Config::OrchestratorConfig& config) const;

    void initUiState();
    void initConnections();
    void initHintState();
    void restoreGestureSectionChineseText();

    bool collectCameraConfig(Config::OrchestratorConfig::CameraConfig& config) const;
    void collectMediaPipeConfig(Config::OrchestratorConfig::MediaPipeConfig& config) const;
    bool collectInteractionConfig(Config::OrchestratorConfig::InteractionConfig& config) const;
    void collectMapperConfig(Config::OrchestratorConfig::MapperConfig& config) const;

    void applyCameraConfigToUi(const Config::OrchestratorConfig::CameraConfig& config);
    void applyMediaPipeConfigToUi(const Config::OrchestratorConfig::MediaPipeConfig& config);
    void applyInteractionConfigToUi(const Config::OrchestratorConfig::InteractionConfig& config);
    void applyMapperConfigToUi(const Config::OrchestratorConfig::MapperConfig& config);

    bool parseCameraResolutionText(const QString& text, CameraResolution& resolution) const;
    QString cameraResolutionToText(const CameraResolution& resolution) const;
    int findResolutionIndex(const CameraResolution& resolution) const;
    void refreshResolutionListForDevice(const QString& device_name,
                                        const QString& preferred_resolution_text = QString());
    void ensureResolutionListHasContent(const QString& preferred_resolution_text = QString());

    MoveMode currentMoveModeFromUi() const;
    void setMoveModeToUi(MoveMode mode);

    void syncFilterStackByName(const QString& filter_name);
    std::vector<float> collectFilterParamsFromUi(const QString& filter_name) const;
    void applyFilterParamsToUi(const QString& filter_name, const std::vector<float>& vec_filter_param);

    void setBottomHint(const QString& text);
    void showResultMessage(bool success, const QString& success_text, const QString& failure_text = QString());

private slots:
    void onRefreshCameraListClicked();
    void onLoadConfigClicked();
    void onSaveConfigClicked();
    void onApplyConfigClicked();
    void onResetConfigClicked();
    void onFilterNameChanged(int index);
    void onCurrentChooseDeviceChanged(QString device_name);
    void onSchemeComboBoxPressed();

signals:
    void reStartCamera();

private:
    Ui::SettingPage* ui = nullptr;
};

} // namespace VisionCursor
