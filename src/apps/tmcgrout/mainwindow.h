// mainwindow.h — главное окно TMCGROUT.
//
// Повторяет окно Windows-версии: те же пункты меню в том же порядке, та же
// панель с теми же пиктограммами, та же строка состояния, многодокументный
// режим (MDI). Команды не расписаны вручную по одной: каждая связана с числовым
// идентификатором из ресурсов, а выполняет её карта сообщений вида — тот же код,
// что и на Windows.

#pragma once

#include <QMainWindow>
#include <QMap>

#include "mfc_dialog.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMdiArea;
class QMdiSubWindow;
class QLabel;
QT_END_NAMESPACE

class MfcViewWidget;
class CTMCGROUTDoc;
class CTMCGROUTView;
class CMainFrame;

/// Одно окно документа: документ, вид и виджет-хозяин живут вместе.
struct TmcDocumentWindow
{
    CTMCGROUTDoc  *doc = nullptr;
    CTMCGROUTView *view = nullptr;
    MfcViewWidget *widget = nullptr;
    QMdiSubWindow *sub = nullptr;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    /// Открыть файл задания (документ TMCGROUT).
    void openDocument(const QString &path);

private slots:
    void onNewDocument();
    void onOpenDocument();
    void onCloseDocument();
    void onAbout();
    void onSubWindowActivated(QMdiSubWindow *sub);
    void onStatusText(int pane, const QString &text);

private:
    void createMenus();
    void createToolBar();
    void createStatusBar();
    void updateMenuForDocument();

    /// Общая часть открытия и создания: окно документа с видом.
    void createDocumentWindow(const QString &path, const QString &title);

    /// Выполнить команду: сначала предлагаем виду активного окна, затем себе.
    void dispatch(UINT commandId);

    /// Опросить обработчики обновления и перенести их решение
    /// на пункты меню и кнопки (как каркас MFC перед показом меню).
    void updateCommandStates();

    QAction *makeAction(UINT id, const QString &text, const QString &shortcut = QString());
    TmcDocumentWindow *activeDocument() const;

    QMdiArea            *m_mdi = nullptr;
    class TmcRecentFiles *m_recent = nullptr;   // список недавних файлов
    QMap<UINT, QAction*> m_actions;
    QList<TmcDocumentWindow*> m_documents;
    QLabel              *m_statusMain = nullptr;
    QLabel              *m_statusExtra = nullptr;
    CMainFrame          *m_frame = nullptr;   // строка состояния для кода из win_src
    QMenu               *m_windowMenu = nullptr;
};
