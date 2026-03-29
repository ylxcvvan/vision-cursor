#include "scheme_control_page.h"
#include "ui_scheme_control_page.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QComboBox>
#include <QPushButton>
#include <QTextCursor>
#include <QTextStream>

namespace VisionCursor
{
namespace
{

QString defaultJsonRuleText()
{
    return QStringLiteral(
        "JSON 编写规则说明\n"
        "\n"
        "一、顶层结构\n"
        "1. 顶层是一个 JSON 对象，可包含：\n"
        "   - motion_transitions：运动状态迁移数组\n"
        "   - button_transitions：控制状态迁移数组\n"
        "2. 迁移项统一包含字段：from、to、priority、rule。\n"
        "\n"
        "二、可用状态\n"
        "1. MotionState：Idle / Ready / Moving\n"
        "2. ControlState：None / LeftPressed / RightPressed / VerticalScrolling / HorizontalScrolling\n"
        "\n"
        "三、可用规则类型\n"
        "1. BoolRule\n"
        "2. HandDetectRule\n"
        "3. DistanceRule\n"
        "4. AngleRule\n"
        "5. MoveDistanceRule\n"
        "6. CompositeRule\n"
        "7. ConsecutiveFrameRule\n"
        "\n"
        "四、关键字段说明\n"
        "1. DistanceRule：joint_a、joint_b、compare_op、threshold\n"
        "2. AngleRule：joint_a、joint_b、joint_c、compare_op、threshold_degree\n"
        "3. MoveDistanceRule：joint、compare_op、threshold、frame_window\n"
        "4. CompositeRule：logic_op（And/Or）、left_rule、right_rule\n"
        "5. ConsecutiveFrameRule：required_frames、base_rule\n"
        "\n"
        "五、运算符\n"
        "1. CompareOp：Less / LessEqual / Greater / GreaterEqual / Equal / NotEqual\n"
        "2. LogicOp：And / Or\n"
        "\n"
        "六、建议\n"
        "1. 需要更稳定触发时，建议在外层包 ConsecutiveFrameRule。\n"
        "2. 多条件同时成立使用 CompositeRule + And。\n"
        "3. 任一条件成立使用 CompositeRule + Or。\n"
        "4. 可先生成 easy / advanced / tiktok 预设，再按需微调。\n");
}

QString presetExplainTextInternal(const QString& scheme_name)
{
    if (scheme_name == QStringLiteral("easy"))
    {
        return QStringLiteral(
            "【easy 方案说明】\n"
            "\n"
            "定位：基础、稳妥、通用。\n"
            "1. 运动状态：Idle / Ready / Moving。\n"
            "2. 进入 Moving：食指两段伸直连续 2 帧。\n"
            "3. 退出 Moving：食指任一段弯曲连续 2 帧。\n"
            "4. 左键：拇指-食指距离阈值控制。\n"
            "5. 右键：拇指-中指距离阈值控制。\n"
            "6. 滚动：拇指-无名指距离阈值控制。\n");
    }
    if (scheme_name == QStringLiteral("advanced"))
    {
        return QStringLiteral(
            "【advanced 方案说明】\n"
            "\n"
            "定位：更丰富手势、更严格触发。\n"
            "1. 运动进入条件：食指和中指伸直，且两指尖贴近，连续 2 帧。\n"
            "2. 运动退出条件：食指与中指分开达到阈值，连续 10 帧。\n"
            "3. 左键：拇指与无名指尖距离触发。\n"
            "4. 右键：拇指与无名指 PIP 距离触发。\n"
            "5. 滚动：拇指与食指尖距离触发。\n");
    }
    if (scheme_name == QStringLiteral("tiktok"))
    {
        return QStringLiteral(
            "【tiktok 方案说明】\n"
            "\n"
            "定位：面向短视频上下滑场景的滚动优先方案。\n"
            "1. 运动状态仅使用 Idle / Ready，不使用 Moving。\n"
            "2. Idle -> Ready：检测到手，连续 3 帧。\n"
            "3. Ready -> Idle：手消失，连续 5 帧。\n"
            "4. None -> VerticalScrolling 触发条件（必须同时满足）：\n"
            "   - 拇指与食指贴合：DistanceRule(ThumbTIP, IndexTIP, <= 4.0)，连续 2 帧；\n"
            "   - 食指移动距离：MoveDistanceRule(IndexTIP, >= 15.0, frame_window=5)。\n"
            "5. VerticalScrolling -> None：ConsecutiveFrameRule(BoolRule(true), 30)。\n"
            "6. 适合快速浏览，不强调左键/右键等通用桌面操作。\n");
    }
    return QStringLiteral("当前预设暂无内置说明。");
}

QString ensureJsonSuffix(QString file_path)
{
    if (!file_path.endsWith(QStringLiteral(".json"), Qt::CaseInsensitive))
    {
        file_path += QStringLiteral(".json");
    }
    return file_path;
}

} // namespace

SchemeControlPage::SchemeControlPage(QWidget* parent) : QWidget(parent), ui(new Ui::SchemeControlPage)
{
    ui->setupUi(this);
}

SchemeControlPage::~SchemeControlPage()
{
    delete ui;
}

void SchemeControlPage::init()
{
    initUiState();
    initConnections();
}

void SchemeControlPage::initUiState()
{
    initPresetSchemes();
    ui->plainTextEditSchemeJson->clear();
    showDefaultJsonRuleText();
    setBottomHint(QStringLiteral("可在此生成、编辑并测试 FSM JSON 方案。"));
}

void SchemeControlPage::initConnections()
{
    connect(ui->comboBoxPresetScheme, &QComboBox::currentTextChanged, this, &SchemeControlPage::onPresetSchemeChanged);
    connect(ui->btnGeneratePreset, &QPushButton::clicked, this, &SchemeControlPage::onGeneratePresetClicked);
    connect(ui->btnExplainPreset, &QPushButton::clicked, this, &SchemeControlPage::onExplainPresetClicked);
    connect(ui->btnLoadJson, &QPushButton::clicked, this, &SchemeControlPage::onLoadJsonClicked);
    connect(ui->btnSaveJson, &QPushButton::clicked, this, &SchemeControlPage::onSaveJsonClicked);
    connect(ui->btnTestScheme, &QPushButton::clicked, this, &SchemeControlPage::onTestSchemeClicked);
}

void SchemeControlPage::initPresetSchemes()
{
    ui->comboBoxPresetScheme->clear();
    ui->comboBoxPresetScheme->addItem(QStringLiteral("easy"));
    ui->comboBoxPresetScheme->addItem(QStringLiteral("advanced"));
    ui->comboBoxPresetScheme->addItem(QStringLiteral("tiktok"));
    ui->comboBoxPresetScheme->setCurrentIndex(0);
}

void SchemeControlPage::setBottomHint(const QString& text)
{
    ui->labelBottomHint->setText(text);
}

void SchemeControlPage::showDefaultJsonRuleText()
{
    setRuleText(defaultJsonRuleText());
}

void SchemeControlPage::showPresetExplainText(const QString& scheme_name)
{
    setRuleText(presetExplainTextInternal(scheme_name));
}

void SchemeControlPage::setSchemeJsonText(const QString& text)
{
    ui->plainTextEditSchemeJson->setPlainText(text);
    ui->plainTextEditSchemeJson->moveCursor(QTextCursor::Start);
}

void SchemeControlPage::setRuleText(const QString& text)
{
    ui->plainTextEditJsonRule->setReadOnly(true);
    ui->plainTextEditJsonRule->setPlainText(text);
    ui->plainTextEditJsonRule->moveCursor(QTextCursor::Start);
}

QString SchemeControlPage::currentPresetName() const
{
    return ui->comboBoxPresetScheme->currentText().trimmed();
}

ControlPreset SchemeControlPage::currentPresetEnum(const QString& preset_name) const
{
    if (preset_name == QStringLiteral("easy"))
    {
        return ControlPreset::Easy;
    }
    if (preset_name == QStringLiteral("advanced"))
    {
        return ControlPreset::Advanced;
    }
    if (preset_name == QStringLiteral("tiktok"))
    {
        return ControlPreset::Tiktok;
    }
    return ControlPreset::Easy;
}

QString SchemeControlPage::presetFilePath(const QString& preset_name) const
{
    return Config::FsmConfig::defaultPath() + QStringLiteral("/") + preset_name + QStringLiteral(".json");
}

QString SchemeControlPage::selectJsonFileToOpen() const
{
    const QString dir_path = Config::FsmConfig::defaultPath();
    QDir dir;
    if (!dir.exists(dir_path))
    {
        dir.mkpath(dir_path);
    }

    return QFileDialog::getOpenFileName(const_cast<SchemeControlPage*>(this),
                                        QStringLiteral("打开 JSON 方案"),
                                        dir_path,
                                        QStringLiteral("JSON 文件 (*.json);;所有文件 (*)"));
}

QString SchemeControlPage::selectJsonFileToSave() const
{
    QString default_name = currentPresetName().trimmed() + "_modified";
    if (default_name.isEmpty())
    {
        default_name = QStringLiteral("scheme");
    }

    const QString dir_path = Config::FsmConfig::defaultPath();
    QDir dir;
    if (!dir.exists(dir_path))
    {
        dir.mkpath(dir_path);
    }

    const QString default_file_path = dir_path + "/" + default_name + ".json";

    QString file_path = QFileDialog::getSaveFileName(const_cast<SchemeControlPage*>(this),
                                                     QStringLiteral("保存 JSON 方案"),
                                                     default_file_path,
                                                     QStringLiteral("JSON 文件 (*.json);;所有文件 (*)"));
    return ensureJsonSuffix(file_path);
}

bool SchemeControlPage::readTextFile(const QString& file_path, QString& out_text, QString* error_text) const
{
    QFile file(file_path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        if (error_text != nullptr)
        {
            *error_text = QStringLiteral("打开读取文件失败。");
        }
        return false;
    }

    QTextStream in(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    in.setEncoding(QStringConverter::Utf8);
#endif
    out_text = in.readAll();
    return true;
}

bool SchemeControlPage::writeTextFile(const QString& file_path,
                                      const QString& text,
                                      bool overwrite,
                                      QString* error_text) const
{
    QFile file(file_path);
    if (file.exists() && !overwrite)
    {
        if (error_text != nullptr)
        {
            *error_text = QStringLiteral("目标文件已存在。");
        }
        return false;
    }

    QDir parent_dir = QFileInfo(file_path).dir();
    if (!parent_dir.exists())
    {
        parent_dir.mkpath(QStringLiteral("."));
    }

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate))
    {
        if (error_text != nullptr)
        {
            *error_text = QStringLiteral("打开写入文件失败。");
        }
        return false;
    }

    QTextStream out(&file);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    out.setEncoding(QStringConverter::Utf8);
#endif
    out << text;
    out.flush();
    return true;
}

bool SchemeControlPage::generateCurrentPresetFile(QString* error_text)
{
    const QString preset_name = currentPresetName();
    const ControlPreset preset = currentPresetEnum(preset_name);
    if (!Config::FsmConfig::generatePreset(preset_name, preset, true))
    {
        if (error_text != nullptr)
        {
            *error_text = QStringLiteral("生成预设文件失败。");
        }
        return false;
    }
    return true;
}

bool SchemeControlPage::loadJsonFromUserSelectedFile(QString* error_text)
{
    const QString file_path = selectJsonFileToOpen();
    if (file_path.isEmpty())
    {
        if (error_text != nullptr)
        {
            *error_text = QStringLiteral("用户取消打开文件。");
        }
        return false;
    }

    QString text;
    if (!readTextFile(file_path, text, error_text))
    {
        return false;
    }

    setSchemeJsonText(text);
    setBottomHint(QStringLiteral("已读取：") + file_path);
    return true;
}

bool SchemeControlPage::saveJsonToUserSelectedFile(QString* error_text)
{
    const QString file_path = selectJsonFileToSave();
    if (file_path.isEmpty())
    {
        if (error_text != nullptr)
        {
            *error_text = QStringLiteral("用户取消保存文件。");
        }
        return false;
    }

    const QString text = ui->plainTextEditSchemeJson->toPlainText();
    if (!writeTextFile(file_path, text, true, error_text))
    {
        return false;
    }

    setBottomHint(QStringLiteral("已保存：") + file_path);
    return true;
}

void SchemeControlPage::onPresetSchemeChanged(const QString& text)
{
    showDefaultJsonRuleText();
    setBottomHint(QStringLiteral("已切换预设：") + text);
}

void SchemeControlPage::onGeneratePresetClicked()
{
    QString error_text;
    if (!generateCurrentPresetFile(&error_text))
    {
        setBottomHint(QStringLiteral("生成预设失败：") + error_text);
        return;
    }

    const QString preset_name = currentPresetName();
    const QString file_path = presetFilePath(preset_name);
    setBottomHint(QStringLiteral("已生成预设文件：") + file_path);

    QString text;
    if (readTextFile(file_path, text))
    {
        setSchemeJsonText(text);
    }
}

void SchemeControlPage::onExplainPresetClicked()
{
    const QString preset_name = currentPresetName();
    showPresetExplainText(preset_name);
    setBottomHint(QStringLiteral("已显示预设说明：") + preset_name);
}

void SchemeControlPage::onLoadJsonClicked()
{
    QString error_text;
    if (!loadJsonFromUserSelectedFile(&error_text))
    {
        if (error_text == QStringLiteral("用户取消打开文件。"))
        {
            setBottomHint(QStringLiteral("已取消读取。"));
        }
        else
        {
            setBottomHint(QStringLiteral("读取失败：") + error_text);
        }
    }
}

void SchemeControlPage::onSaveJsonClicked()
{
    QString error_text;
    if (!saveJsonToUserSelectedFile(&error_text))
    {
        if (error_text == QStringLiteral("用户取消保存文件。"))
        {
            setBottomHint(QStringLiteral("已取消保存。"));
        }
        else
        {
            setBottomHint(QStringLiteral("保存失败：") + error_text);
        }
    }
}

void SchemeControlPage::onTestSchemeClicked()
{
    if (ui == nullptr)
    {
        return;
    }

    const QString json_text = ui->plainTextEditSchemeJson->toPlainText().trimmed();
    if (json_text.isEmpty())
    {
        setBottomHint(QStringLiteral("当前 JSON 为空，无法测试。"));
        return;
    }

    const QString test_preset_name = "__vc_temp_test_scheme__";
    const QString dir_path = Config::FsmConfig::defaultPath();
    const QString file_path = dir_path + "/" + test_preset_name + ".json";

    QDir dir;
    if (!dir.exists(dir_path) && !dir.mkpath(dir_path))
    {
        setBottomHint(QStringLiteral("测试失败：无法创建方案目录。"));
        return;
    }

    QString write_error;
    if (!writeTextFile(file_path, json_text, true, &write_error))
    {
        setBottomHint(QStringLiteral("测试失败：%1").arg(write_error));
        return;
    }

    Interaction::FSM::RuleSet rules;
    const bool load_ok = Config::FsmConfig::load(rules, test_preset_name);

    QFile::remove(file_path);

    if (!load_ok)
    {
        setBottomHint(QStringLiteral("方案测试失败：JSON 或规则无效。"));
        return;
    }

    setBottomHint(QStringLiteral("方案测试通过。"));
}

} // namespace VisionCursor
