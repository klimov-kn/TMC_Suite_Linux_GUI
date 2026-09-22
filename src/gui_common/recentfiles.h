// recentfiles.h — список недавно открытых файлов, как в меню File Windows-версии.
//
// В MFC каркас сам вёл этот список (пункт «Recent File») и хранил его в реестре.
// Здесь то же самое: список хранится в настройках программы
// (`~/.config/TMC_Suite/<программа>.conf`, тот же файл, что и остальные
// настройки), а пункты вставляются в меню File между разделителями — на том же
// месте, что и на Windows.

#pragma once

#include <QObject>
#include <QStringList>

QT_BEGIN_NAMESPACE
class QAction;
class QMenu;
QT_END_NAMESPACE

class TmcRecentFiles : public QObject
{
    Q_OBJECT

public:
    explicit TmcRecentFiles(QObject *parent = nullptr);

    /// Сколько файлов помнить. В MFC по умолчанию четыре.
    void setMaxCount(int count);

    /// Прочитать список из настроек программы и обновить меню.
    void load();

    /// Добавить файл наверх списка (и сохранить).
    void add(const QString &path);

    /// Встроить пункты в меню. Меню запоминается: при изменении списка пункты
    /// обновляются сами.
    void attachToMenu(QMenu *menu, QAction *before = nullptr);

    QStringList files() const { return m_files; }

signals:
    /// Пользователь выбрал файл из списка.
    void fileRequested(const QString &path);

private:
    void save();
    void rebuildMenu();

    QStringList     m_files;
    int             m_maxCount = 4;
    QMenu          *m_menu = nullptr;
    QAction        *m_before = nullptr;
    QList<QAction*> m_actions;
};
