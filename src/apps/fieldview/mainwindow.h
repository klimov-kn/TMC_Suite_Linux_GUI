// mainwindow.h — главное окно FieldView.
//
// Повторяет окно Windows-версии: те же пункты меню в том же порядке, та же
// панель из 35 кнопок, та же строка состояния, многодокументный режим (MDI).
// Область рисования — MfcGLWidget: контекст OpenGL создаёт Qt, а кадр рисует
// вид, перенесённый из Windows-версии без изменений.

#pragma once

#include <QMainWindow>
#include <QMap>

#include "mfc_dialog.h"

QT_BEGIN_NAMESPACE
class QAction;
class QMdiArea;
class QMdiSubWindow;
class QLabel;
class QMenu;
class QTimer;
QT_END_NAMESPACE

class MfcGLWidget;
class CFldViewDoc;
class CFldViewView;
class CMainFrame;

/// Одно окно документа: документ, вид и виджет-хозяин живут вместе.
struct FieldDocumentWindow
{
    CFldViewDoc  *doc = nullptr;
    CFldViewView *view = nullptr;
    MfcGLWidget  *widget = nullptr;
    QMdiSubWindow *sub = nullptr;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    /// Открыть файл поля (.tf, .AMP, .FAZ, .ex).
    void openDocument(const QString &path);

    /// Выполнить команду по её числовому идентификатору из ресурсов —
    /// то же, что нажать пункт меню. Нужно средству снимка.
    void runCommand(UINT commandId) { dispatch(commandId); }

    /// Текущий вид. Нужен средствам проверки: они печатают его в файл, минуя
    /// диалог печати. В самой программе не используется.
    CFldViewView *currentView() const;

    /// Показать текст в строке состояния. Зовётся хозяином строки состояния,
    /// через которого работают PutTrace и PutStatistics из кода Windows-версии.
    void putStatusText(int pane, const QString &text);

private slots:
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
    FieldDocumentWindow *activeDocument() const;

    QMdiArea            *m_mdi = nullptr;
    QMap<UINT, QAction*> m_actions;
    QList<FieldDocumentWindow*> m_documents;
    QLabel              *m_statusMain = nullptr;
    QLabel              *m_statusExtra = nullptr;
    CMainFrame          *m_frame = nullptr;   // строка состояния для кода из win_src
    class MfcQtHost     *m_statusHost = nullptr;  // хозяин этой строки состояния
    QMenu               *m_windowMenu = nullptr;
    QTimer              *m_uiTimer = nullptr;   // обновление состояния команд (как «на простое» в MFC)
};
