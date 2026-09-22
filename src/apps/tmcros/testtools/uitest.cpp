// Проверка работы окна без участия человека.
//
// Повторяет то, что делает пользователь: открывает задание, нажимает все кнопки
// панели, меняет размер окна, делает двойной щелчок по графику. Если что-то
// зависает, сторож по времени печатает стек — сразу видно, где именно.
//
// В поставку не входит: средство разработки.
//
//   tmcros_uitest <файл задания> [секунд на всё]

#include <QAction>
#include <QApplication>
#include <QDialog>
#include <QMenu>
#include <QMouseEvent>
#include <QTimer>
#include <QWidget>

#include <csignal>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

#include <execinfo.h>

#include "mainwindow.h"
#include "theme.h"

namespace {

const char* g_stage = "запуск";

// Сторож: если шаг не закончился за отведённое время, печатаем, на чём стоим,
// и стек — программа считается зависшей.
void watchdog(int)
{
    const char* prefix = "\nЗАВИСАНИЕ на шаге: ";
    ssize_t ignored = ::write(2, prefix, std::strlen(prefix));
    ignored = ::write(2, g_stage, std::strlen(g_stage));
    ignored = ::write(2, "\n", 1);
    (void)ignored;

    void* frames[64];
    const int n = ::backtrace(frames, 64);
    ::backtrace_symbols_fd(frames, n, 2);
    ::_exit(2);
}

void stage(const char* name, int seconds)
{
    g_stage = name;
    ::alarm(unsigned(seconds));
    std::printf("  %s\n", name);
    std::fflush(stdout);
}

} // namespace

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    CWinApp::SetAppName("TMCROS");
    theme::apply();

    const QStringList args = QApplication::arguments();
    const QString doc = args.size() > 1 ? args.at(1) : QString();
    const int limit = args.size() > 2 ? args.at(2).toInt() : 20;

    std::signal(SIGALRM, watchdog);

    // Команды открывают модальные окна и ждут ответа. Чтобы проверка шла сама,
    // каждое появившееся окно закрывается через полсекунды — как если бы
    // пользователь нажал «Отмена».
    QTimer closer;
    closer.setInterval(500);
    QObject::connect(&closer, &QTimer::timeout, []() {
        if (QWidget* modal = QApplication::activeModalWidget()) {
            if (QDialog* dialog = qobject_cast<QDialog*>(modal))
                dialog->reject();
            else
                modal->close();
        }
    });
    closer.start();

    MainWindow window;
    window.resize(1100, 780);
    window.show();
    QApplication::processEvents();

    stage("открытие задания", limit);
    if (!doc.isEmpty())
        window.openDocument(doc);
    QApplication::processEvents();

    // Все команды панели и меню — по очереди, как если бы их нажимали.
    const QList<QAction*> actions = window.findChildren<QAction*>();
    std::printf("  команд в окне: %d\n", int(actions.size()));
    for (QAction* action : actions) {
        if (!action->isEnabled() || action->menu())
            continue;
        stage(qPrintable(QStringLiteral("команда: %1")
                             .arg(action->text().remove(QLatin1Char('&')))),
              limit);
        action->trigger();
        QApplication::processEvents();
    }

    stage("изменение размера окна", limit);
    window.resize(1400, 900);
    QApplication::processEvents();

    // Двойной щелчок по середине области графика.
    stage("двойной щелчок по графику", limit);
    if (QWidget* view = window.findChild<QWidget*>(QStringLiteral("qt_scrollarea_viewport"))) {
        const QPointF center(view->width() / 2.0, view->height() / 2.0);
        QMouseEvent press(QEvent::MouseButtonPress, center, view->mapToGlobal(center),
                          Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent dbl(QEvent::MouseButtonDblClick, center, view->mapToGlobal(center),
                        Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
        QMouseEvent release(QEvent::MouseButtonRelease, center, view->mapToGlobal(center),
                            Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
        QApplication::sendEvent(view, &press);
        QApplication::sendEvent(view, &release);
        QApplication::sendEvent(view, &dbl);
        QApplication::sendEvent(view, &release);
    } else {
        std::printf("  (область графика не найдена)\n");
    }
    QApplication::processEvents();


    // Повторяющиеся действия: пользователь несколько раз меняет размер окна и
    // щёлкает по графику. Именно на повторе всплывал захват мыши, из-за
    // которого панель переставала отвечать.
    if (QWidget* view = window.findChild<QWidget*>(QStringLiteral("qt_scrollarea_viewport"))) {
        for (int pass = 1; pass <= 5; ++pass) {
            stage(qPrintable(QStringLiteral("повтор %1: размер окна").arg(pass)), limit);
            window.resize(1100 + 60 * pass, 780 + 40 * pass);
            QApplication::processEvents();

            stage(qPrintable(QStringLiteral("повтор %1: щелчки по графику").arg(pass)), limit);
            const QPointF inside(view->width() * 0.4, view->height() * 0.4);
            const QPointF outside(view->width() * 0.92, view->height() * 0.92);
            for (const QPointF& p : { inside, outside }) {
                QMouseEvent press(QEvent::MouseButtonPress, p, view->mapToGlobal(p),
                                  Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QMouseEvent move(QEvent::MouseMove, p + QPointF(10, 10),
                                 view->mapToGlobal(p + QPointF(10, 10)),
                                 Qt::NoButton, Qt::LeftButton, Qt::NoModifier);
                QMouseEvent release(QEvent::MouseButtonRelease, p, view->mapToGlobal(p),
                                    Qt::LeftButton, Qt::NoButton, Qt::NoModifier);
                QMouseEvent dbl(QEvent::MouseButtonDblClick, p, view->mapToGlobal(p),
                                Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
                QApplication::sendEvent(view, &press);
                QApplication::sendEvent(view, &move);
                QApplication::sendEvent(view, &release);
                QApplication::sendEvent(view, &dbl);
                QApplication::sendEvent(view, &release);
                QApplication::processEvents();
            }

            // Мышь не должна остаться захваченной: иначе все щелчки уходят в
            // график, и программа выглядит зависшей.
            if (QWidget* grabber = QWidget::mouseGrabber()) {
                std::printf("  СБОЙ: мышь захвачена виджетом %s\n",
                            grabber->metaObject()->className());
                return 3;
            }

            // Панель обязана продолжать работать: проверяем, что команда идёт.
            stage(qPrintable(QStringLiteral("повтор %1: команда после щелчков").arg(pass)), limit);
            for (QAction* action : window.findChildren<QAction*>()) {
                if (action->data().toInt() == 32778) {   // ReSize viewport
                    action->trigger();
                    break;
                }
            }
            QApplication::processEvents();
        }
    }

    stage("завершение", limit);
    ::alarm(0);
    std::printf("проверка окна пройдена без зависаний\n");
    return 0;
}
