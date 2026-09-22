// Снимок окна FieldView без участия человека.
//
// Запускает то же самое главное окно, что и сама программа, рисует его в
// картинку и сохраняет. Пятым доводом можно назвать команду — тогда снимается
// окно, которое она открывает (диалог). Так проверяются и вид, и диалоги.
//
// Рисование идёт через OpenGL, а в режиме offscreen у Qt нет буфера кадра,
// поэтому снимать нужно на настоящем экране:
//
//   QT_QPA_PLATFORM=xcb LIBGL_ALWAYS_SOFTWARE=1 \
//       fieldview_shot <файл.png> [файл поля] [ширина] [высота] [команда]
//
// Команда: parameters | level | autocolor | about | printpdf | <число>.
//
// В поставку не входит: это инструмент разработки.

#include <QApplication>

#include <cstdio>
#include <QDialog>
#include <QPixmap>
#include <QTimer>
#include <QWidget>

#include "mainwindow.h"
#include "mfcglwidget.h"
#include "dialogshot.h"
#include "mfc_view.h"        // CScrollView целиком
#include "printsupport.h"
#include "theme.h"
#include "tmc_mfc_doc.h"

#include "resource.h"
#include "tmcasetcolor.h"

#include "fldviewview.h"     // вид целиком: в mainwindow.h он только объявлен,
                             // и без определения не видно, что он CScrollView


namespace {

/// Перевести имя команды в её числовой идентификатор из ресурсов.
/// 0 — команды нет, снимаем само окно.
UINT commandByName(const QString &name)
{
    if (name.isEmpty())                                 return 0;
    if (name == QLatin1String("parameters"))            return ID_VIEW_PARAMETERS;
    if (name == QLatin1String("level"))                 return ID_FIELD_L;
    if (name == QLatin1String("about"))                 return ID_APP_ABOUT;
    bool ok = false;
    const UINT id = name.toUInt(&ok);
    return ok ? id : 0;
}

/// Снять то окно, которое сейчас на виду: модальный диалог, если он открыт,
/// иначе главное окно.
void grabAndSave(QWidget *fallback, const QString &out)
{
    QWidget *target = QApplication::activeModalWidget();
    if (!target)
        target = QApplication::activeWindow();
    if (!target)
        target = fallback;
    target->grab().save(out);
}

} // namespace

int main(int argc, char *argv[])
{
    MfcGLWidget::setupSurfaceFormat();

    QApplication app(argc, argv);
    CWinApp::SetAppName("FldView");
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

    const QString out = args.size() > 1 ? args.at(1) : QStringLiteral("fieldview.png");
    const QString doc = args.size() > 2 ? args.at(2) : QString();
    const int width   = args.size() > 3 ? args.at(3).toInt() : 1130;
    const int height  = args.size() > 4 ? args.at(4).toInt() : 700;
    const QString cmd = args.size() > 5 ? args.at(5) : QString();

    MainWindow window;
    window.resize(width, height);
    window.show();
    if (!doc.isEmpty())
        window.openDocument(doc);

    // Печать в файл PDF: проверяем, что на листе оказывается картинка сцены,
    // а не пустая страница (окно FieldView рисуется через OpenGL, и раньше
    // печать давала белый лист).
    if (cmd == QLatin1String("printpdf")) {
        QTimer::singleShot(1500, &app, [&]() {
            CFldViewView *view = window.currentView();
            if (!view) {
                std::fprintf(stderr, "нет открытого документа\n");
                app.exit(3);
                return;
            }
            const bool ok = tmcprint::printViewToPdf(view, out);
            std::printf("печать в %s: %s\n", out.toLocal8Bit().constData(),
                        ok ? "выполнена" : "не удалась");
            app.quit();
        });
        return app.exec();
    }

    // Окно «Auto color» из меню не открывается — его показывает вкладка Field
    // кнопкой «Set color». Для проверки создаём его напрямую.
    const bool autoColor = (cmd == QLatin1String("autocolor"));
    const UINT commandId = autoColor ? 0 : commandByName(cmd);

    // Даём окну пройти раскладку, создать контекст OpenGL и нарисовать первый
    // кадр, затем открываем диалог (если он заказан).
    QTimer::singleShot(1200, &app, [&]() {
        if (autoColor) {
            COLORREF colors[20];
            for (int i = 0; i < 20; ++i)
                colors[i] = RGB(i * 12, 0, 255 - i * 12);
            CTmcASetColor dlg;
            dlg.DoModal(colors);      // ждёт здесь, пока окно не закроют
        } else if (commandId) {
            window.runCommand(commandId);   // ждёт здесь, если диалог модальный
        }
    });

    // Снимок делается позже открытия диалога: к этому моменту он уже на экране,
    // а вложенный цикл событий продолжает работать.
    QTimer::singleShot(2400, &app, [&]() {
        grabAndSave(&window, out);
        if (QWidget *modal = QApplication::activeModalWidget())
            modal->close();
    });

    QTimer::singleShot(3000, &app, [&]() { app.quit(); });

    return app.exec();
}
