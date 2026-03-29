#pragma once

#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui
{
class mainWidget;
}
QT_END_NAMESPACE

namespace VisionCursor
{

class MonitorPage;
class SettingPage;
class SchemeControlPage;
class InformationPage;

/**
 * @brief 主界面类
 */
class mainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit mainWidget(QWidget* parent = nullptr);
    ~mainWidget();

    mainWidget(const mainWidget&)            = delete;
    mainWidget& operator=(const mainWidget&) = delete;

private:
    // ================= 初始化流程 =================
    void initPages();
    void initNavigation();
    void initConnections();
    void initUiState();
    void initPageStates();

private:
    // ================= 页面切换 =================
    void switchToPage(int index);
    QWidget* pageAt(int index) const;

private slots:
    void onNavigationRowChanged(int current_row);

private:
    Ui::mainWidget* ui = nullptr;

    // ================= 页面对象 =================
    MonitorPage* monitor_page_              = nullptr;
    SettingPage* setting_page_              = nullptr;
    SchemeControlPage* scheme_control_page_ = nullptr;
    InformationPage* information_page_      = nullptr;

    // ================= 页面索引 =================
    enum PageIndex
    {
        MonitorPageIndex = 0,
        SettingPageIndex,
        SchemeControlPageIndex,
        InformationPageIndex
    };
};

}      // namespace VisionCursor