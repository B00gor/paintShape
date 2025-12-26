#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include "vkcanvas.h"
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <qquickwindow.h>
#include <qsgrendererinterface.h>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    qmlRegisterType<VKCanvas>("VKCanvas", 1, 0, "VKCanvas");
    QQmlApplicationEngine engine;
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("paintShape", "Main");

    return app.exec();
}
