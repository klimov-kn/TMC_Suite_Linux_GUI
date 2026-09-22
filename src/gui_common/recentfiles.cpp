#include "recentfiles.h"

#include <QAction>
#include <QFileInfo>
#include <QMenu>

#include "tmc_mfc_doc.h"

namespace {

// Раздел и ключи в настройках — как у MFC: «Recent File List» и «File1»…«FileN».
const char *kSection = "Recent File List";

QString keyFor(int index)
{
    return QStringLiteral("File%1").arg(index + 1);
}

} // namespace

TmcRecentFiles::TmcRecentFiles(QObject *parent)
    : QObject(parent)
{
}

void TmcRecentFiles::setMaxCount(int count)
{
    m_maxCount = count > 0 ? count : 1;
    while (m_files.size() > m_maxCount)
        m_files.removeLast();
    rebuildMenu();
}

void TmcRecentFiles::load()
{
    m_files.clear();
    for (int i = 0; i < m_maxCount; ++i) {
        const CString value = AfxGetApp()->GetProfileString(
            kSection, keyFor(i).toLocal8Bit().constData(), "");
        const QString path = QString::fromLocal8Bit(value.GetString());
        if (!path.isEmpty())
            m_files.append(path);
    }
    rebuildMenu();
}

void TmcRecentFiles::save()
{
    for (int i = 0; i < m_maxCount; ++i) {
        const QString path = i < m_files.size() ? m_files.at(i) : QString();
        AfxGetApp()->WriteProfileString(kSection,
                                        keyFor(i).toLocal8Bit().constData(),
                                        path.toLocal8Bit().constData());
    }
}

void TmcRecentFiles::add(const QString &path)
{
    if (path.isEmpty())
        return;
    const QString full = QFileInfo(path).absoluteFilePath();
    m_files.removeAll(full);
    m_files.prepend(full);
    while (m_files.size() > m_maxCount)
        m_files.removeLast();
    save();
    rebuildMenu();
}

void TmcRecentFiles::attachToMenu(QMenu *menu, QAction *before)
{
    m_menu = menu;
    m_before = before;
    rebuildMenu();
}

void TmcRecentFiles::rebuildMenu()
{
    if (!m_menu)
        return;

    for (QAction *a : m_actions) {
        m_menu->removeAction(a);
        delete a;
    }
    m_actions.clear();

    if (m_files.isEmpty()) {
        // В Windows-версии при пустом списке пункт «Recent File» показан, но
        // недоступен — повторяем, чтобы меню не «прыгало».
        QAction *empty = new QAction(QStringLiteral("Recent File"), m_menu);
        empty->setEnabled(false);
        m_menu->insertAction(m_before, empty);
        m_actions.append(empty);
        return;
    }

    int index = 1;
    for (const QString &path : m_files) {
        // Подпись как в MFC: номер и имя файла.
        QAction *a = new QAction(QStringLiteral("&%1 %2")
                                     .arg(index++)
                                     .arg(QFileInfo(path).fileName()),
                                 m_menu);
        a->setToolTip(path);
        connect(a, &QAction::triggered, this, [this, path]() { emit fileRequested(path); });
        m_menu->insertAction(m_before, a);
        m_actions.append(a);
    }
}
