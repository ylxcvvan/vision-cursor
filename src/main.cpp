#include <QApplication>
#include <QCoreApplication>
#include <QIcon>

#include "common/paths.hpp"
#include "log/logging.h"
#include "ui/mainwidget.h"

int main(int argc, char* argv[])
{
    using namespace VisionCursor;

    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/vision_cursor.png"));

    Paths::init(QCoreApplication::applicationDirPath().toStdString());

    Log::Init("VisionCursor");
    LOG(INFO) << "Logger started";

    mainWidget widget;
    widget.show();

    const int code = app.exec();
    Log::Shutdown();
    return code;
}
