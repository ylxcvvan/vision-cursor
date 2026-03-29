#pragma once

#include <QString>
#include <QWidget>

#include "config/fsm_config.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
class SchemeControlPage;
}
QT_END_NAMESPACE

namespace VisionCursor
{

class SchemeControlPage : public QWidget
{
    Q_OBJECT

public:
    explicit SchemeControlPage(QWidget* parent = nullptr);
    ~SchemeControlPage() override;

    void init();

private:
    void initUiState();
    void initConnections();
    void initPresetSchemes();

    void setBottomHint(const QString& text);
    void showDefaultJsonRuleText();
    void showPresetExplainText(const QString& scheme_name);
    void setSchemeJsonText(const QString& text);
    void setRuleText(const QString& text);

    QString currentPresetName() const;
    ControlPreset currentPresetEnum(const QString& preset_name) const;
    QString presetFilePath(const QString& preset_name) const;

    QString selectJsonFileToOpen() const;
    QString selectJsonFileToSave() const;

    bool readTextFile(const QString& file_path, QString& out_text, QString* error_text = nullptr) const;
    bool writeTextFile(const QString& file_path,
                       const QString& text,
                       bool overwrite,
                       QString* error_text = nullptr) const;

    bool generateCurrentPresetFile(QString* error_text = nullptr);
    bool loadJsonFromUserSelectedFile(QString* error_text = nullptr);
    bool saveJsonToUserSelectedFile(QString* error_text = nullptr);

private slots:
    void onPresetSchemeChanged(const QString& text);
    void onGeneratePresetClicked();
    void onExplainPresetClicked();
    void onLoadJsonClicked();
    void onSaveJsonClicked();
    void onTestSchemeClicked();

private:
    Ui::SchemeControlPage* ui = nullptr;
};

} // namespace VisionCursor
