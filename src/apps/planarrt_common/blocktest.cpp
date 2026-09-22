// Проверка сценария «Statistics -> View block list -> Next» без участия человека.
//
// Повторяет то, что делает человек мышью: открывает задание, вызывает окно
// Statistics, нажимает в нём «View block list», а затем несколько раз «Next».
// На macOS этот сценарий валил программу на втором блоке; средство нужно, чтобы
// проверить то же самое на Linux и убедиться, что правка помогла.
//
// В поставку не входит: это инструмент разработки.
//
//   planarrt_h_blocktest <файл задания.tpl> [сколько раз нажать Next]

#include <QApplication>
#include <QAction>
#include <QDialog>
#include <QPushButton>
#include <QTimer>
#include <QVariant>
#include <QWidget>

#include <cstdio>

#include "mainwindow.h"
#include "planrt_app.h"

#undef CScrollView
#include "theme.h"
#define CScrollView TmcPlanRtScrollView

#include "tmc_mfc_doc.h"

namespace {

QDialog *activeDialog()
{
    // Сначала спрашиваем систему окон, но в режиме без экрана «активного»
    // окна может не быть — тогда берём любой показанный диалог.
    if (QDialog *d = qobject_cast<QDialog *>(QApplication::activeModalWidget()))
        return d;
    const QWidgetList tops = QApplication::topLevelWidgets();
    for (QWidget *w : tops) {
        if (QDialog *d = qobject_cast<QDialog *>(w)) {
            if (d->isVisible())
                return d;
        }
    }
    return nullptr;
}

QPushButton *button(QDialog *dlg, const char *name)
{
    return dlg ? dlg->findChild<QPushButton *>(QString::fromLatin1(name)) : nullptr;
}

// Нажатие откладываем: обработчик кнопки открывает следующее модальное окно и
// не возвращает управление, а рабочий таймер должен продолжать работать.
void clickLater(QPushButton *b)
{
    QTimer::singleShot(0, b, [b]() { b->click(); });
}

// Номер блока показан в поле IDC_TMCBLOCKNUMBER; тип виджета зависит от
// разметки, поэтому читаем свойство «text» у любого.
QString blockNumber(QDialog *dlg)
{
    QWidget *w = dlg->findChild<QWidget *>(QStringLiteral("IDC_TMCBLOCKNUMBER"));
    return w ? w->property("text").toString() : QStringLiteral("?");
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    CWinApp::SetAppName(TmcPlanRtApp().appName);
    theme::apply();

    const QStringList args = QApplication::arguments();
    if (args.size() < 2) {
        std::fprintf(stderr, "нужен путь к заданию .tpl\n");
        return 2;
    }
    const QString doc = args.at(1);
    const int wanted = args.size() > 2 ? args.at(2).toInt() : 8;

    MainWindow window;
    window.resize(1130, 700);
    window.show();
    window.openDocument(doc);

    // Диагностика: видит ли окно открытый документ (без документа пункты меню
    // недоступны и нажимать нечего).
    {
        const QList<QAction *> actions = window.findChildren<QAction *>();
        int total = 0, enabled = 0;
        for (QAction *a : actions) {
            ++total;
            if (a->isEnabled())
                ++enabled;
            if (a->text().contains(QStringLiteral("atistics")))
                std::printf("пункт Statistics: доступен=%d\n", int(a->isEnabled()));
        }
        std::printf("действий всего %d, доступно %d\n", total, enabled);
        std::fflush(stdout);
    }

    int clicks = 0;
    bool statisticsAsked = false;
    bool blockListSeen = false;

    QTimer timer;
    timer.setInterval(200);
    QObject::connect(&timer, &QTimer::timeout, &app, [&]() {
        QDialog *dlg = activeDialog();

        if (!dlg) {
            if (statisticsAsked && blockListSeen) {
                std::printf("сценарий пройден: нажатий Next — %d, падения нет\n", clicks);
                std::fflush(stdout);
                app.quit();
            }
            return;
        }

        // Какое окно перед нами — печатаем один раз на каждое новое.
        static QDialog *seen = nullptr;
        if (dlg != seen) {
            seen = dlg;
            std::printf("окно «%s» (имя «%s»), кнопки:",
                        dlg->windowTitle().toLocal8Bit().constData(),
                        dlg->objectName().toLocal8Bit().constData());
            const QList<QPushButton *> btns = dlg->findChildren<QPushButton *>();
            for (QPushButton *b : btns)
                std::printf(" %s", b->objectName().toLocal8Bit().constData());
            std::printf("\n");
            std::fflush(stdout);
        }

        // Окно списка блоков узнаём по кнопке «Close» — её нет в Statistics.
        if (QPushButton *close = button(dlg, "ID_TMCBLOCKBUTTONNEXT2")) {
            blockListSeen = true;
            std::printf("  показан блок: %s\n", blockNumber(dlg).toLocal8Bit().constData());
            std::fflush(stdout);
            if (clicks < wanted) {
                ++clicks;
                // «Next» в этом окне — ID_TMCBLOCKBUTTONNEXT (номер 6), а под
                // этим номером в разметке лежит имя IDC_TMCSTATISTICSPREVSTEP.
                if (QPushButton *next = button(dlg, "IDC_TMCSTATISTICSPREVSTEP")) {
                    std::printf("  жму Next (%d)\n", clicks);
                    std::fflush(stdout);
                    clickLater(next);
                } else {
                    std::fprintf(stderr, "кнопка Next не найдена\n");
                    clickLater(close);
                }
            } else {
                clickLater(close);
            }
            return;
        }

        if (QPushButton *view = button(dlg, "IDC_TMCSTATISTICSVIEWBLOCKLIST2")) {
            std::printf("окно Statistics открыто, жму «View block list»\n");
            std::fflush(stdout);
            clickLater(view);
            return;
        }
    });
    timer.start();

    // Statistics вызываем отдельным срабатыванием: этот вызов не возвращается,
    // пока окно открыто (DoModal), и держать его внутри рабочего таймера нельзя
    // — тот перестал бы обрабатывать сами окна.
    QTimer::singleShot(800, &app, [&]() {
        const QList<QAction *> actions = window.findChildren<QAction *>();
        for (QAction *a : actions) {
            if (a->text().contains(QStringLiteral("atistics"))) {
                a->setEnabled(true);   // недоступный пункт не срабатывает
                statisticsAsked = true;
                std::printf("вызываю Statistics\n");
                std::fflush(stdout);
                a->trigger();
                std::printf("Statistics: окно закрыто\n");
                std::fflush(stdout);
                return;
            }
        }
        std::fprintf(stderr, "пункт Statistics не найден\n");
        app.exit(3);
    });

    // Страховка: сценарий не должен идти дольше минуты.
    QTimer::singleShot(60000, &app, [&]() {
        std::fprintf(stderr, "время вышло: нажатий Next — %d\n", clicks);
        app.exit(4);
    });

    return app.exec();
}
