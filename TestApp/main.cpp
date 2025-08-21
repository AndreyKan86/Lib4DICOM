#include <QApplication>  // Используйте либо QGuiApplication, либо QApplication
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QIcon>
#include "lib4dicom.h"

#ifdef _MSC_VER
#include <crtdbg.h>   // <- обязательно для _Crt*
#endif


int main(int argc, char* argv[])
{
#ifdef _DEBUG
    // Проверять кучу на каждом alloc/free + дамп утечек при выходе
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF
        | _CRTDBG_LEAK_CHECK_DF
        | _CRTDBG_CHECK_ALWAYS_DF);
    // вывод ошибок в Output/Debug
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_DEBUG);
    // точечная проверка где надо:
    _CrtCheckMemory();
#endif
    QApplication app(argc, argv); 

    app.setWindowIcon(QIcon(":/qml/icon.png"));  

    qputenv("QT_QUICK_CONTROLS_STYLE", "Fusion");

    QQmlApplicationEngine engine;

    Lib4DICOM* lib = new Lib4DICOM(&engine);

    engine.rootContext()->setContextProperty("appLogic", lib);

    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}