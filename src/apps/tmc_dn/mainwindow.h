// mainwindow.h — главное окно TMC_DN (вьювер диаграмм направленности).
//
// Повторяет окно Windows-версии: те же пункты меню в том же порядке (ресурс
// IDR_TMCGROTYPE), та же панель с теми же пиктограммами (IDR_MAINFRAME TOOLBAR),
// та же строка состояния, многодокументный режим (MDI). Команды не расписаны
// вручную по одной: каждая связана с числовым идентификатором из ресурсов, а
// выполняет её карта сообщений вида — тот же код, что и на Windows.
//
// Классы вида и документа унаследованы от TMCGROUT и потому названы
// CTMCGROUTView / CTMCGROUTDoc, но рисуют они диаграмму направленности.

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

    /// Открыть файл задания (документ TMC_DN).
    void openDocument(const QString &path);

    /// Текст в строку состояния от кода Windows-версии (PutTrace, PutStatistics).
    void putStatusText(int pane, const QString &text);

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

    /// Выполнить команду: сначала предлагаем виду активного окна, затем себе.
    void dispatch(UINT commandId);

    /// Опросить обработчики обновления и перенести их решение
    /// на пункты меню и кнопки (как каркас MFC перед показом меню).
    void updateCommandStates();

    QAction *makeAction(UINT id, const QString &text, const QString &shortcut = QString());
    TmcDocumentWindow *activeDocument() const;

    QMdiArea            *m_mdi = nullptr;
    QMap<UINT, QAction*> m_actions;
    QList<TmcDocumentWindow*> m_documents;
    QLabel              *m_statusMain = nullptr;
    QLabel              *m_statusExtra = nullptr;
    CMainFrame          *m_frame = nullptr;   // строка состояния для кода из win_src
    class MfcQtHost     *m_statusHost = nullptr;  // хозяин этой строки состояния
    QMenu               *m_windowMenu = nullptr;
};
