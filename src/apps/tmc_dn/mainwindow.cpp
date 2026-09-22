#include "aboutbox.h"
#include "mainwindow.h"

#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QLabel>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMenuBar>
#include <QMessageBox>
#include <QStatusBar>
#include <QThread>
#include <QToolBar>

#include "dnviewwidget.h"
#include "mfcviewwidget.h"
#include "commandupdate.h"
#include "theme.h"
#include "mousewatchdog.h"
#include "windowplacement.h"

// Классы, перенесённые из Windows-версии: документ, вид и рамка окна.
// c2darray.h идёт первым: документ держит указатель на решётку излучателей,
// а в Windows-версии это объявление приходило из stdafx.h программы.
#include "c2darray.h"
#include "tmcgroutdoc.h"
#include "tmcgroutview.h"
#include "mainfrm.h"
#include "resource.h"

// Разметка диалогов из dialogs/dialog_registry.cpp (порождён tools/rc2ui.py):
// заполняет таблицы слоя совместимости — «диалог -> разметка» и
// «номер элемента -> имя».
void TmcInstallDialogs_tmc_dn();

namespace {

// Подписи меню — дословно из ресурсов Windows-версии (IDR_TMCGROTYPE).
// Мнемоника «&» и подсказки клавиш сохранены: пользователь, привыкший к Alt+F,
// должен попасть в то же меню.
struct MenuItem
{
    UINT        id;          // 0 — разделитель
    const char *text;
    const char *shortcut;
};

const MenuItem kFileMenu[] = {
    { ID_FILE_NEW,           "&New",             "Ctrl+N" },
    { ID_FILE_OPEN,          "&Open...",         "Ctrl+O" },
    { ID_FILE_CLOSE,         "&Close",           nullptr },
    { ID_FILE_SAVE,          "&Save",            "Ctrl+S" },
    { ID_FILE_SAVE_AS,       "Save &As...",      nullptr },
    { 0, nullptr, nullptr },
    { ID_FILE_PRINT,         "&Print...",        "Ctrl+P" },
    { ID_FILE_PRINT_PREVIEW, "Print Pre&view",   nullptr },
    { ID_FILE_PRINT_SETUP,   "P&rint Setup...",  nullptr },
    { 0, nullptr, nullptr },
    { ID_APP_EXIT,           "E&xit",            nullptr },
};

// В TMC_DN пункта «To S-matrix» нет: диаграмма направленности строится по
// решётке излучателей, а не по S-матрице.
const MenuItem kEditMenu[] = {
    { ID_EDIT_DOCUMENT,           "&Characteristics",      "Ctrl+D" },
    { ID_EDIT_ADDCHARACTERISTICS, "&Add characteristics",  "Ctrl+A" },
    { ID_EDIT_EDIT,               "&Editor",               "Alt+F7" },
};

const MenuItem kZoomMenu[] = {
    { ID_VIEW_RESIZECTRLR,   "&ReSize viewport", "Ctrl+R" },
    { 0, nullptr, nullptr },
    { ID_VIEW_ZOOM_ZOOMP,    "Zoom +",           "+" },
    { ID_VIEW_ZOOM_ZOOMXP,   "Zoom +X",          "X" },
    { ID_VIEW_ZOOM_ZOOMYP,   "Zoom +Y",          "Y" },
    { ID_VIEW_ZOOM_ZOOM,     "Zoom -",           "-" },
    { ID_VIEW_ZOOM_ZOOMX,    "Zoom -X",          "Shift+X" },
    { ID_VIEW_ZOOM_ZOOMY,    "Zoom -Y",          "Shift+Y" },
    { 0, nullptr, nullptr },
    { ID_VIEW_RESIZEWINDOW,  "R&esize window",   "Ctrl+T" },
    { ID_VIEW_AUTOXSIZE,     "Auto X size",      "Alt+X" },
    { ID_VIEW_AUTOYSIZE,     "Auto Y size",      "Alt+Y" },
};

const MenuItem kChangeXMenu[] = {
    { ID_VIEW_CHANGEXMAXXMIN_DECRIMENT,        "Decrement", "Left" },
    { ID_VIEW_CHANGEXMAXXMIN_INCREMENT,        "Increment", "Right" },
    { ID_VIEW_TRANSLATE_CHANGEXMAXXMIN_HOME,   "Home",      "Home" },
    { ID_VIEW_TRANSLATE_CHANGEXMAXXMIN_END,    "End",       "End" },
};

const MenuItem kChangeYMenu[] = {
    { ID_VIEW_CHANGEYMAXYMIN_DECREMENT, "Decrement", "V" },
    { ID_VIEW_CHANGEYMAXYMIN_INCREMENT, "Increment", "Up" },
};

const MenuItem kColorMenu[] = {
    { ID_CONFIG_COLOR_BACKGROUND, "&Background", nullptr },
    { ID_CONFIG_COLOR_GRID,       "&Grid",       nullptr },
    { ID_CONFIG_COLOR_AXIS,       "&Axies",      nullptr },
    { ID_CONFIG_COLORPOINT,       "&Point",      nullptr },
    { ID_CONFIG_COLOR_GRAPHICS,   "&Graphics",   nullptr },
};

// Панель инструментов — порядок кнопок и разделителей из секции
// IDR_MAINFRAME TOOLBAR ресурсов TMC_DN.
struct ToolItem
{
    UINT        id;          // 0 — разделитель
    const char *iconName;
    const char *tip;
};

const ToolItem kToolBar[] = {
    { ID_FILE_OPEN,                          "ID_FILE_OPEN",                          "Open" },
    { 0, nullptr, nullptr },
    { ID_EDIT_ADDCHARACTERISTICS,            "ID_EDIT_ADDCHARACTERISTICS",            "Add characteristics" },
    { ID_EDIT_DOCUMENT,                      "ID_EDIT_DOCUMENT",                      "Characteristics" },
    { 0, nullptr, nullptr },
    { ID_EDIT_GRAPHICSPARAMETERS,            "ID_EDIT_GRAPHICSPARAMETERS",            "Viewport" },
    { ID_VIEW_GRAPHICS,                      "ID_VIEW_GRAPHICS",                      "Redraw data" },
    { 0, nullptr, nullptr },
    { ID_VIEW_RESIZECTRLR,                   "ID_VIEW_RESIZECTRLR",                   "ReSize viewport" },
    { 0, nullptr, nullptr },
    { ID_VIEW_ZOOM_ZOOMP,                    "ID_VIEW_ZOOM_ZOOMP",                    "Zoom +" },
    { ID_VIEW_ZOOM_ZOOMXP,                   "ID_VIEW_ZOOM_ZOOMXP",                   "Zoom +X" },
    { ID_VIEW_ZOOM_ZOOMYP,                   "ID_VIEW_ZOOM_ZOOMYP",                   "Zoom +Y" },
    { ID_VIEW_ZOOM_ZOOM,                     "ID_VIEW_ZOOM_ZOOM",                     "Zoom -" },
    { ID_VIEW_ZOOM_ZOOMX,                    "ID_VIEW_ZOOM_ZOOMX",                    "Zoom -X" },
    { ID_VIEW_ZOOM_ZOOMY,                    "ID_VIEW_ZOOM_ZOOMY",                    "Zoom -Y" },
    { 0, nullptr, nullptr },
    { ID_VIEW_RESIZEWINDOW,                  "ID_VIEW_RESIZEWINDOW",                  "Resize window" },
    { ID_VIEW_AUTOXSIZE,                     "ID_VIEW_AUTOXSIZE",                     "Auto X size" },
    { ID_VIEW_AUTOYSIZE,                     "ID_VIEW_AUTOYSIZE",                     "Auto Y size" },
    { 0, nullptr, nullptr },
    { ID_VIEW_TRANSLATE_CHANGEXMAXXMIN_HOME, "ID_VIEW_TRANSLATE_CHANGEXMAXXMIN_HOME", "Home" },
    { ID_VIEW_TRANSLATE_CHANGEXMAXXMIN_END,  "ID_VIEW_TRANSLATE_CHANGEXMAXXMIN_END",  "End" },
    { ID_VIEW_CHANGEXMAXXMIN_DECRIMENT,      "ID_VIEW_CHANGEXMAXXMIN_DECRIMENT",      "Decrement X" },
    { ID_VIEW_CHANGEXMAXXMIN_INCREMENT,      "ID_VIEW_CHANGEXMAXXMIN_INCREMENT",      "Increment X" },
    { ID_VIEW_CHANGEYMAXYMIN_DECREMENT,      "ID_VIEW_CHANGEYMAXYMIN_DECREMENT",      "Decrement Y" },
    { ID_VIEW_CHANGEYMAXYMIN_INCREMENT,      "ID_VIEW_CHANGEYMAXYMIN_INCREMENT",      "Increment Y" },
    { 0, nullptr, nullptr },
    { ID_FILE_PRINT,                         "ID_FILE_PRINT",                         "Print" },
    { ID_APP_ABOUT,                          "ID_APP_ABOUT",                          "About TMC_DN" },
};

} // namespace

namespace {

// Строка состояния для кода, перенесённого из Windows-версии.
//
// Функции PutTrace и PutStatistics (tmcgroutview.cpp) пишут текст не через вид,
// а через строку состояния главной рамки: AfxGetApp()->m_pMainWnd->m_wndStatusBar.
// В Windows это была настоящая CStatusBar; здесь у неё должен быть «хозяин»,
// иначе текст (в TMC_DN это «Pattern; Freq=10 GHz» в правой части) никуда не
// попадёт. Хозяин обязан реализовать весь интерфейс MfcQtHost, но строке
// состояния из него нужен ровно один метод — остальные пустые.
class StatusBarHost : public MfcQtHost
{
public:
    explicit StatusBarHost(MainWindow *owner) : m_owner(owner) {}

    void hostSetStatusText(int pane, const char *text) override
    {
        if (!m_owner)
            return;
        const QString s = QString::fromLocal8Bit(text ? text : "");
        // PutStatistics зовётся и из потока расчёта диаграммы. Менять надпись
        // в чужом потоке нельзя, поэтому оттуда — через очередь событий окна.
        if (QThread::currentThread() == m_owner->thread()) {
            m_owner->putStatusText(pane, s);
        } else {
            MainWindow *owner = m_owner;
            QMetaObject::invokeMethod(owner, [owner, pane, s]() {
                owner->putStatusText(pane, s);
            }, Qt::QueuedConnection);
        }
    }

    // Ниже — то, чем строка состояния не пользуется.
    void hostInvalidate(const RECT *, BOOL) override {}
    void hostUpdateWindow() override {}
    void hostGetClientRect(RECT *rect) const override
    {
        if (rect) { rect->left = rect->top = rect->right = rect->bottom = 0; }
    }
    void hostSetCapture() override {}
    void hostReleaseCapture() override {}
    UINT hostSetTimer(UINT id, UINT) override { return id; }
    BOOL hostKillTimer(UINT) override { return TRUE; }
    int  hostMessageBox(const char *, const char *, UINT) override { return 0; }
    void hostSetScrollSizes(const SIZE &, const SIZE &, const SIZE &) override {}
    CPoint hostScrollPosition() const override { return CPoint(0, 0); }
    void hostScrollToPosition(const POINT &) override {}
    int  hostLogicalDpiX() const override { return 96; }
    int  hostLogicalDpiY() const override { return 96; }

private:
    MainWindow *m_owner;
};

} // namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // Пиктограммы собраны в статическую библиотеку. Компоновщик выбрасывает
    // инициализатор ресурсов, если на него никто не ссылается, поэтому зовём
    // его явно — иначе панель окажется без картинок.
    Q_INIT_RESOURCE(tmc_dn);

    // Разметка диалогов (dialogs/dialog_registry.cpp, порождён tools/rc2ui.py).
    // Вызов явный по той же причине, что и Q_INIT_RESOURCE: файл лежит в
    // статической библиотеке, и без ссылки на него компоновщик его не подключит —
    // диалоги остались бы без разметки.
    TmcInstallDialogs_tmc_dn();

    // Заголовок — как в ресурсах Windows-версии (строка IDR_MAINFRAME).
    // Строка AFX_IDS_APP_TITLE в том же .rc осталась от TMCGROUT и не берётся.
    setWindowTitle(QStringLiteral("Tamic Directional Pattern Viewer"));
    setWindowIcon(QIcon(QStringLiteral(":/icons/appicon.png")));

    m_mdi = new QMdiArea(this);
    m_mdi->setViewMode(QMdiArea::SubWindowView);
    m_mdi->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_mdi->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    setCentralWidget(m_mdi);
    connect(m_mdi, &QMdiArea::subWindowActivated, this, &MainWindow::onSubWindowActivated);

    createMenus();

    // Перед показом каждого меню обновляем состояние его пунктов — тот же
    // порядок, что был в MFC: каркас опрашивал обработчики обновления,
    // а не хранил состояние отдельно.
    for (QMenu *menu : menuBar()->findChildren<QMenu *>())
        connect(menu, &QMenu::aboutToShow, this, [this]() { updateCommandStates(); });

    createToolBar();
    createStatusBar();

    // Код, перенесённый из Windows-версии, достаёт строку состояния через
    // главное окно приложения. Отдаём ему рамку, у которой строка состояния
    // связана с нашей — так PutTrace и PutStatistics работают без правок.
    m_frame = new CMainFrame();
    m_statusHost = new StatusBarHost(this);
    m_frame->m_wndStatusBar.SetHost(m_statusHost);
    AfxGetApp()->m_pMainWnd = m_frame;

    resize(1130, 700);

    // Положение окна запоминается между запусками — как это делал
    // каркас MFC. Иначе окно каждый раз появляется в новом месте.
    tmcui::keepPlacement(this);

    // Защита от «залипшего» захвата мыши дочерним окном: без неё
    // программа может перестать отвечать после перетаскивания.
    tmcui::installMouseGrabWatchdog(this);
    updateMenuForDocument();
}

MainWindow::~MainWindow()
{
    // Разрушение окна закрывает окна документов, а на каждое закрытие область
    // MDI шлёт subWindowActivated. Слот читает карту действий, а её к тому
    // времени уже нет: члены разрушаются раньше базового QWidget. Поэтому
    // связь с областью документов снимаем первым делом.
    if (m_mdi)
        disconnect(m_mdi, nullptr, this, nullptr);

    for (TmcDocumentWindow *d : m_documents) {
        // Сначала отвязываем вид от виджета. Виджеты Qt удаляются позже нас
        // (они дети окна), и в своём деструкторе виджет-хозяин трогает вид:
        // если вид уже удалён, это обращение к освобождённой памяти.
        if (d->widget)
            d->widget->setView(nullptr);
        delete d->view;
        delete d->doc;
        delete d;
    }
    delete m_frame;
    delete static_cast<StatusBarHost *>(m_statusHost);
}

QAction *MainWindow::makeAction(UINT id, const QString &text, const QString &shortcut)
{
    QAction *a = new QAction(text, this);
    // Номер команды храним в самом действии: по нему работают
    // обновление состояния пунктов и автоматические проверки.
    a->setData(int(id));
    if (!shortcut.isEmpty())
        a->setShortcut(QKeySequence(shortcut));
    connect(a, &QAction::triggered, this, [this, id]() { dispatch(id); });
    m_actions.insert(id, a);
    return a;
}

void MainWindow::createMenus()
{
    QMenu *file = menuBar()->addMenu(QStringLiteral("&File"));
    for (const MenuItem &it : kFileMenu) {
        if (!it.id) { file->addSeparator(); continue; }
        file->addAction(makeAction(it.id, QString::fromLatin1(it.text),
                                   it.shortcut ? QString::fromLatin1(it.shortcut) : QString()));
    }

    QMenu *edit = menuBar()->addMenu(QStringLiteral("&Edit"));
    for (const MenuItem &it : kEditMenu)
        edit->addAction(makeAction(it.id, QString::fromLatin1(it.text),
                                   it.shortcut ? QString::fromLatin1(it.shortcut) : QString()));

    QMenu *view = menuBar()->addMenu(QStringLiteral("&View"));
    view->addAction(makeAction(ID_EDIT_GRAPHICSPARAMETERS, QStringLiteral("&Viewport"), QStringLiteral("Ctrl+H")));
    view->addAction(makeAction(ID_VIEW_GRAPHICS, QStringLiteral("Redra&w data"), QStringLiteral("Ctrl+W")));
    view->addSeparator();

    QMenu *zoom = view->addMenu(QStringLiteral("Zoom"));
    for (const MenuItem &it : kZoomMenu) {
        if (!it.id) { zoom->addSeparator(); continue; }
        zoom->addAction(makeAction(it.id, QString::fromLatin1(it.text),
                                   it.shortcut ? QString::fromLatin1(it.shortcut) : QString()));
    }

    QMenu *translate = view->addMenu(QStringLiteral("Translate"));
    QMenu *changeX = translate->addMenu(QStringLiteral("Change Xmax, Xmin"));
    for (const MenuItem &it : kChangeXMenu)
        changeX->addAction(makeAction(it.id, QString::fromLatin1(it.text),
                                      it.shortcut ? QString::fromLatin1(it.shortcut) : QString()));
    QMenu *changeY = translate->addMenu(QStringLiteral("Change Ymax, Ymin"));
    for (const MenuItem &it : kChangeYMenu)
        changeY->addAction(makeAction(it.id, QString::fromLatin1(it.text),
                                      it.shortcut ? QString::fromLatin1(it.shortcut) : QString()));

    view->addSeparator();
    QAction *statusAction = new QAction(QStringLiteral("&Status Bar"), this);
    statusAction->setCheckable(true);
    statusAction->setChecked(true);
    connect(statusAction, &QAction::toggled, this, [this](bool on) { statusBar()->setVisible(on); });
    view->addAction(statusAction);

    QAction *toolbarAction = new QAction(QStringLiteral("&Toolbar"), this);
    toolbarAction->setCheckable(true);
    toolbarAction->setChecked(true);
    connect(toolbarAction, &QAction::toggled, this, [this](bool on) {
        if (QToolBar *tb = findChild<QToolBar *>())
            tb->setVisible(on);
    });
    view->addAction(toolbarAction);

    QMenu *config = menuBar()->addMenu(QStringLiteral("&Config"));
    config->addAction(makeAction(ID_CONFIG_EDITOR, QStringLiteral("&Editor")));
    QMenu *color = config->addMenu(QStringLiteral("&Color"));
    for (const MenuItem &it : kColorMenu)
        color->addAction(makeAction(it.id, QString::fromLatin1(it.text)));
    config->addAction(makeAction(ID_CONFIG_FONT, QStringLiteral("&Font")));

    m_windowMenu = menuBar()->addMenu(QStringLiteral("&Window"));
    QAction *newWindow = new QAction(QStringLiteral("&New Window"), this);
    connect(newWindow, &QAction::triggered, this, [this]() {
        if (TmcDocumentWindow *d = activeDocument())
            openDocument(QString::fromLocal8Bit(d->doc->GetPathName()));
    });
    m_windowMenu->addAction(newWindow);
    QAction *cascade = m_windowMenu->addAction(QStringLiteral("&Cascade"));
    connect(cascade, &QAction::triggered, m_mdi, &QMdiArea::cascadeSubWindows);
    QAction *tile = m_windowMenu->addAction(QStringLiteral("&Tile"));
    connect(tile, &QAction::triggered, m_mdi, &QMdiArea::tileSubWindows);

    QMenu *help = menuBar()->addMenu(QStringLiteral("&Help"));
    help->addAction(makeAction(ID_APP_ABOUT, QStringLiteral("&About TMC_DN...")));
}

void MainWindow::createToolBar()
{
    QToolBar *tb = addToolBar(QStringLiteral("Toolbar"));
    tb->setMovable(false);
    tb->setIconSize(QSize(16, 15));    // размер кнопок из ресурсов Windows

    for (const ToolItem &it : kToolBar) {
        if (!it.id) { tb->addSeparator(); continue; }
        QAction *a = m_actions.value(it.id, nullptr);
        if (!a) {
            a = makeAction(it.id, QString::fromLatin1(it.tip));
        }
        a->setIcon(theme::commandIcon(QString::fromLatin1(it.iconName)));
        a->setToolTip(QString::fromLatin1(it.tip));
        tb->addAction(a);
    }
}

void MainWindow::createStatusBar()
{
    // Текст по умолчанию — из строковой таблицы MFC (AFX_IDS_IDLEMESSAGE).
    m_statusMain = new QLabel(QStringLiteral("Ready"), this);
    m_statusExtra = new QLabel(QString(), this);
    statusBar()->addWidget(m_statusMain, 1);
    statusBar()->addPermanentWidget(m_statusExtra);
}

void MainWindow::putStatusText(int pane, const QString &text)
{
    onStatusText(pane, text);
}

void MainWindow::onStatusText(int pane, const QString &text)
{
    if (pane <= 0)
        m_statusMain->setText(text);
    else
        m_statusExtra->setText(text);
}

TmcDocumentWindow *MainWindow::activeDocument() const
{
    QMdiSubWindow *sub = m_mdi->activeSubWindow();
    if (!sub)
        return nullptr;
    for (TmcDocumentWindow *d : m_documents) {
        if (d->sub == sub)
            return d;
    }
    return nullptr;
}

void MainWindow::dispatch(UINT commandId)
{
    // Сначала — вид активного окна: там живут обработчики Windows-версии.
    if (TmcDocumentWindow *d = activeDocument()) {
        MfcMessageMap map;
        d->view->TmcBuildMessageMap(map);
        if (map.call(commandId)) {
            updateCommandStates();
            d->widget->viewport()->update();
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
        // остаётся недоступным. Молчим, чтобы не пугать пользователя.
        break;
    }
}

void MainWindow::onNewDocument()
{
    QMessageBox::information(this, windowTitle(),
                             QStringLiteral("Создание нового задания появится вместе с "
                                            "диалогом характеристик."));
}

void MainWindow::onOpenDocument()
{
    // Фильтр — из строковой таблицы Windows-версии (IDR_TMCGROTYPE):
    // "*.dat;*.dop;*.$op". Второй пункт «все файлы» MFC добавляла сама.
    const QString path = QFileDialog::getOpenFileName(
        this, QStringLiteral("Open"), QString(),
        QStringLiteral("TMCRTOS (*.dat *.dop *.$op);;All Files (*.*)"));
    if (!path.isEmpty())
        openDocument(path);
}

void MainWindow::openDocument(const QString &path)
{
    TmcDocumentWindow *d = new TmcDocumentWindow;
    d->doc = CTMCGROUTDoc::TmcCreate();
    d->doc->SetPathName(path.toLocal8Bit().constData());
    d->doc->SetTitle(QFileInfo(path).fileName().toLocal8Bit().constData());

    d->view = CTMCGROUTView::TmcCreate();
    d->view->m_pDocument = d->doc;

    d->widget = new DnViewWidget(this);
    d->widget->setView(d->view);
    connect(d->widget, &MfcViewWidget::statusTextChanged, this, &MainWindow::onStatusText);

    d->sub = m_mdi->addSubWindow(d->widget);
    d->sub->setWindowTitle(QFileInfo(path).fileName());
    d->sub->setAttribute(Qt::WA_DeleteOnClose, false);
    d->widget->show();
    // В Windows-версии дочернее окно создаётся развёрнутым (стиль WS_MAXIMIZE
    // в CChildFrame::PreCreateWindow) — повторяем.
    d->sub->showMaximized();

    // Подложка области рисования — цвет фона из документа (в Windows его давало
    // само окно; документ хранит тот же цвет).
    d->doc->ReadGraphParametersDefault();
    d->doc->ReadGraphParameters();
    d->widget->setDocumentBackground(QColor(int(GetRValue(d->doc->scBackgoundColor)),
                                            int(GetGValue(d->doc->scBackgoundColor)),
                                            int(GetBValue(d->doc->scBackgoundColor))));

    // Вид читает документ и считает диаграмму — тот же порядок, что в MFC.
    // Сам расчёт идёт в отдельном потоке, поверх него показывается окно
    // прогресса (CThreadCalcDirPat); всё это внутри OnInitialUpdate вида.
    d->widget->initialUpdate();

    m_documents.append(d);
    updateMenuForDocument();
    // В строке состояния Windows-версия держит "Ready" — имя файла видно
    // в заголовке окна, повторять его не нужно.
}

void MainWindow::onCloseDocument()
{
    if (QMdiSubWindow *sub = m_mdi->activeSubWindow())
        sub->close();
}

void MainWindow::onAbout()
{
    tmcabout::show(this, QStringLiteral("About TMC_DN"),
                   QStringLiteral("TMC_DN"),
                   QStringLiteral("Просмотр диаграмм направленности "
                                  "пакета TMC Suite."));
}

void MainWindow::onSubWindowActivated(QMdiSubWindow *)
{
    updateMenuForDocument();
}

void MainWindow::updateMenuForDocument()
{
    // В Windows-версии при отсутствии документа показывалось короткое меню
    // (IDR_MAINFRAME). Здесь тот же смысл: команды документа недоступны.
    const bool hasDoc = activeDocument() != nullptr;
    for (QMap<UINT, QAction *>::iterator it = m_actions.begin(); it != m_actions.end(); ++it) {
        const UINT id = it.key();
        const bool always = (id == ID_FILE_NEW || id == ID_FILE_OPEN ||
                             id == ID_APP_EXIT || id == ID_APP_ABOUT);
        it.value()->setEnabled(always || hasDoc);
    }
}

void MainWindow::updateCommandStates()
{
    // Как в MFC перед показом меню: спрашиваем у вида активного окна, какие
    // команды сейчас доступны и какие включены, и переносим ответ на пункты
    // меню и кнопки панели.
    TmcDocumentWindow *d = activeDocument();
    tmccmd::updateActions(m_actions, d ? static_cast<CObject *>(d->view) : nullptr);
}
