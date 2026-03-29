#pragma once


#include <QWidget>

namespace Ui
{
class InformationPage;
}

namespace VisionCursor
{

class InformationPage : public QWidget
{
    Q_OBJECT

public:
    explicit InformationPage(QWidget* parent = nullptr);
    ~InformationPage();

private:
    void initDescriptionText();

private:
    Ui::InformationPage* ui = nullptr;
};

}      // namespace VisionCursor
