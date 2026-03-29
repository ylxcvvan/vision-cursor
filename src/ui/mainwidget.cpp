#include "mainwidget.h"
#include "ui_mainwidget.h"

#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QPixmap>
#include <QStackedWidget>

#include "information_page.h"
#include "monitor_page.h"
#include "scheme_control_page.h"
#include "setting_page.h"

namespace VisionCursor
{

mainWidget::mainWidget(QWidget* parent) : QWidget(parent), ui(new Ui::mainWidget)
{
    ui->setupUi(this);

    setWindowIcon(QIcon(":/icons/vision_cursor.png"));

    const QPixmap logo_pixmap(":/icons/vision_cursor.png");
    if (!logo_pixmap.isNull())
    {
        ui->labelLogo->setText(QString());
        ui->labelLogo->setStyleSheet("background: transparent; border: none;");
        ui->labelLogo->setPixmap(
            logo_pixmap.scaled(ui->labelLogo->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
        ui->labelLogo->setAlignment(Qt::AlignCenter);
    }

    initPages();
    initNavigation();
    initConnections();
    initUiState();
    initPageStates();
}

mainWidget::~mainWidget()
{
    delete ui;
}

void mainWidget::initPages()
{
    monitor_page_        = new MonitorPage(this);
    setting_page_        = new SettingPage(this);
    scheme_control_page_ = new SchemeControlPage(this);
    information_page_    = new InformationPage(this);

    ui->stackedWidgetPages->addWidget(monitor_page_);
    ui->stackedWidgetPages->addWidget(setting_page_);
    ui->stackedWidgetPages->addWidget(scheme_control_page_);
    ui->stackedWidgetPages->addWidget(information_page_);
}

void mainWidget::initNavigation()
{
    ui->listWidgetNavigation->clear();

    const QStringList nav_texts = {
        QStringLiteral("实时监控"), QStringLiteral("基本设置"), QStringLiteral("手势控制"), QStringLiteral("关于系统")};

    for (const QString& text : nav_texts)
    {
        QListWidgetItem* item = new QListWidgetItem(text);
        item->setTextAlignment(Qt::AlignCenter);
        ui->listWidgetNavigation->addItem(item);
    }

    ui->listWidgetNavigation->setSpacing(4);
    ui->listWidgetNavigation->setUniformItemSizes(true);
}

void mainWidget::initConnections()
{
    connect(ui->listWidgetNavigation, &QListWidget::currentRowChanged, this, &mainWidget::onNavigationRowChanged);
    connect(this->setting_page_, &SettingPage::reStartCamera, this->monitor_page_, &MonitorPage::onSettingApply);
}

void mainWidget::initUiState()
{
    ui->listWidgetNavigation->setCurrentRow(MonitorPageIndex);
    ui->stackedWidgetPages->setCurrentIndex(MonitorPageIndex);
}

void mainWidget::initPageStates()
{
    if (monitor_page_)
        monitor_page_->init();

    if (setting_page_)
        setting_page_->init();

    if (scheme_control_page_)
        scheme_control_page_->init();
    // if (information_page_) information_page_->init();
}

void mainWidget::switchToPage(int index)
{
    if (index < 0 || index >= ui->stackedWidgetPages->count())
        return;

    ui->stackedWidgetPages->setCurrentIndex(index);
}

QWidget* mainWidget::pageAt(int index) const
{
    if (index < 0 || index >= ui->stackedWidgetPages->count())
        return nullptr;

    return ui->stackedWidgetPages->widget(index);
}

void mainWidget::onNavigationRowChanged(int current_row)
{
    switchToPage(current_row);
}

}      // namespace VisionCursor
