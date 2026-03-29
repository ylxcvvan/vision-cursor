#include "setting_page.h"
#include "ui_setting_page.h"

#include <QComboBox>
#include <QDir>
#include <QEvent>
#include <QFileInfoList>
#include <QList>
#include <QSize>
#include <QSignalBlocker>
#include <QVariant>
#include <QShowEvent>

#include "common/constants.h"
#include "config/fsm_config.h"
#include "log/logging.h"
#include "single_instance.hpp"
#include "utils/to_string.h"

namespace VisionCursor
{
namespace
{
QString normalizeDeviceDisplayName(const QString& text)
{
    const QString trimmed = text.trimmed();
    int i = 0;
    while (i < trimmed.size() && trimmed[i].isDigit())
    {
        ++i;
    }
    if (i > 0 && i < trimmed.size() && trimmed[i] == QLatin1Char(':'))
    {
        return trimmed.mid(i + 1).trimmed();
    }
    return trimmed;
}
} // namespace

SettingPage::SettingPage(QWidget* parent) : QWidget(parent), ui(new Ui::SettingPage)
{
    ui->setupUi(this);
}

SettingPage::~SettingPage()
{
    delete ui;
}

bool SettingPage::init()
{
    initUiState();
    initConnections();
    initHintState();

    if (!loadConfig())
    {
        Config::OrchestratorConfig default_config = Config::CoreConfig::createDefault();
        applyConfigToUi(default_config);
        CoreManager::instance().setConfig(default_config);
        setBottomHint(
            QStringLiteral("\u9ED8\u8BA4\u914D\u7F6E\u6587\u4EF6\u8BFB\u53D6\u5931\u8D25\uFF0C\u5DF2\u4F7F\u7528\u5185\u7F6E\u9ED8\u8BA4\u914D\u7F6E\u3002"));
    }

    if (!refreshCameraDevices())
    {
        setBottomHint(
            QStringLiteral("\u9875\u9762\u521D\u59CB\u5316\u5B8C\u6210\uFF0C\u4F46\u6444\u50CF\u5934\u5217\u8868\u5237\u65B0\u5931\u8D25\u3002"));
    }

    if (!applyConfig())
    {
        setBottomHint(
            QStringLiteral("\u9875\u9762\u521D\u59CB\u5316\u5B8C\u6210\uFF0C\u4F46\u914D\u7F6E\u5E94\u7528\u5931\u8D25\u3002"));
        return false;
    }

    // 页面初始化时，手动确保一次分辨率列表刷新且有内容。
    refreshResolutionListForDevice(ui->comboBoxCameraDevice->currentText().trimmed(),
                                   ui->comboBoxCameraResolution->currentText().trimmed());
    ensureResolutionListHasContent(ui->comboBoxCameraResolution->currentText().trimmed());

    return true;
}

bool SettingPage::loadConfig(const QString& file_path)
{
    const QString actual_path = file_path.isEmpty() ? Config::CoreConfig::defaultPath() : file_path;

    Config::OrchestratorConfig config;
    if (!Config::CoreConfig::load(config, actual_path))
    {
        showResultMessage(false, QString(), QStringLiteral("配置读取失败。"));
        return false;
    }

    applyConfigToUi(config);
    CoreManager::instance().setConfig(config);

    showResultMessage(true, QStringLiteral("配置读取成功。"));
    return true;
}

bool SettingPage::saveConfig(const QString& file_path)
{
    const QString actual_path = file_path.isEmpty() ? Config::CoreConfig::defaultPath() : file_path;

    Config::OrchestratorConfig config;
    if (!collectConfigFromUiChecked(config))
    {
        showResultMessage(false, QString(), QStringLiteral("保存失败：输入参数无效。"));
        return false;
    }

    if (!Config::CoreConfig::save(config, actual_path))
    {
        showResultMessage(false, QString(), QStringLiteral("配置保存失败。"));
        return false;
    }

    CoreManager::instance().setConfig(config);
    showResultMessage(true, QStringLiteral("配置保存成功。"));
    return true;
}

bool SettingPage::applyConfig()
{
    Config::OrchestratorConfig config;
    if (!collectConfigFromUiChecked(config))
    {
        showResultMessage(false, QString(), QStringLiteral("应用失败：输入参数无效。"));
        return false;
    }

    CoreManager::instance().setConfig(config);
    showResultMessage(true, QStringLiteral("配置已应用。"));
    return true;
}

bool SettingPage::resetConfig()
{
    const Config::OrchestratorConfig default_config = Config::CoreConfig::createDefault();
    applyConfigToUi(default_config);
    CoreManager::instance().setConfig(default_config);

    setBottomHint(QStringLiteral("已重置为默认配置，点击“应用配置”后生效。"));
    return true;
}

bool SettingPage::refreshCameraDevices()
{
    auto& core = CoreManager::instance().core();
    if (core.isRunning())
    {
        showResultMessage(
            false,
            QString(),
            QStringLiteral("\u8BF7\u5728\u5173\u95ED\u6444\u50CF\u5934\u540E\u518D\u5237\u65B0\u8BBE\u5907\u5217\u8868\u3002"));
        return false;
    }

    if (!core.updateCameraDevices())
    {
        showResultMessage(false, QString(), QStringLiteral("\u6444\u50CF\u5934\u5217\u8868\u5237\u65B0\u5931\u8D25\u3002"));
        return false;
    }

    const std::vector<std::string> names = core.allCameraDeviceNames();
    QStringList device_names;
    device_names.reserve(static_cast<int>(names.size()));
    for (const std::string& name : names)
    {
        device_names.push_back(QString::fromStdString(name));
    }

    const QString previous_device_name = ui->comboBoxCameraDevice->currentText().trimmed();
    const QString previous_device_normalized = normalizeDeviceDisplayName(previous_device_name);
    setCameraDeviceList(device_names);

    if (ui->comboBoxCameraDevice->count() > 0)
    {
        // 刷新后优先保持当前摄像头；若设备已变化，回退到索引 0。
        const QString current_device_name = ui->comboBoxCameraDevice->currentText().trimmed();
        const QString current_device_normalized = normalizeDeviceDisplayName(current_device_name);
        if (!previous_device_name.isEmpty() && current_device_normalized != previous_device_normalized)
        {
            ui->comboBoxCameraDevice->setCurrentIndex(0);
        }
        onCurrentChooseDeviceChanged(ui->comboBoxCameraDevice->currentText().trimmed());
    }

    showResultMessage(true, QStringLiteral("\u6444\u50CF\u5934\u5217\u8868\u5237\u65B0\u6210\u529F\u3002"));
    return true;
}

Config::OrchestratorConfig SettingPage::collectConfigFromUi() const
{
    Config::OrchestratorConfig config;
    if (!collectConfigFromUiChecked(config))
    {
        LOG(ERROR) << "collectConfigFromUi failed. Fallback to defaults.";
    }
    return config;
}

bool SettingPage::collectConfigFromUiChecked(Config::OrchestratorConfig& config) const
{
    config = Config::CoreConfig::createDefault();

    if (!collectCameraConfig(config.camera))
    {
        return false;
    }

    collectMediaPipeConfig(config.mediapipe);

    if (!collectInteractionConfig(config.interaction))
    {
        return false;
    }

    collectMapperConfig(config.mapper);
    return true;
}

void SettingPage::applyConfigToUi(const Config::OrchestratorConfig& config)
{
    applyCameraConfigToUi(config.camera);
    applyMediaPipeConfigToUi(config.mediapipe);
    applyInteractionConfigToUi(config.interaction);
    applyMapperConfigToUi(config.mapper);
}

void SettingPage::setCameraDeviceList(const QStringList& device_names)
{
    const QSignalBlocker blocker(ui->comboBoxCameraDevice);

    const QString old_text = ui->comboBoxCameraDevice->currentText();
    const QString old_normalized_name = normalizeDeviceDisplayName(old_text);

    ui->comboBoxCameraDevice->clear();
    ui->comboBoxCameraDevice->addItems(device_names);

    int idx = ui->comboBoxCameraDevice->findText(old_text);
    if (idx < 0 && !old_normalized_name.isEmpty())
    {
        for (int i = 0; i < ui->comboBoxCameraDevice->count(); ++i)
        {
            if (normalizeDeviceDisplayName(ui->comboBoxCameraDevice->itemText(i)) == old_normalized_name)
            {
                idx = i;
                break;
            }
        }
    }
    if (idx < 0 && !device_names.isEmpty())
    {
        idx = 0;
    }

    if (idx >= 0)
    {
        ui->comboBoxCameraDevice->setCurrentIndex(idx);
    }
}

QString SettingPage::selectedCameraDeviceName() const
{
    return ui->lineEditCameraDeviceName->text().trimmed();
}

void SettingPage::initUiState()
{
    syncFilterStackByName(ui->comboBoxFilterName->currentText());
    ui->lineEditCameraDeviceName->clear();
    ui->comboBoxSchemeName->installEventFilter(this);
    ui->comboBoxCameraResolution->installEventFilter(this);
    restoreGestureSectionChineseText();
}

void SettingPage::initConnections()
{
    connect(ui->btnRefreshCameraList, &QPushButton::clicked, this, &SettingPage::onRefreshCameraListClicked);
    connect(ui->btnLoadConfig, &QPushButton::clicked, this, &SettingPage::onLoadConfigClicked);
    connect(ui->btnSaveConfig, &QPushButton::clicked, this, &SettingPage::onSaveConfigClicked);
    connect(ui->btnApplyConfig, &QPushButton::clicked, this, &SettingPage::onApplyConfigClicked);
    connect(ui->btnResetConfig, &QPushButton::clicked, this, &SettingPage::onResetConfigClicked);

    connect(ui->comboBoxFilterName,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            &SettingPage::onFilterNameChanged);

    connect(ui->comboBoxCameraDevice, &QComboBox::currentTextChanged, this, &SettingPage::onCurrentChooseDeviceChanged);
    connect(ui->comboBoxCameraDevice,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this,
            [this](int) { onCurrentChooseDeviceChanged(ui->comboBoxCameraDevice->currentText()); });
    connect(ui->comboBoxCameraDevice,
            QOverload<int>::of(&QComboBox::activated),
            this,
            [this](int) { onCurrentChooseDeviceChanged(ui->comboBoxCameraDevice->currentText()); });
}

void SettingPage::initHintState()
{
    setBottomHint(
        QStringLiteral("\u5C31\u7EEA\u3002\u8BF7\u5148\u8BFB\u53D6\u914D\u7F6E\u6216\u5237\u65B0\u6444\u50CF\u5934\u5217\u8868\u3002"));
}

bool SettingPage::collectCameraConfig(Config::OrchestratorConfig::CameraConfig& config) const
{
    config.camera_device_name = selectedCameraDeviceName().toStdString();
    config.camera_fps = ui->spinBoxCameraFps->value();
    config.is_mirror = ui->checkBoxCameraMirror->isChecked();

    CameraResolution resolution;
    if (!parseCameraResolutionText(ui->comboBoxCameraResolution->currentText(), resolution))
    {
        return false;
    }
    config.camera_resolution = resolution;
    return true;
}

void SettingPage::collectMediaPipeConfig(Config::OrchestratorConfig::MediaPipeConfig& config) const
{
    config.min_score_thresh = static_cast<float>(ui->doubleSpinBoxMinScoreThresh->value());
    config.threshold = ui->doubleSpinBoxThreshold->value();
    config.model_complexity = ui->spinBoxModelComplexity->value();
    config.use_prev_landmarks = ui->checkBoxUsePrevLandmarks->isChecked();
}

bool SettingPage::collectInteractionConfig(Config::OrchestratorConfig::InteractionConfig& config) const
{
    config.scheme_name = ui->comboBoxSchemeName->currentText().trimmed().toStdString();
    config.filter_name = ui->comboBoxFilterName->currentText().trimmed().toStdString();

    HandJoint joint = HandJoint::IndexTIP;
    if (!Utils::parseHandJoint(ui->comboBoxControlJoint->currentText().trimmed(), joint))
    {
        LOG(ERROR) << "Invalid control joint selected.";
        return false;
    }

    config.control_joint = joint;
    config.vec_filter_param = collectFilterParamsFromUi(ui->comboBoxFilterName->currentText());
    return true;
}

void SettingPage::collectMapperConfig(Config::OrchestratorConfig::MapperConfig& config) const
{
    config.x_sensitivity = static_cast<float>(ui->doubleSpinBoxXSensitivity->value());
    config.y_sensitivity = static_cast<float>(ui->doubleSpinBoxYSensitivity->value());
    config.scroll_sensitivity = static_cast<float>(ui->doubleSpinBoxScrollSensitivity->value());
    config.move_mode = currentMoveModeFromUi();

    config.region.left = ui->spinBoxRegionLeftPercent->value() / 100.0f;
    config.region.top = ui->spinBoxRegionTopPercent->value() / 100.0f;
    config.region.right = ui->spinBoxRegionRightPercent->value() / 100.0f;
    config.region.bottom = ui->spinBoxRegionBottomPercent->value() / 100.0f;
}

void SettingPage::applyCameraConfigToUi(const Config::OrchestratorConfig::CameraConfig& config)
{
    ui->lineEditCameraDeviceName->setText(QString::fromStdString(config.camera_device_name));
    ui->spinBoxCameraFps->setValue(config.camera_fps);
    ui->checkBoxCameraMirror->setChecked(config.is_mirror);

    const QString resolution_text = cameraResolutionToText(config.camera_resolution);
    int resolution_idx = ui->comboBoxCameraResolution->findText(resolution_text);
    if (resolution_idx < 0)
    {
        for (int i = 0; i < ui->comboBoxCameraResolution->count(); ++i)
        {
            CameraResolution candidate;
            if (parseCameraResolutionText(ui->comboBoxCameraResolution->itemText(i), candidate) &&
                candidate.width == config.camera_resolution.width &&
                candidate.height == config.camera_resolution.height)
            {
                resolution_idx = i;
                break;
            }
        }
    }
    if (resolution_idx >= 0)
    {
        ui->comboBoxCameraResolution->setCurrentIndex(resolution_idx);
    }

    const QString camera_name = QString::fromStdString(config.camera_device_name);
    if (!camera_name.isEmpty())
    {
        const int idx = ui->comboBoxCameraDevice->findText(camera_name);
        if (idx >= 0)
        {
            ui->comboBoxCameraDevice->setCurrentIndex(idx);
        }
    }
}

void SettingPage::applyMediaPipeConfigToUi(const Config::OrchestratorConfig::MediaPipeConfig& config)
{
    ui->doubleSpinBoxMinScoreThresh->setValue(config.min_score_thresh);
    ui->doubleSpinBoxThreshold->setValue(config.threshold);
    ui->spinBoxModelComplexity->setValue(config.model_complexity);
    ui->checkBoxUsePrevLandmarks->setChecked(config.use_prev_landmarks);
}

void SettingPage::applyInteractionConfigToUi(const Config::OrchestratorConfig::InteractionConfig& config)
{
    const QString scheme_name = QString::fromStdString(config.scheme_name);
    int scheme_idx = ui->comboBoxSchemeName->findText(scheme_name);
    if (scheme_idx >= 0)
    {
        ui->comboBoxSchemeName->setCurrentIndex(scheme_idx);
    }

    const QString filter_name = QString::fromStdString(config.filter_name);
    int filter_idx = ui->comboBoxFilterName->findText(filter_name);
    if (filter_idx >= 0)
    {
        ui->comboBoxFilterName->setCurrentIndex(filter_idx);
    }

    ui->comboBoxControlJoint->setCurrentText(Utils::ToString(config.control_joint));

    syncFilterStackByName(filter_name);
    applyFilterParamsToUi(filter_name, config.vec_filter_param);
}

void SettingPage::applyMapperConfigToUi(const Config::OrchestratorConfig::MapperConfig& config)
{
    ui->doubleSpinBoxXSensitivity->setValue(config.x_sensitivity);
    ui->doubleSpinBoxYSensitivity->setValue(config.y_sensitivity);
    ui->doubleSpinBoxScrollSensitivity->setValue(config.scroll_sensitivity);

    setMoveModeToUi(config.move_mode);

    ui->spinBoxRegionLeftPercent->setValue(static_cast<int>(config.region.left * 100.0f));
    ui->spinBoxRegionTopPercent->setValue(static_cast<int>(config.region.top * 100.0f));
    ui->spinBoxRegionRightPercent->setValue(static_cast<int>(config.region.right * 100.0f));
    ui->spinBoxRegionBottomPercent->setValue(static_cast<int>(config.region.bottom * 100.0f));
}

bool SettingPage::parseCameraResolutionText(const QString& text, CameraResolution& resolution) const
{
    QString normalized = text.trimmed();
    normalized.replace(QChar(0x00D7), QLatin1String("x"));
    normalized.replace(QStringLiteral("脳"), QStringLiteral("x"));
    normalized.replace(QStringLiteral("X"), QStringLiteral("x"));
    normalized.remove(' ');

    const QStringList parts = normalized.split('x', Qt::SkipEmptyParts);
    if (parts.size() != 2)
    {
        LOG(ERROR) << "Invalid camera resolution text: " << text.toStdString();
        return false;
    }

    bool ok_w = false;
    bool ok_h = false;
    const int width = parts[0].toInt(&ok_w);
    const int height = parts[1].toInt(&ok_h);
    if (!ok_w || !ok_h || width <= 0 || height <= 0)
    {
        LOG(ERROR) << "Failed to parse camera resolution: " << text.toStdString();
        return false;
    }

    resolution.width = width;
    resolution.height = height;
    return true;
}

QString SettingPage::cameraResolutionToText(const CameraResolution& resolution) const
{
    return QStringLiteral("%1 × %2").arg(resolution.width).arg(resolution.height);
}

int SettingPage::findResolutionIndex(const CameraResolution& resolution) const
{
    const QString resolution_text = cameraResolutionToText(resolution);
    int resolution_idx = ui->comboBoxCameraResolution->findText(resolution_text);
    if (resolution_idx >= 0)
    {
        return resolution_idx;
    }

    for (int i = 0; i < ui->comboBoxCameraResolution->count(); ++i)
    {
        CameraResolution candidate;
        if (parseCameraResolutionText(ui->comboBoxCameraResolution->itemText(i), candidate) &&
            candidate.width == resolution.width &&
            candidate.height == resolution.height)
        {
            return i;
        }
    }

    return -1;
}

void SettingPage::refreshResolutionListForDevice(const QString& device_name, const QString& preferred_resolution_text)
{
    if (device_name.trimmed().isEmpty())
    {
        return;
    }

    auto& core = CoreManager::instance().core();
    const CameraInfo camera_info = core.getCameraInfo(device_name.toStdString());

    QStringList new_resolution_texts;
    QList<QVariant> new_resolution_data;
    new_resolution_texts.reserve(static_cast<int>(camera_info.set_supported_resolution.size()));
    new_resolution_data.reserve(static_cast<int>(camera_info.set_supported_resolution.size()));
    for (const CameraResolution& res : camera_info.set_supported_resolution)
    {
        const QString text = QStringLiteral("%1 × %2").arg(res.width).arg(res.height);
        const QVariant data = QVariant::fromValue(QSize(res.width, res.height));
        new_resolution_texts.push_back(text);
        new_resolution_data.push_back(data);
    }

    if (new_resolution_texts.isEmpty())
    {
        // Keep previous list when backend query fails temporarily.
        return;
    }

    const QString current_resolution_text = ui->comboBoxCameraResolution->currentText().trimmed();
    const QSignalBlocker blocker(ui->comboBoxCameraResolution);

    ui->comboBoxCameraResolution->clear();
    for (int i = 0; i < new_resolution_texts.size(); ++i)
    {
        ui->comboBoxCameraResolution->addItem(new_resolution_texts[i], new_resolution_data[i]);
    }

    QString preferred_text = preferred_resolution_text.trimmed();
    if (preferred_text.isEmpty())
    {
        preferred_text = current_resolution_text;
    }

    int resolution_idx = ui->comboBoxCameraResolution->findText(preferred_text);
    if (resolution_idx < 0)
    {
        const Config::OrchestratorConfig config = CoreManager::instance().config();
        resolution_idx = findResolutionIndex(config.camera.camera_resolution);
    }
    if (resolution_idx < 0)
    {
        resolution_idx = 0;
    }
    ui->comboBoxCameraResolution->setCurrentIndex(resolution_idx);
}

void SettingPage::ensureResolutionListHasContent(const QString& preferred_resolution_text)
{
    if (ui->comboBoxCameraResolution->count() > 0)
    {
        return;
    }

    for (const CameraResolution& res : ARR_SUGGEST_RESOLUTIONS)
    {
        const QString text = QStringLiteral("%1 × %2").arg(res.width).arg(res.height);
        const QVariant data = QVariant::fromValue(QSize(res.width, res.height));
        ui->comboBoxCameraResolution->addItem(text, data);
    }

    if (ui->comboBoxCameraResolution->count() == 0)
    {
        return;
    }

    int resolution_idx = ui->comboBoxCameraResolution->findText(preferred_resolution_text.trimmed());
    if (resolution_idx < 0)
    {
        const Config::OrchestratorConfig config = CoreManager::instance().config();
        resolution_idx = findResolutionIndex(config.camera.camera_resolution);
    }
    if (resolution_idx < 0)
    {
        resolution_idx = 0;
    }
    ui->comboBoxCameraResolution->setCurrentIndex(resolution_idx);
}

void SettingPage::restoreGestureSectionChineseText()
{
    ui->groupBoxInteraction->setTitle(QStringLiteral("\u624B\u52BF\u63A7\u5236\u65B9\u6848"));
    ui->labelSchemeNameText->setText(QStringLiteral("\u65B9\u6848\u540D\u79F0"));
    ui->labelFilterNameText->setText(QStringLiteral("\u5E73\u6ED1\u6EE4\u6CE2\u7C7B\u578B"));
    ui->labelControlJointText->setText(QStringLiteral("\u6307\u9488\u79FB\u52A8\u63A7\u5236\u5173\u8282"));
    ui->labelFilterNoneHint->setText(
        QStringLiteral("\u5F53\u524D\u672A\u542F\u7528\u6EE4\u6CE2\uFF0C\u65E0\u9700\u586B\u5199\u53C2\u6570\u3002"));
}

MoveMode SettingPage::currentMoveModeFromUi() const
{
    MoveMode mode = MoveMode::Absolute;
    if (Utils::parseMoveMode(ui->comboBoxMoveMode->currentText().trimmed(), mode))
    {
        return mode;
    }
    return MoveMode::Absolute;
}

void SettingPage::setMoveModeToUi(MoveMode mode)
{
    ui->comboBoxMoveMode->setCurrentText(Utils::ToString(mode));
}

void SettingPage::syncFilterStackByName(const QString& filter_name)
{
    const QString name = filter_name.trimmed().toLower();

    if (name == QStringLiteral("none"))
    {
        ui->stackedWidgetFilterParams->setCurrentIndex(0);
    }
    else if (name == QStringLiteral("ema"))
    {
        ui->stackedWidgetFilterParams->setCurrentIndex(1);
    }
    else if (name == QStringLiteral("sma"))
    {
        ui->stackedWidgetFilterParams->setCurrentIndex(2);
    }
    else if (name == QStringLiteral("one_euro"))
    {
        ui->stackedWidgetFilterParams->setCurrentIndex(3);
    }
    else
    {
        ui->stackedWidgetFilterParams->setCurrentIndex(0);
    }
}

std::vector<float> SettingPage::collectFilterParamsFromUi(const QString& filter_name) const
{
    const QString name = filter_name.trimmed().toLower();

    if (name == QStringLiteral("ema"))
    {
        return {static_cast<float>(ui->doubleSpinBoxEmaAlpha->value())};
    }
    if (name == QStringLiteral("sma"))
    {
        return {static_cast<float>(ui->spinBoxSmaWindowSize->value())};
    }
    if (name == QStringLiteral("one_euro"))
    {
        return {static_cast<float>(ui->doubleSpinBoxOneEuroMinCutoff->value()),
                static_cast<float>(ui->doubleSpinBoxOneEuroBeta->value()),
                static_cast<float>(ui->doubleSpinBoxOneEuroDCutoff->value()),
                static_cast<float>(ui->doubleSpinBoxOneEuroFrequency->value())};
    }
    return {};
}

void SettingPage::applyFilterParamsToUi(const QString& filter_name, const std::vector<float>& vec_filter_param)
{
    const QString name = filter_name.trimmed().toLower();

    if (name == QStringLiteral("ema"))
    {
        if (vec_filter_param.size() >= 1)
        {
            ui->doubleSpinBoxEmaAlpha->setValue(vec_filter_param[0]);
        }
        return;
    }
    if (name == QStringLiteral("sma"))
    {
        if (vec_filter_param.size() >= 1)
        {
            ui->spinBoxSmaWindowSize->setValue(static_cast<int>(vec_filter_param[0]));
        }
        return;
    }
    if (name == QStringLiteral("one_euro"))
    {
        if (vec_filter_param.size() >= 1)
        {
            ui->doubleSpinBoxOneEuroMinCutoff->setValue(vec_filter_param[0]);
        }
        if (vec_filter_param.size() >= 2)
        {
            ui->doubleSpinBoxOneEuroBeta->setValue(vec_filter_param[1]);
        }
        if (vec_filter_param.size() >= 3)
        {
            ui->doubleSpinBoxOneEuroDCutoff->setValue(vec_filter_param[2]);
        }
        if (vec_filter_param.size() >= 4)
        {
            ui->doubleSpinBoxOneEuroFrequency->setValue(vec_filter_param[3]);
        }
        return;
    }
}

void SettingPage::setBottomHint(const QString& text)
{
    ui->labelBottomHint->setText(text);
}

void SettingPage::showResultMessage(bool success, const QString& success_text, const QString& failure_text)
{
    if (success)
    {
        setBottomHint(success_text.isEmpty() ? QStringLiteral("\u64CD\u4F5C\u6210\u529F\u3002") : success_text);
        return;
    }
    setBottomHint(failure_text.isEmpty() ? QStringLiteral("\u64CD\u4F5C\u5931\u8D25\u3002") : failure_text);
}

void SettingPage::onRefreshCameraListClicked()
{
    refreshCameraDevices();
}

void SettingPage::onLoadConfigClicked()
{
    loadConfig();
}

void SettingPage::onSaveConfigClicked()
{
    saveConfig();
}

void SettingPage::onApplyConfigClicked()
{
    if (applyConfig())
    {
        emit reStartCamera();
    }
}

void SettingPage::onResetConfigClicked()
{
    resetConfig();
}

void SettingPage::onFilterNameChanged(int index)
{
    Q_UNUSED(index);
    syncFilterStackByName(ui->comboBoxFilterName->currentText());
}

void SettingPage::onCurrentChooseDeviceChanged(QString device_name)
{
    const QString normalized_device_name = device_name.trimmed();
    ui->lineEditCameraDeviceName->setText(normalized_device_name);

    const QString previous_resolution_text = ui->comboBoxCameraResolution->currentText().trimmed();
    refreshResolutionListForDevice(normalized_device_name, previous_resolution_text);
    ensureResolutionListHasContent(previous_resolution_text);
}

void SettingPage::onSchemeComboBoxPressed()
{
    if (ui == nullptr || ui->comboBoxSchemeName == nullptr)
    {
        return;
    }

    const QString current_text = ui->comboBoxSchemeName->currentText().trimmed();

    QStringList preset_names;
    preset_names << "easy" << "advanced" << "tiktok";

    const QString dir_path = Config::FsmConfig::defaultPath();
    QDir dir(dir_path);

    if (dir.exists())
    {
        const QFileInfoList file_list =
            dir.entryInfoList(QStringList() << "*.json", QDir::Files | QDir::Readable, QDir::Name);

        for (const QFileInfo& file_info : file_list)
        {
            const QString file_name = file_info.completeBaseName().trimmed();
            if (file_name.isEmpty())
            {
                continue;
            }
            if (!preset_names.contains(file_name, Qt::CaseInsensitive))
            {
                preset_names.push_back(file_name);
            }
        }
    }

    QSignalBlocker blocker(ui->comboBoxSchemeName);
    ui->comboBoxSchemeName->clear();
    ui->comboBoxSchemeName->addItems(preset_names);

    int target_index = 0;
    const int old_index = ui->comboBoxSchemeName->findText(current_text, Qt::MatchFixedString);
    if (old_index >= 0)
    {
        target_index = old_index;
    }
    ui->comboBoxSchemeName->setCurrentIndex(target_index);

    if (!dir.exists())
    {
        setBottomHint(QString("方案目录不存在，仅显示内置方案：%1").arg(dir_path));
    }
}

bool SettingPage::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == ui->comboBoxSchemeName && event->type() == QEvent::MouseButtonPress)
    {
        onSchemeComboBoxPressed();
    }
    else if (watched == ui->comboBoxCameraResolution && event->type() == QEvent::MouseButtonPress)
    {
        // 每次点击分辨率下拉框，都重新获取当前摄像头的支持分辨率列表。
        onCurrentChooseDeviceChanged(ui->comboBoxCameraDevice->currentText().trimmed());
    }

    return QWidget::eventFilter(watched, event);
}

void SettingPage::showEvent(QShowEvent* event)
{
    QWidget::showEvent(event);

    // 每次进入页面时手动刷新一次。运行中仅刷新当前设备分辨率，避免无效报错提示。
    auto& core = CoreManager::instance().core();
    if (!core.isRunning())
    {
        refreshCameraDevices();
    }

    refreshResolutionListForDevice(ui->comboBoxCameraDevice->currentText().trimmed(),
                                   ui->comboBoxCameraResolution->currentText().trimmed());
    ensureResolutionListHasContent(ui->comboBoxCameraResolution->currentText().trimmed());
}

} // namespace VisionCursor


