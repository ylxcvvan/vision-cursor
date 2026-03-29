#include "information_page.h"
#include "ui_information_page.h"

#include <QMessageBox>
#include <QPushButton>

namespace VisionCursor
{

InformationPage::InformationPage(QWidget* parent) : QWidget(parent), ui(new Ui::InformationPage)
{
    ui->setupUi(this);

    connect(ui->btnAboutQt, &QPushButton::clicked, this, [this]() {
        QMessageBox::aboutQt(this, tr("关于 Qt"));
    });

    initDescriptionText();
}

InformationPage::~InformationPage()
{
    delete ui;
}

void InformationPage::initDescriptionText()
{
    const QString os_version             = QStringLiteral("Windows 11 24H2");
    const QString compiler_version       = QStringLiteral("MSVC 2022 64bit");
    const QString qt_version             = QStringLiteral("Qt 6.8.3 LTS");
    const QString opencv_version         = QStringLiteral("OpenCV 4.7");
    const QString mediapipe_version      = QStringLiteral("MediaPipe 0.8.11");
    const QString cmake_version          = QStringLiteral("CMake 4.2.1");
    const QString python_version         = QStringLiteral("Python 3.12");
    const QString llvm_version           = QStringLiteral("LLVM 15.0");
    const QString bazel_version          = QStringLiteral("Bazel 5.2.0");
    const QString concurrentqueue_version = QStringLiteral("ConcurrentQueue 1.0.0");
    const QString gtest_version          = QStringLiteral("GoogleTest 1.17.0");

    const QString qt_creator_version     = QStringLiteral("Qt Creator 18.0.1");
    const QString qt_designer_version    = QStringLiteral("Qt Designer 6.8.3");
    const QString vscode_version         = QStringLiteral("2026");
    const QString git_version            = QStringLiteral("Git 2.52");
    const QString cmake_gui_version      = QStringLiteral("CMake GUI 4.21");

    ui->textBrowserSystemDesc->setHtml(QStringLiteral(R"HTML(
<html>
<head>
<style>
body { font-family: "Microsoft YaHei", "Segoe UI", sans-serif; color: #1f2937; line-height: 1.68; }
h2 { color: #1e40af; margin: 10px 0 8px 0; font-size: 18px; }
h3 { color: #334155; margin: 12px 0 6px 0; font-size: 15px; }
p  { margin: 2px 0 9px 0; }
table { border-collapse: collapse; width: 100%; margin: 6px 0 10px 0; }
th, td { border: 1px solid #dbe3ee; padding: 8px; text-align: left; vertical-align: top; }
th { background: #eff6ff; color: #1e3a8a; font-weight: 700; }
td { background: #ffffff; }
a { color: #2563eb; text-decoration: none; }
a:hover { text-decoration: underline; }
</style>
</head>
<body>
<h2>VisionCursor 系统说明</h2>
<p>VisionCursor 是一个基于摄像头手势识别的桌面交互系统，核心链路为“图像采集 → 关键点检测 → 规则状态机 → 鼠标事件输出”。</p>

<h3>一、项目环境与框架版本</h3>
<table>
  <tr><th style="width: 26%;">组件</th><th style="width: 22%;">版本</th><th>用途说明</th></tr>
  <tr><td>操作系统</td><td>%1</td><td>系统开发与运行环境</td></tr>
  <tr><td>C++ 编译器</td><td>%2</td><td>程序编译与构建</td></tr>
  <tr><td>Qt</td><td>%3</td><td>图形界面开发框架</td></tr>
  <tr><td>OpenCV</td><td>%4</td><td>图像采集与处理</td></tr>
  <tr><td>MediaPipe</td><td>%5</td><td>手部关键点检测</td></tr>
  <tr><td>CMake</td><td>%6</td><td>项目构建与管理</td></tr>
  <tr><td>Python</td><td>%7</td><td>MediaPipe 构建环境</td></tr>
  <tr><td>LLVM</td><td>%8</td><td>MediaPipe 构建环境</td></tr>
  <tr><td>Bazel</td><td>%9</td><td>MediaPipe 构建工具链</td></tr>
  <tr><td>ConcurrentQueue</td><td>%10</td><td>无锁并发队列</td></tr>
  <tr><td>GoogleTest</td><td>%11</td><td>单元测试框架</td></tr>
</table>

<h3>二、开发工具版本</h3>
<table>
  <tr><th style="width: 26%;">工具名称</th><th style="width: 22%;">版本</th><th>用途说明</th></tr>
  <tr><td>Qt Creator</td><td>%12</td><td>C++ 项目开发与调试</td></tr>
  <tr><td>Qt Designer</td><td>%13</td><td>图形界面设计工具</td></tr>
  <tr><td>Visual Studio Code</td><td>%14</td><td>代码编辑与辅助开发</td></tr>
  <tr><td>Git</td><td>%15</td><td>版本控制</td></tr>
  <tr><td>CMake GUI</td><td>%16</td><td>构建配置工具</td></tr>
</table>

<h3>三、第三方开源库与链接</h3>
<p>项目集成第三方库时，需遵循各自仓库/官网给出的开源许可证与使用规则（以对应 LICENSE 文本为准），在使用、修改和分发时保留必要的版权与许可声明。</p>
<table>
  <tr><th style="width: 24%;">名称</th><th style="width: 30%;">开源规则</th><th>链接</th></tr>
  <tr><td>libmediapipe</td><td>遵循仓库 LICENSE</td><td><a href="https://github.com/cpvrlab/libmediapipe">https://github.com/cpvrlab/libmediapipe</a></td></tr>
  <tr><td>ConcurrentQueue</td><td>遵循仓库 LICENSE</td><td><a href="https://github.com/cameron314/concurrentqueue">https://github.com/cameron314/concurrentqueue</a></td></tr>
  <tr><td>OpenCV</td><td>遵循官方发布版本许可证</td><td><a href="https://opencv.org/">https://opencv.org/</a></td></tr>
  <tr><td>MediaPipe</td><td>遵循官方仓库 LICENSE</td><td><a href="https://github.com/google-ai-edge/mediapipe">https://github.com/google-ai-edge/mediapipe</a></td></tr>
  <tr><td>GoogleTest</td><td>遵循官方仓库 LICENSE</td><td><a href="https://github.com/google/googletest">https://github.com/google/googletest</a></td></tr>
</table>
</body>
</html>
)HTML")
                                         .arg(os_version)
                                         .arg(compiler_version)
                                         .arg(qt_version)
                                         .arg(opencv_version)
                                         .arg(mediapipe_version)
                                         .arg(cmake_version)
                                         .arg(python_version)
                                         .arg(llvm_version)
                                         .arg(bazel_version)
                                         .arg(concurrentqueue_version)
                                         .arg(gtest_version)
                                         .arg(qt_creator_version)
                                         .arg(qt_designer_version)
                                         .arg(vscode_version)
                                         .arg(git_version)
                                         .arg(cmake_gui_version));
}

}      // namespace VisionCursor
