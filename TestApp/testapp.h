#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_testapp.h"

class TestApp : public QMainWindow
{
    Q_OBJECT

public:
    TestApp(QWidget *parent = nullptr);
    ~TestApp();

private:
    Ui::TestAppClass ui;
};
