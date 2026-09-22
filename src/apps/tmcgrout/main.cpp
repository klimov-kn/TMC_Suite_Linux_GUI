// Точка входа TMCGROUT — вьювера выходных характеристик S-матриц пакета
// TMC Suite (графики модуля, фазы и т. п. по частоте).
//
// Программа собрана из двух слоёв: расчёт и работа с файлами взяты из
// Windows-версии без изменений, интерфейс — Qt 6.

#include <QApplication>

#include "mainwindow.h"
#include "theme.h"
#include "tmc_mfc_doc.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("TMCGROUT"));
    QApplication::setOrganizationName(QStringLiteral("TMC Suite"));

    // Настройки программы (цвета графиков, формат осей) на Windows лежали в
    // реестре; здесь — в ~/.config/TMC_Suite/TMCGROUT.conf.
    CWinApp::SetAppName("TMCGROUT");

    theme::apply();

    MainWindow window;
    window.show();

    // Файл задания можно передать в командной строке — как в Windows-версии.
    const QStringList args = QApplication::arguments();
    if (args.size() > 1)
        window.openDocument(args.at(1));

    return app.exec();
}
