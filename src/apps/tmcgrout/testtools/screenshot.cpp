// Снимок окна программы без участия человека.
//
// Нужен для проверки интерфейса: запускает то же самое главное окно, что и
// сама программа, рисует его в картинку и сохраняет. Работает без экрана
// (QT_QPA_PLATFORM=offscreen), поэтому годится и для проверки на сервере сборки.
//
// В поставку не входит: это инструмент разработки.
//
//   tmcgrout_shot <файл.png> [файл задания] [ширина] [высота]

#include <QApplication>

#include <cstdio>
#include <QPixmap>
#include <QTimer>

#include "mainwindow.h"
#include "dialogshot.h"
#include "theme.h"
#include "tmc_mfc_doc.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    CWinApp::SetAppName("TMCGROUT");
    theme::apply();

    const QStringList args = QApplication::arguments();

    // Режим проверки диалогов: строит каждое окно из ресурсов и сохраняет вид.
    // Нужен, чтобы глазами убедиться, что подписи помещаются и окно не обрезано.
    if (args.size() > 1 && args.at(1) == QStringLiteral("--dialogs")) {
        const QString dir = args.size() > 2 ? args.at(2) : QStringLiteral(".");
        MainWindow window;          // конструктор регистрирует разметку диалогов
        window.show();
        QApplication::processEvents();
        const int n = tmcshot::grabAllDialogs(dir);
        std::printf("снято окон: %d -> %s\n", n, dir.toLocal8Bit().constData());
        return 0;
    }

    const QString out = args.size() > 1 ? args.at(1) : QStringLiteral("tmcgrout.png");
    const QString doc = args.size() > 2 ? args.at(2) : QString();
    const int width  = args.size() > 3 ? args.at(3).toInt() : 1100;
    const int height = args.size() > 4 ? args.at(4).toInt() : 780;

    MainWindow window;
    window.resize(width, height);
    window.show();
    if (!doc.isEmpty())
        window.openDocument(doc);

    // Даём окну пройти раскладку и первую отрисовку, затем снимаем.
    QTimer::singleShot(300, &app, [&]() {
        const QPixmap shot = window.grab();
        shot.save(out);
        app.quit();
    });

    return app.exec();
}
