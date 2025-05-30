#include <QtCore/QCoreApplication>
#include "lib4dicom.h""
#include <QDebug>
#include <QImage>
#include <windows.h>
#include <QApplication>
#include <QCoreApplication>
#include <QApplication>
#include <qqmlapplicationengine.h>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include <qevent.h>

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);    
    app.setWindowIcon(QIcon(":\\qml\\icon.png"));
    //app.setWindowIcon(QIcon("C:\\my_files\\DICOM\\Lib4DICOM\\Lib4DICOM\\qml\\icon.png"));

    qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion"); //Стиль интерфейса

    QQmlApplicationEngine engine; //Движок QML 

    Lib4DICOM* lib = new Lib4DICOM(); 

    engine.rootContext()->setContextProperty("appLogic", lib); //Передача в QML объекта для управления

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml"))); //Загрузка интерфейса
    qputenv("QML_IMPORT_TRACE", "1");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}











