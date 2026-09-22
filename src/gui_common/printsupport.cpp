#include "printsupport.h"

#include <QImage>
#include <QOpenGLWidget>
#include <QPageSetupDialog>
#include <QPainter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QPrinter>

#include "mfc_view.h"

namespace {

// Настройки принтера живут между вызовами, как в Windows-версии, где параметры
// страницы запоминались на время работы программы.
QPrinter &sharedPrinter()
{
    static QPrinter printer(QPrinter::HighResolution);
    return printer;
}

// Рисование одной страницы: контекст подключается к принтеру, и вид рисует
// себя тем же кодом, что и на экране. Масштаб получается сам: режим отображения
// MM_HIMETRIC считает пиксели из разрешения устройства, а у принтера оно своё.
void drawPage(QPrinter *printer, CScrollView *view)
{
    if (!view)
        return;
    QPainter painter(printer);

    // Вид, рисующий через OpenGL (FieldView), контекстом устройства не
    // пользуется: его OnDraw зовёт DrawScene, а та рисует прямо в кадровый
    // буфер окна и делает SwapBuffers. Контекст принтера при этом остаётся
    // нетронутым — лист выходил пустым, и печать, и сохранение в PDF давали
    // белую страницу. Поэтому для таких видов переносим на бумагу готовое
    // изображение из кадрового буфера.
    if (QOpenGLWidget *gl = dynamic_cast<QOpenGLWidget *>(view->Host())) {
        const QImage shot = gl->grabFramebuffer();
        if (!shot.isNull()) {
            const QRect page =
                printer->pageLayout().paintRectPixels(printer->resolution());
            QSize size = shot.size();
            size.scale(page.size(), Qt::KeepAspectRatio);
            const QPoint at(page.left() + (page.width() - size.width()) / 2,
                            page.top() + (page.height() - size.height()) / 2);
            painter.drawImage(QRect(at, size), shot);
            return;
        }
    }

    CDC dc;
    dc.Attach(&painter, printer);
    dc.SetPrinting(TRUE);

    // На бумаге прокрутки нет: начало координат — левый верхний угол листа.
    dc.SetMapMode(MM_HIMETRIC);
    dc.SetViewportOrg(0, 0);
    view->OnDraw(&dc);
}

} // namespace

namespace tmcprint {

bool printView(QWidget *parent, CScrollView *view, const QString &documentName)
{
    if (!view)
        return false;

    QPrinter &printer = sharedPrinter();
    printer.setDocName(documentName);

    QPrintDialog dialog(&printer, parent);
    dialog.setWindowTitle(QStringLiteral("Print"));
    if (dialog.exec() != QDialog::Accepted)
        return false;

    drawPage(&printer, view);
    return true;
}

bool previewView(QWidget *parent, CScrollView *view, const QString &documentName)
{
    if (!view)
        return false;

    QPrinter &printer = sharedPrinter();
    printer.setDocName(documentName);

    QPrintPreviewDialog dialog(&printer, parent);
    dialog.setWindowTitle(QStringLiteral("Print Preview"));
    QObject::connect(&dialog, &QPrintPreviewDialog::paintRequested,
                     [view](QPrinter *p) { drawPage(p, view); });
    dialog.exec();
    return true;
}

bool printViewToPdf(CScrollView *view, const QString &path)
{
    if (!view || path.isEmpty())
        return false;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(path);
    drawPage(&printer, view);
    return true;
}

bool pageSetup(QWidget *parent)
{
    QPageSetupDialog dialog(&sharedPrinter(), parent);
    dialog.setWindowTitle(QStringLiteral("Print Setup"));
    return dialog.exec() == QDialog::Accepted;
}

} // namespace tmcprint
