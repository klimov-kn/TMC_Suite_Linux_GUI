// Точка входа оболочек счётных ядер PlanarRT_H и PlanarRT_X.
//
// Программа собрана из двух слоёв: расчёт, работа с файлами задания и счёт в
// отдельном потоке взяты из Windows-версии без изменений, интерфейс — Qt 6.
//
// Файл общий для обеих программ: чем они отличаются, собрано в planrt_app.h.

#include <QApplication>

#include "mainwindow.h"
#include "planrt_app.h"

// Заголовки общего слоя интерфейса — при снятой подмене CScrollView
// (см. planrt_prelude.h); дальше подмена нужна снова, её ждут заголовки
// Windows-версии.
#undef CScrollView
#include "theme.h"
#define CScrollView TmcPlanRtScrollView

#include "tmc_mfc_doc.h"
#include "resource.h"
#include "pl_iofor.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QString::fromLatin1(TmcPlanRtApp().appName));
    QApplication::setOrganizationName(QStringLiteral("TMC Suite"));

    // Настройки программы (внешние просмотрщики, форматы вывода, мелодии) на
    // Windows лежали в реестре; здесь — в ~/.config/TMC_Suite/<программа>.conf.
    CWinApp::SetAppName(TmcPlanRtApp().appName);

    theme::apply();

    // Ключи режима разбираются так же, как в Windows-версии: тем же кодом и по
    // всей командной строке целиком (в MFC это было theApp.m_lpCmdLine).
    QStringList tail = QApplication::arguments();
    if (!tail.isEmpty())
        tail.removeFirst();
    QByteArray cmdLine = tail.join(QLatin1Char(' ')).toLocal8Bit();
    cmdLine.append('\0');
    Set_CommandLine_Flags(cmdLine.data());

    MainWindow window;
    window.show();

    // Файл задания — первый довод, не являющийся ключом. Ключом считается
    // только довод, начинающийся с дефиса: в Windows-версии ключи писались
    // ещё и через косую черту (/Ar), но на Linux с косой черты начинается
    // любой полный путь, и файл задания принимался бы за ключ.
    for (const QString &arg : tail) {
        if (arg.startsWith(QLatin1Char('-')))
            continue;
        window.openDocument(arg);
        break;
    }

    return app.exec();
}
