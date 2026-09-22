#include "aboutbox.h"
#include "mainwindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QThread>
#include <QTimer>
#include <QToolBar>

// Общий слой интерфейса объявляет MfcViewWidget::setView(CScrollView*). В этой
// программе CScrollView подменён прослойкой (см. planrt_prelude.h), поэтому
// заголовки общего слоя включаются при снятой подмене — иначе в объявлении
// оказался бы другой тип и программа не скомпоновалась бы.
#undef CScrollView
#include "commandupdate.h"
#include "mfcviewwidget.h"
#include "theme.h"
#include "mousewatchdog.h"
#include "windowplacement.h"
#define CScrollView TmcPlanRtScrollView

// Классы, перенесённые из Windows-версии: документ, вид и рамка окна.
#include "resource.h"
#include "mainfrm.h"
#include "planrt_hdoc.h"
#include "planrt_hview.h"
#include "pl_iofor.h"
#include "planrt_app.h"

namespace {

// Единственное главное окно программы. Нужно обработчикам, которые зовёт код
// Windows-версии из расчётного потока: строка состояния, окно с вопросом,
// просьба закрыть программу.
MainWindow *g_mainWindow = nullptr;

// --- Строка состояния для кода из win_src -------------------------------------
// PL_GLFUN.CPP пишет ход счёта через CStatusBar::SetPaneText, а тот — через
// «хозяина» окна. Хозяин здесь такой: он переправляет текст главному окну.
// Вызов может прийти из расчётного потока, поэтому только через очередь
// событий Qt — трогать виджеты из чужого потока нельзя.
class TmcStatusHost : public MfcQtHost
{
public:
    void hostSetStatusText(int pane, const char *text) override
    {
        if (!g_mainWindow)
            return;
        QMetaObject::invokeMethod(g_mainWindow, "setStatusPane", Qt::QueuedConnection,
                                  Q_ARG(int, pane),
                                  Q_ARG(QString, text ? QString::fromLocal8Bit(text) : QString()));
    }

    // Строке состояния остальное не нужно.
    void hostInvalidate(const RECT *, BOOL) override {}
    void hostUpdateWindow() override {}
    void hostGetClientRect(RECT *r) const override { if (r) { r->left = r->top = r->right = r->bottom = 0; } }
    void hostSetCapture() override {}
    void hostReleaseCapture() override {}
    UINT hostSetTimer(UINT, UINT) override { return 0; }
    BOOL hostKillTimer(UINT) override { return TRUE; }
    int  hostMessageBox(const char *, const char *, UINT) override { return IDOK; }
    void hostSetScrollSizes(const SIZE &, const SIZE &, const SIZE &) override {}
    CPoint hostScrollPosition() const override { return CPoint(0, 0); }
    void hostScrollToPosition(const POINT &) override {}
    int  hostLogicalDpiX() const override { return 96; }
    int  hostLogicalDpiY() const override { return 96; }
};

TmcStatusHost g_statusHost;

/// Окно сообщения для кода из win_src (AfxMessageBox). Из расчётного потока
/// спрашивать пользователя нельзя, поэтому оттуда сообщение уходит в поток
/// вывода, а ответ берётся такой же, как у пакетного режима.
int TmcQtMessageBox(const char *text, UINT type)
{
    const QString message = text ? QString::fromLocal8Bit(text) : QString();
    if (QThread::currentThread() != QApplication::instance()->thread()) {
        qWarning("%s", qPrintable(message));
        return (type & MB_YESNO) ? IDYES : IDOK;
    }

    const QString caption = QString::fromLatin1(TmcPlanRtApp().windowTitle);
    if (type & MB_YESNO) {
        const QMessageBox::StandardButton answer =
            QMessageBox::question(g_mainWindow, caption, message,
                                  QMessageBox::Yes | QMessageBox::No);
        return (answer == QMessageBox::Yes) ? IDYES : IDNO;
    }
    QMessageBox::information(g_mainWindow, caption, message);
    return IDOK;
}

/// Однострочное сообщение библиотек (AfxMessageBox с одним доводом).
void TmcQtMessage(const char *text)
{
    TmcQtMessageBox(text, MB_OK);
}

/// Просьба расчётного ядра закрыть программу (пакетный режим).
void TmcQtCloseFrame(UINT)
{
    if (g_mainWindow)
        QMetaObject::invokeMethod(g_mainWindow, "closeFromKernel", Qt::QueuedConnection);
}

// --- Меню ---------------------------------------------------------------------
// Подписи и горячие клавиши дословно из ресурса IDR_PLANRTTYPE. Мнемоника «&»
// сохранена: пользователь, привыкший к Alt+F, должен попасть в то же меню.
struct MenuItem
{
    UINT        id;          // 0 — разделитель
    const char *text;
    const char *shortcut;
    bool        checkable;
};

const MenuItem kFileMenu[] = {
    { ID_FILE_NEW,           "&New",            "Ctrl+N", false },
    { ID_FILE_OPEN,          "&Open...",        "Ctrl+O", false },
    { ID_FILE_CLOSE,         "&Close",          nullptr,  false },
    { ID_FILE_SAVE,          "&Save",           "Ctrl+S", false },
    { ID_FILE_SAVE_AS,       "Save &As...",     nullptr,  false },
    { 0, nullptr, nullptr, false },
    { ID_FILE_PRINT,         "&Print...",       "Ctrl+P", false },
    { ID_FILE_PRINT_PREVIEW, "Print Pre&view",  nullptr,  false },
    { ID_FILE_PRINT_SETUP,   "P&rint Setup...", nullptr,  false },
    { 0, nullptr, nullptr, false },
    { ID_APP_EXIT,           "E&xit",           nullptr,  false },
};

const MenuItem kEditMenu[] = {
    { ID_EDIT_EDIT,   "&Edit",   "Alt+F7", false },
    { 0, nullptr, nullptr, false },
    { ID_EDIT_UNDO,   "&Undo",   "Ctrl+Z", false },
    { 0, nullptr, nullptr, false },
    { ID_EDIT_CUT,    "Cu&t",    "Ctrl+X", false },
    { ID_EDIT_COPY,   "&Copy",   "Ctrl+C", false },
    { ID_EDIT_PASTE,  "&Paste",  "Ctrl+V", false },
};

const MenuItem kViewMenu[] = {
    { ID_VIEW_OUTPUT,               "Outp&ut",              "Alt+Shift+F4",  false },
    { ID_VIEW_FIELD_1,              "&Field and topology",  nullptr,         false },
    { ID_VIEW_STATISTICS,           "St&atistics",          "Shift+F4",      false },
    { ID_CONFIG_SMATRIX,            "S-&matrix",            "Ctrl+Shift+F4", false },
    { ID_VIEW_DIRECTIONALPATTERN,   "&Directional Pattern", "Ctrl+Alt+F4",   false },
};

const MenuItem kRunMenu[] = {
    { ID_RUN_RUN,          "Run",               "F5",       false },
    { ID_RUN_RESTARTALL,   "Load first step",   "Shift+F5", false },
    { 0, nullptr, nullptr, false },
    { ID_RUN_STARTSTEP,    "Run Step",          "F8",       false },
    { ID_RUN_RESTARTSTEP,  "Load Step",         "Shift+F8", false },
    { 0, nullptr, nullptr, false },
    { ID_RUN_SKIPSTEP,     "Skip Step",         "Ctrl+F6",  false },
    { ID_RUN_BACKSTEP,     "Back Step",         "Alt+F6",   false },
    { 0, nullptr, nullptr, false },
    { ID_RUN_STOP,         "&Stop",             "Alt+F5",   false },
};

const MenuItem kConfigViewerMenu[] = {
    { ID_CONFIG_VIEWER_OUTPUTSIGNAL,       "&Output signal",      nullptr, false },
    { ID_CONFIG_VIEWER_FIELD,              "&Field",              nullptr, false },
    { ID_CONFIG_VIEWER_SMATRIX,            "&S-matrix",           nullptr, false },
    { ID_CONFIG_VIEWER_DIRECTIONALPATTERN, "Directional Pattern", nullptr, false },
};

const MenuItem kConfigTailMenu[] = {
    { ID_VIEW_TOPOLOGY,           "T&opology",            "F4",      true  },
    { ID_VIEW_FIELD,              "&Field",               "Ctrl+F4", true  },
    { ID_CONFIG_SINCHRONIZATION,  "&Synchronization",     nullptr,   true  },
    { ID_CONFIG_DIRECTIONALPATTERN, "&Directional Pattern", nullptr, false },
};

// --- Панель инструментов ------------------------------------------------------
// Порядок кнопок и разделителей — из секции IDR_MAINFRAME TOOLBAR. Подсказки —
// вторая половина строк таблицы (после \n), как их показывала MFC.
struct ToolItem
{
    UINT        id;          // 0 — разделитель
    const char *iconName;
    const char *tip;
};

const ToolItem kToolBar[] = {
    { ID_FILE_NEW,                  "ID_FILE_NEW",                  "New" },
    { ID_FILE_OPEN,                 "ID_FILE_OPEN",                 "Open" },
    { 0, nullptr, nullptr },
    { ID_EDIT_EDIT,                 "ID_EDIT_EDIT",                 "Extern edit" },
    { ID_VIEW_OUTPUT,               "ID_VIEW_OUTPUT",               "View output" },
    { ID_VIEW_FIELD_1,              "ID_VIEW_FIELD_1",              "View field and topology" },
    { ID_CONFIG_SMATRIX,            "ID_CONFIG_SMATRIX",            "View S-matrix" },
    { ID_VIEW_DIRECTIONALPATTERN,   "ID_VIEW_DIRECTIONALPATTERN",   "View Directional Pattern" },
    { 0, nullptr, nullptr },
    { ID_VIEW_STATISTICS,           "ID_VIEW_STATISTICS",           "View statistics dialog" },
    { 0, nullptr, nullptr },
    { ID_RUN_RESTARTALL,            "ID_RUN_RESTARTALL",            "Load first step" },
    { ID_RUN_RUN,                   "ID_RUN_RUN",                   "Run" },
    { ID_RUN_STOP,                  "ID_RUN_STOP",                  "Stop" },
    { ID_RUN_STARTSTEP,             "ID_RUN_STARTSTEP",             "Run step" },
    { ID_RUN_RESTARTSTEP,           "ID_RUN_RESTARTSTEP",           "Load step" },
    { ID_RUN_SKIPSTEP,              "ID_RUN_SKIPSTEP",              "Skip step" },
    { ID_RUN_BACKSTEP,              "ID_RUN_BACKSTEP",              "Back step" },
    { 0, nullptr, nullptr },
    { ID_VIEW_TOPOLOGY,             "ID_VIEW_TOPOLOGY",             "Output topology" },
    { ID_VIEW_FIELD,                "ID_VIEW_FIELD",                "Output fields" },
    { ID_CONFIG_SINCHRONIZATION,    "ID_CONFIG_SINCHRONIZATION",    "Synchronization" },
    { ID_CONFIG_DIRECTIONALPATTERN, "ID_CONFIG_DIRECTIONALPATTERN", "Export to directional Pattern file" },
    { 0, nullptr, nullptr },
    { ID_CONFIG_SOUND,              "ID_CONFIG_SOUND",              "Sound effect" },
    { ID_CONFIG_AUTORUN,            "ID_CONFIG_AUTORUN",            "Batch mode" },
    { 0, nullptr, nullptr },
    { ID_FILE_PRINT,                "ID_FILE_PRINT",                "Print" },
    { ID_APP_ABOUT,                 "ID_APP_ABOUT",                 "About" },
};

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    g_mainWindow = this;

    // Пиктограммы и разметка диалогов лежат в статической библиотеке. Без явной
    // ссылки компоновщик выбросил бы их инициализаторы: панель осталась бы без
    // картинок, а диалоги — без разметки.
    TmcPlanRtInstallResources();

    // Сообщения библиотек и вопросы кода Windows-версии — в окна Qt.
    tmc_set_message_handler(&TmcQtMessage);
    TmcPlanRtSetMessageBoxHandler(&TmcQtMessageBox);
    TmcPlanRtSetCloseHandler(&TmcQtCloseFrame);

    // Заголовок — строка IDR_MAINFRAME из ресурсов программы.
    setWindowTitle(QString::fromLatin1(TmcPlanRtApp().windowTitle));
    setWindowIcon(QIcon(QString::fromLatin1(TmcPlanRtApp().windowIcon)));

    m_mdi = new QMdiArea(this);
    m_mdi->setViewMode(QMdiArea::SubWindowView);
    setCentralWidget(m_mdi);
    connect(m_mdi, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated);

    createMenus();
    createToolBar();
    createStatusBar();

    // Код, перенесённый из Windows-версии, достаёт строку состояния через
    // главное окно приложения (PutTrace, PutStatistics, PutSinchronizFlag).
    // Отдаём ему рамку, у которой строка состояния связана с нашей.
    m_frame = new CMainFrame();
    m_frame->m_wndStatusBar.SetHost(&g_statusHost);
    AfxGetApp()->m_pMainWnd = m_frame;
    // По этому указателю расчётный узел закрывает программу в пакетном режиме
    // (в Windows-версии его ставил CMainFrame::OnCreate).
    SetMainFramePointer__(m_frame);

    // В MFC доступность пунктов меню пересчитывалась в цикле ожидания
    // (ON_UPDATE_COMMAND_UI). Здесь то же самое делает таймер: состояние
    // меняется из расчётного потока, и меню должно за ним успевать.
    m_uiTimer = new QTimer(this);
    m_uiTimer->setInterval(200);
    connect(m_uiTimer, &QTimer::timeout, this, &MainWindow::updateCommandStates);
    m_uiTimer->start();

    resize(1130, 700);

    // Положение окна запоминается между запусками — как это делал
    // каркас MFC. Иначе окно каждый раз появляется в новом месте.
    tmcui::keepPlacement(this);

    // Защита от «залипшего» захвата мыши дочерним окном: без неё
    // программа может перестать отвечать после перетаскивания.
    tmcui::installMouseGrabWatchdog(this);
    updateCommandStates();
}

MainWindow::~MainWindow()
{
    // Разрушение окна закрывает окна документов, а на каждое закрытие область
    // MDI шлёт subWindowActivated. Слот читает карту действий, а её к тому
    // времени уже нет: члены разрушаются раньше базового QWidget. Поэтому
    // связь с областью документов снимаем первым делом.
    if (m_mdi)
        disconnect(m_mdi, nullptr, this, nullptr);

    for (TmcPlanRtWindow *d : m_documents) {
        // Виджет-хозяин Qt удалит позже, вместе с окном, а в своём деструкторе
        // он обращается к виду. Поэтому связь снимаем заранее — иначе выход из
        // программы кончается обращением к освобождённой памяти.
        if (d->widget)
            d->widget->setView(nullptr);
        delete d->view;
        delete d->doc;
        delete d;
    }
    SetMainFramePointer__(nullptr);
    AfxGetApp()->m_pMainWnd = nullptr;
    delete m_frame;
    g_mainWindow = nullptr;
}

QAction *MainWindow::makeAction(UINT id, const QString &text, const QString &shortcut,
                                bool checkable)
{
    QAction *a = new QAction(text, this);
    // Номер команды храним в самом действии: по нему работают
    // обновление состояния пунктов и автоматические проверки.
    a->setData(int(id));
    if (!shortcut.isEmpty())
        a->setShortcut(QKeySequence(shortcut));
    a->setCheckable(checkable);
    connect(a, &QAction::triggered, this, [this, id]() { dispatch(id); });
    m_actions.insert(id, a);
    return a;
}

void MainWindow::createMenus()
{
    const auto addItems = [this](QMenu *menu, const MenuItem *items, int count) {
        for (int i = 0; i < count; i++) {
            const MenuItem &it = items[i];
            if (!it.id) { menu->addSeparator(); continue; }
            menu->addAction(makeAction(it.id, QString::fromLatin1(it.text),
                                       it.shortcut ? QString::fromLatin1(it.shortcut) : QString(),
                                       it.checkable));
        }
    };

    QMenu *file = menuBar()->addMenu(QStringLiteral("&File"));
    addItems(file, kFileMenu, int(sizeof(kFileMenu) / sizeof(kFileMenu[0])));

    QMenu *edit = menuBar()->addMenu(QStringLiteral("&Edit"));
    addItems(edit, kEditMenu, int(sizeof(kEditMenu) / sizeof(kEditMenu[0])));

    QMenu *view = menuBar()->addMenu(QStringLiteral("&View"));
    addItems(view, kViewMenu, int(sizeof(kViewMenu) / sizeof(kViewMenu[0])));
    view->addSeparator();
    // Показ панели и строки состояния делает сам Qt — обработчиков в виде нет.
    QAction *toolbarAction = new QAction(QStringLiteral("&Toolbar"), this);
    toolbarAction->setCheckable(true);
    toolbarAction->setChecked(true);
    connect(toolbarAction, &QAction::toggled, this, [this](bool on) {
        if (QToolBar *tb = findChild<QToolBar *>())
            tb->setVisible(on);
    });
    view->addAction(toolbarAction);
    QAction *statusAction = new QAction(QStringLiteral("&Status Bar"), this);
    statusAction->setCheckable(true);
    statusAction->setChecked(true);
    connect(statusAction, &QAction::toggled, this, [this](bool on) { statusBar()->setVisible(on); });
    view->addAction(statusAction);

    QMenu *run = menuBar()->addMenu(QStringLiteral("&Run"));
    addItems(run, kRunMenu, int(sizeof(kRunMenu) / sizeof(kRunMenu[0])));

    QMenu *config = menuBar()->addMenu(QStringLiteral("&Config"));
    config->addAction(makeAction(ID_CONFIG_EDITOR, QStringLiteral("&Editor")));
    config->addSeparator();
    QMenu *viewer = config->addMenu(QStringLiteral("Viewer"));
    addItems(viewer, kConfigViewerMenu, int(sizeof(kConfigViewerMenu) / sizeof(kConfigViewerMenu[0])));
    QMenu *color = config->addMenu(QStringLiteral("&Color"));
    color->addAction(makeAction(ID_CONFIG_COLOR_BACKGROUND, QStringLiteral("&BackGround")));
    QMenu *format = config->addMenu(QStringLiteral("&Format"));
    format->addAction(makeAction(ID_CONFIG_FORMAT_OUTPUTDATAFILE, QStringLiteral("&OutputDataFile")));
    config->addSeparator();
    addItems(config, kConfigTailMenu, int(sizeof(kConfigTailMenu) / sizeof(kConfigTailMenu[0])));
    config->addSeparator();
    QMenu *sound = config->addMenu(QStringLiteral("Sound"));
    sound->addAction(makeAction(ID_CONFIG_SOUND, QStringLiteral("On/Off"), QString(), true));
    sound->addAction(makeAction(ID_CONFIG_SOUND_MELODY, QStringLiteral("Melody")));
    config->addAction(makeAction(ID_CONFIG_AUTORUN, QStringLiteral("&AutoRun"), QString(), true));
    config->addAction(makeAction(ID_CONFIG_SETUP, QStringLiteral("Setup")));

    QMenu *window = menuBar()->addMenu(QStringLiteral("&Window"));
    QAction *cascade = window->addAction(QStringLiteral("&Cascade"));
    connect(cascade, &QAction::triggered, m_mdi, &QMdiArea::cascadeSubWindows);
    QAction *tile = window->addAction(QStringLiteral("&Tile"));
    connect(tile, &QAction::triggered, m_mdi, &QMdiArea::tileSubWindows);
    QAction *arrange = window->addAction(QStringLiteral("&Arrange Icons"));
    connect(arrange, &QAction::triggered, m_mdi, &QMdiArea::tileSubWindows);

    QMenu *help = menuBar()->addMenu(QStringLiteral("&Help"));
    help->addAction(makeAction(ID_APP_ABOUT, QString::fromLatin1(TmcPlanRtApp().aboutMenuItem)));

    // Перед показом любого меню состояние пунктов пересчитывается — ровно так,
    // как каркас MFC опрашивал обработчики ON_UPDATE_COMMAND_UI перед выводом
    // меню на экран.
    const QList<QMenu *> menus = menuBar()->findChildren<QMenu *>();
    for (QMenu *menu : menus)
        connect(menu, &QMenu::aboutToShow, this, &MainWindow::updateCommandStates);
}

void MainWindow::createToolBar()
{
    QToolBar *tb = addToolBar(QStringLiteral("Toolbar"));
    tb->setMovable(false);
    tb->setIconSize(QSize(16, 15));    // размер кнопок из ресурсов Windows

    for (const ToolItem &it : kToolBar) {
        if (!it.id) { tb->addSeparator(); continue; }
        QAction *a = m_actions.value(it.id, nullptr);
        if (!a)
            a = makeAction(it.id, QString::fromLatin1(it.tip));
        a->setIcon(theme::commandIcon(QString::fromLatin1(it.iconName)));
        a->setToolTip(QString::fromLatin1(it.tip));
        tb->addAction(a);
    }
}

void MainWindow::createStatusBar()
{
    // Три панели, как в CMainFrame::indicators Windows-версии:
    //   0 — ход счёта (PutTrace), 1 — статистика (PutStatistics),
    //   2 — что выводится (PutSinchronizFlag).
    m_statusTrace = new QLabel(QStringLiteral("Ready"), this);
    m_statusStat  = new QLabel(QString(), this);
    m_statusFlags = new QLabel(QString(), this);
    statusBar()->addWidget(m_statusTrace, 1);
    statusBar()->addPermanentWidget(m_statusStat);
    statusBar()->addPermanentWidget(m_statusFlags);
}

void MainWindow::setStatusPane(int pane, const QString &text)
{
    switch (pane) {
    case 0:  m_statusTrace->setText(text); break;
    case 1:  m_statusStat->setText(text);  break;
    default: m_statusFlags->setText(text); break;
    }
}

void MainWindow::closeFromKernel()
{
    close();
}

TmcPlanRtWindow *MainWindow::activeDocument() const
{
    QMdiSubWindow *sub = m_mdi->activeSubWindow();
    if (!sub)
        return nullptr;
    for (TmcPlanRtWindow *d : m_documents) {
        if (d->sub == sub)
            return d;
    }
    return nullptr;
}

void MainWindow::dispatch(UINT commandId)
{
    // Сначала — вид активного окна: там живут обработчики Windows-версии.
    if (TmcPlanRtWindow *d = activeDocument()) {
        MfcMessageMap map;
        d->view->TmcBuildMessageMap(map);
        if (map.call(commandId)) {
            d->widget->viewport()->update();
            updateCommandStates();
            return;
        }
    }

    // Затем — команды самого окна.
    switch (commandId) {
    case ID_FILE_NEW:   onNewDocument(); break;
    case ID_FILE_OPEN:  onOpenDocument(); break;
    case ID_FILE_CLOSE: onCloseDocument(); break;
    case ID_APP_EXIT:   close(); break;
    case ID_APP_ABOUT:  onAbout(); break;
    default:
        // Команда есть в меню, но обработчика нет — как в MFC, где пункт
        // оставался недоступным. Молчим, чтобы не пугать пользователя.
        break;
    }
}

void MainWindow::onNewDocument()
{
    // В Windows-версии новое задание создаётся во внешнем редакторе: программа
    // умеет открывать и считать уже готовый .tpl.
    QMessageBox::information(this, windowTitle(),
                             QStringLiteral("Задание (.tpl) готовится во внешнем редакторе. "
                                            "Откройте готовый файл через File -> Open."));
}

void MainWindow::onOpenDocument()
{
    // Фильтр — из строки IDR_PLANRTTYPE: «Planar RT H analyzer Files (*.tpl)».
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Open"), QString(),
        QString::fromLatin1(TmcPlanRtApp().docTypeName) + QStringLiteral(";;All Files (*.*)"));
    if (!path.isEmpty())
        openDocument(path);
}

void MainWindow::openDocument(const QString &path)
{
    TmcPlanRtWindow *d = new TmcPlanRtWindow;
    d->doc = CPlanRT_HDoc::TmcCreate();
    d->doc->SetPathName(path.toLocal8Bit().constData());
    d->doc->SetTitle(QFileInfo(path).fileName().toLocal8Bit().constData());

    d->view = CPlanRT_HView::TmcCreate();
    d->view->m_pDocument = d->doc;

    d->widget = new MfcViewWidget(this);
    d->widget->setView(d->view);
    connect(d->widget, &MfcViewWidget::statusTextChanged, this, &MainWindow::setStatusPane);

    d->sub = m_mdi->addSubWindow(d->widget);
    d->sub->setWindowTitle(QFileInfo(path).fileName());
    d->sub->setAttribute(Qt::WA_DeleteOnClose, false);
    d->widget->show();
    // В Windows-версии дочернее окно создаётся развёрнутым (WS_MAXIMIZE в
    // CChildFrame::PreCreateWindow) — повторяем.
    d->sub->showMaximized();

    m_documents.append(d);

    // Вид читает настройки и файл задания — тот же порядок, что в MFC:
    // OnInitialUpdate вызывается после того, как окно получило размер.
    d->widget->initialUpdate();

    updateCommandStates();
}

void MainWindow::onCloseDocument()
{
    if (QMdiSubWindow *sub = m_mdi->activeSubWindow())
        sub->close();
}

void MainWindow::onAbout()
{
    // Содержание окна — из диалога IDD_ABOUTBOX ресурса программы, плюс строка
    // о модели счёта, которую Windows-версия дописывала во время работы.
    const TmcPlanRtAppInfo &app = TmcPlanRtApp();
    tmcabout::show(this, QString::fromLatin1(app.aboutMenuItem).remove(QLatin1Char('&')),
                   QString::fromLatin1(app.aboutProduct),
                   QStringLiteral("%1<br>Model : %2")
                       .arg(QString::fromLatin1(app.aboutCopyright),
                            QString::fromLatin1((const char *)GetModel())));
}

void MainWindow::onSubWindowActivated(QMdiSubWindow *)
{
    updateCommandStates();
}

void MainWindow::updateCommandStates()
{
    TmcPlanRtWindow *d = activeDocument();
    const bool hasDoc = (d != nullptr);

    // Без открытого документа доступны только команды самого окна — тот же
    // смысл, что в Windows-версии, где при закрытых окнах показывалось короткое
    // меню IDR_MAINFRAME.
    for (QMap<UINT, QAction *>::iterator it = m_actions.begin(); it != m_actions.end(); ++it) {
        const UINT id = it.key();
        const bool always = (id == ID_FILE_NEW || id == ID_FILE_OPEN ||
                             id == ID_APP_EXIT || id == ID_APP_ABOUT);
        it.value()->setEnabled(always || hasDoc);
    }

    // Дальше слово за обработчиками ON_UPDATE_COMMAND_UI вида — тем же кодом,
    // что и на Windows: доступность пунктов Run и Stop, галочки Topology,
    // Field, Synchronization, Sound, AutoRun. Ничего из этого здесь не
    // повторяется, иначе условия пришлось бы держать в двух местах.
    if (hasDoc)
        tmccmd::updateActions(m_actions, d->view);
}

bool MainWindow::stopAllAndConfirm()
{
    // Тот же порядок, что в CMainFrame::OnClose Windows-версии: если счёт идёт
    // и это не пакетный режим — спросить; затем остановить и дождаться потока.
    for (TmcPlanRtWindow *d : m_documents) {
        if (d->doc->IsRun() && !IsBatchRun()) {
            const QMessageBox::StandardButton answer = QMessageBox::warning(
                this, windowTitle(),
                QStringLiteral("Computation in progress. Stop and close?"),
                QMessageBox::Yes | QMessageBox::No);
            if (answer != QMessageBox::Yes)
                return false;
        }
    }
    for (TmcPlanRtWindow *d : m_documents)
        d->doc->StopAndWait();
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (!stopAllAndConfirm()) {
        event->ignore();
        return;
    }
    if (m_uiTimer)
        m_uiTimer->stop();
    event->accept();
}
