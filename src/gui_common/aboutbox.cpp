// Доводка окна «О программе».
//
// Разметка окна пришла из ресурсов Windows: там есть подпись с адресом сайта,
// строка версии и отдельная метка под ссылку на руководство пользователя
// (IDC_TMC_DOCLINK) — в Windows-версии её делает кликабельной код
// TmcDocLink.h. Здесь то же самое средствами Qt, плюс приписка, для какой
// системы собрана программа.
//
// Руководство ищется рядом с самой программой — так же, как на Windows оно
// ищется относительно .exe. Не нашли — открываем руководство на сайте.

#include "aboutbox.h"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QMessageBox>
#include <QPixmap>
#include <QList>
#include <QUrl>
#include <QWidget>

#include "mfc_dialog.h"

namespace {

const char kSiteUrl[]   = "https://www.tamic.ru";
const char kOnlineDoc[] = "https://www.tamic.ru/docs/user-manual/index.html";

// Имя файла руководства, который кладётся рядом с программами в поставке.
const char kManualFile[] = "TMC_Suite_User_Manual.pdf";

// Версия пакета — та же, что показывает окно «О программе» в Windows-версии
// (строка «Version 4.1» в ресурсе IDD_ABOUTBOX).
const char kPackageVersion[] = "4.1";

// Для какой системы собрана программа — это же пишет и окно Windows-версии,
// только там система одна.
QString systemName()
{
#if defined(__APPLE__)
    return QStringLiteral("macOS");
#elif defined(__linux__)
    return QStringLiteral("Linux");
#else
    return QStringLiteral("Windows");
#endif
}

QString architectureName()
{
#if defined(__aarch64__) || defined(_M_ARM64)
    return QStringLiteral("ARM64");
#elif defined(__x86_64__) || defined(_M_X64)
    return QStringLiteral("x64");
#else
    return QStringLiteral("x86");
#endif
}

// Где искать руководство: рядом с программой и на уровень-два выше — так же,
// как TmcDocLink.h поднимается по дереву от .exe. Для пакета .deb это
// /usr/share/tmc-suite, для macOS — Resources внутри .app.
QString findManual()
{
    const QString exeDir = QCoreApplication::applicationDirPath();
    QStringList places;
    places << exeDir
           << exeDir + QStringLiteral("/../share/tmc-suite")
           << exeDir + QStringLiteral("/../share/doc/tmc-suite")
           << exeDir + QStringLiteral("/../Resources")
           << exeDir + QStringLiteral("/../../Resources")
           << exeDir + QStringLiteral("/../doc")
           << exeDir + QStringLiteral("/..");

    for (const QString &dir : places) {
        const QFileInfo fi(QDir(dir).filePath(QString::fromLatin1(kManualFile)));
        if (fi.isFile())
            return fi.absoluteFilePath();
    }
    return QString();
}

// Метка становится ссылкой: текст в HTML, переходы разрешены. Цвет берём тот
// же, которым выделены ссылки в оформлении пакета: синий по умолчанию на
// тёмном фоне почти не читается.
void makeLink(QLabel *label, const QString &url, const QString &text)
{
    label->setTextFormat(Qt::RichText);
    label->setText(QStringLiteral(
                       "<a href=\"%1\" style=\"color:#34e3c4;\">%2</a>")
                       .arg(url, text));
    label->setOpenExternalLinks(true);
    label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    label->setCursor(Qt::PointingHandCursor);
}

} // namespace

namespace tmcabout {

QString aboutText(const QString &title, const QString &description)
{
    const QString manual = findManual();
    const QString docUrl = manual.isEmpty()
                               ? QString::fromLatin1(kOnlineDoc)
                               : QUrl::fromLocalFile(manual).toString();

    return QStringLiteral(
               "<b>%1</b><br>%2<br><br>"
               "Версия %3 · %4 (%5)<br><br>"
               "<a href=\"%6\" style=\"color:#34e3c4;\">Руководство пользователя</a>"
               "&nbsp;&nbsp;·&nbsp;&nbsp;"
               "<a href=\"%7\" style=\"color:#34e3c4;\">www.tamic.ru</a>")
        .arg(title, description, QString::fromLatin1(kPackageVersion),
             systemName(), architectureName(), docUrl,
             QString::fromLatin1(kSiteUrl));
}

void show(QWidget *parent, const QString &windowTitle, const QString &title,
          const QString &description)
{
    // Своё окно вместо QMessageBox::about: у стандартного ссылки только
    // выделяются мышью, а открывать их некому. Здесь метка открывает их сама.
    QMessageBox box(parent);
    box.setWindowTitle(windowTitle);
    box.setIconPixmap(parent && !parent->windowIcon().isNull()
                          ? parent->windowIcon().pixmap(64, 64)
                          : QPixmap());
    box.setTextFormat(Qt::RichText);
    box.setText(aboutText(title, description));
    box.setStandardButtons(QMessageBox::Ok);

    for (QLabel *label : box.findChildren<QLabel *>()) {
        label->setOpenExternalLinks(true);
        label->setTextInteractionFlags(Qt::TextBrowserInteraction);
    }
    box.exec();
}

void decorate(QWidget *dialog)
{
    if (!dialog || dialog->objectName() != QLatin1String("IDD_ABOUTBOX"))
        return;

    const QList<QLabel *> labels = dialog->findChildren<QLabel *>();
    for (QLabel *label : labels) {
        const QString text = label->text().trimmed();

        // Адрес сайта — рабочая ссылка.
        if (text.contains(QLatin1String("tamic.ru"), Qt::CaseInsensitive)
            && !text.startsWith(QLatin1Char('<'))) {
            makeLink(label, QString::fromLatin1(kSiteUrl), text);
            continue;
        }

        // Строка версии — дописываем систему и разрядность.
        if (text.startsWith(QLatin1String("Version"), Qt::CaseInsensitive)
            && !text.contains(systemName())) {
            label->setText(QStringLiteral("%1 · %2 (%3)")
                               .arg(text, systemName(), architectureName()));
            continue;
        }

        // Пустая метка под ссылку на руководство — заполняем.
        if (label->objectName() == QLatin1String("IDC_TMC_DOCLINK")) {
            const QString manual = findManual();
            const QString url = manual.isEmpty()
                                    ? QString::fromLatin1(kOnlineDoc)
                                    : QUrl::fromLocalFile(manual).toString();
            makeLink(label, url, QStringLiteral("Руководство пользователя"));
            label->setToolTip(manual.isEmpty()
                                  ? QStringLiteral("Открыть руководство на сайте")
                                  : manual);
        }
    }
}

namespace {
// Слой совместимости не знает о типах Qt, поэтому принимает указатель без типа.
void decorateRaw(void *qDialog)
{
    decorate(static_cast<QWidget *>(qDialog));
}
} // namespace

void install()
{
    tmc_set_dialog_decorator(&decorateRaw);
}

} // namespace tmcabout
