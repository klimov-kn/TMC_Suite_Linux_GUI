// mainwindow.h — главное окно оболочек счётных ядер PlanarRT_H и PlanarRT_X.
//
// Повторяет окно Windows-версии: те же пункты меню в том же порядке (ресурс
// IDR_PLANRTTYPE), та же панель с теми же пиктограммами (IDR_MAINFRAME
// TOOLBAR), строка состояния из трёх панелей, многодокументный режим (MDI).
//
// Команды не расписаны вручную по одной: каждая связана с числовым
// идентификатором из ресурсов, а выполняет её карта сообщений вида — тот же
// код, что и на Windows (CPlanRT_HView).
//
// Счёт идёт в отдельном потоке, как на Windows (AfxBeginThread поверх QThread).
// Ход счёта приходит в строку состояния из этого потока, поэтому текст
// доставляется через очередь событий Qt, а не прямым вызовом виджета.

#pragma once

#include <QMainWindow>
#include <QMap>

QT_BEGIN_NAMESPACE
class QAction;
class QLabel;
class QMdiArea;
class QMdiSubWindow;
class QTimer;
QT_END_NAMESPACE

class MfcViewWidget;
class CPlanRT_HDoc;
class CPlanRT_HView;
class CMainFrame;

/// Одно окно документа: документ, вид и виджет-хозяин живут вместе.
struct TmcPlanRtWindow
{
    CPlanRT_HDoc  *doc = nullptr;
    CPlanRT_HView *view = nullptr;
    MfcViewWidget *widget = nullptr;
    QMdiSubWindow *sub = nullptr;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    /// Открыть файл задания (.tpl).
    void openDocument(const QString &path);

public slots:
    /// Текст в панель строки состояния. Вызывается в том числе из расчётного
    /// потока — только через очередь событий (Qt::QueuedConnection).
    void setStatusPane(int pane, const QString &text);

    /// Закрыть программу по просьбе расчётного ядра (пакетный режим: досчитав
    /// последний шаг, программа закрывается сама — как PostMessage(WM_CLOSE)
    /// в Windows-версии).
    void closeFromKernel();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onNewDocument();
    void onOpenDocument();
    void onCloseDocument();
    void onAbout();
    void onSubWindowActivated(QMdiSubWindow *sub);

private:
    void createMenus();
    void createToolBar();
    void createStatusBar();

    /// Доступность и «галочки» пунктов меню. Повторяет обработчики
    /// ON_UPDATE_COMMAND_UI Windows-версии.
    void updateCommandStates();

    /// Выполнить команду: сначала предлагаем виду активного окна, затем себе.
    void dispatch(UINT commandId);

    QAction *makeAction(UINT id, const QString &text, const QString &shortcut = QString(),
                        bool checkable = false);
    TmcPlanRtWindow *activeDocument() const;

    /// Остановить счёт и дождаться потока перед закрытием (как OnClose рамки
    /// Windows-версии). Возвращает false, если пользователь передумал.
    bool stopAllAndConfirm();

    QMdiArea                 *m_mdi = nullptr;
    QMap<UINT, QAction *>     m_actions;
    QList<TmcPlanRtWindow *>  m_documents;
    QLabel                   *m_statusTrace = nullptr;   // панель 0: ход счёта
    QLabel                   *m_statusStat = nullptr;    // панель 1: статистика
    QLabel                   *m_statusFlags = nullptr;   // панель 2: что выводится
    CMainFrame               *m_frame = nullptr;         // рамка для кода из win_src
    QTimer                   *m_uiTimer = nullptr;       // обновление меню, как в MFC
};
