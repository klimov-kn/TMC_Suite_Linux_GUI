// Точка входа FieldView — визуализатора полей пакета TMC Suite.
//
// Расчёт и работа с файлами взяты из Windows-версии без изменений, интерфейс —
// Qt 6. Кадр рисуется средствами OpenGL внутри QOpenGLWidget.

#include <QApplication>

#include "mainwindow.h"
#include "mfcglwidget.h"
#include "theme.h"
#include "tmc_mfc_doc.h"

int main(int argc, char *argv[])
{
    // Совместимый профиль OpenGL нужно запросить ДО создания виджетов: вид
    // рисует в старом стиле (glBegin/glEnd, матрицы), и в профиле «ядро»
    // такие вызовы не работают.
    MfcGLWidget::setupSurfaceFormat();

    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("FieldView"));
    QApplication::setOrganizationName(QStringLiteral("TMC Suite"));

    // Настройки программы на Windows лежали в реестре; здесь —
    // в ~/.config/TMC_Suite/FldView.conf. Имя то же, что у Windows-версии,
    // чтобы совпадали названия разделов настроек.
    CWinApp::SetAppName("FldView");

    theme::apply();

    MainWindow window;
    window.show();

    // Файл поля можно передать в командной строке — как в Windows-версии.
    const QStringList args = QApplication::arguments();
    if (args.size() > 1)
        window.openDocument(args.at(1));

    return app.exec();
}
