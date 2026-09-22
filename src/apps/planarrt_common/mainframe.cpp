// mainframe.cpp — рамка главного окна (CMainFrame) для PlanarRT_H и PlanarRT_X.
//
// Почему файл не взят из win_src. MainFrm.cpp Windows-версии — это оконная
// обвязка MFC: создание панели инструментов и строки состояния, стыковка
// панелей, поиск активного дочернего окна MDI, чтение и запись положения окна.
// Всё это в Qt-версии делает сам интерфейс (см. mainwindow.cpp), а в слое
// совместимости соответствующих методов MFC (MDIGetActive, RecalcLayout,
// CMDIFrameWnd::OnClose) нет — они выписаны в src/apps/planarrt_h/NEEDS.md.
//
// Коду, перенесённому из Windows-версии, от рамки нужно ровно две вещи:
//   * строка состояния m_wndStatusBar — через неё работают PutTrace и
//     PutStatistics из PL_GLFUN.CPP;
//   * указатель на рамку в SetMainFramePointer__ — по нему расчётный узел
//     закрывает программу, досчитав последний шаг в пакетном режиме.
// Здесь определено только это. Ни одной формулы и ни одного формата файла.
//
// Диалог «Computation in progress. Stop and close?» при закрытии окна во время
// счёта перенесён в Qt-слой (MainWindow::closeEvent) — с тем же текстом и тем
// же порядком действий, что в CMainFrame::OnClose Windows-версии.

#include "resource.h"        // номера диалогов: их ждут заголовки Windows-версии
#include "mainfrm.h"
#include "planrt_hdoc.h"
#include "pl_iofor.h"

// Карта сообщений рамки. В Windows-версии в ней два пункта — показ и отметка
// строки состояния (ID_VIEW_STATUS_BAR); в Qt-версии этим распоряжается само
// главное окно (пункт View -> Status Bar).
//
// Остаётся одно сообщение: WM_CLOSE. Его шлёт расчётный узел, досчитав
// последний шаг в пакетном режиме (`HwndMainFrame->PostMessage( WM_CLOSE )` в
// TmcRTHRectNode.cpp). Обработчик записан прямо здесь, а не отдельным методом:
// добавить метод в CMainFrame нельзя — заголовок MainFrm.h принадлежит снимку
// Windows-версии и не правится.
BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
    tmcMap.addMessage(UINT(WM_CLOSE), [](WPARAM, LPARAM) -> LRESULT {
        TmcPostFrameClose(WM_CLOSE);
        return 0;
    });
END_MESSAGE_MAP()

CMainFrame::CMainFrame()
{
}

CMainFrame::~CMainFrame()
{
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
    // Стиль окна — тот же, что в Windows-версии: обычное окно с рамкой,
    // системным меню и кнопками свёртки/развёртки, открывается развёрнутым.
    cs.style = WS_OVERLAPPED | WS_CAPTION | FWS_ADDTOTITLE
             | WS_THICKFRAME | WS_SYSMENU | WS_MINIMIZEBOX | WS_MAXIMIZEBOX
             | WS_MAXIMIZE;

    return CMDIFrameWnd::PreCreateWindow(cs);
}

int CMainFrame::OnCreate(LPCREATESTRUCT /*lpCreateStruct*/)
{
    // Панель инструментов и строка состояния в Qt-версии создаются главным
    // окном (mainwindow.cpp). Остаётся то, ради чего код Windows-версии зовёт
    // OnCreate: запомнить рамку, чтобы расчётный узел мог её закрыть.
    SetMainFramePointer__(this);
    return 0;
}

void CMainFrame::OnDestroy()
{
    // Windows-версия сохраняла положение окна в реестре (в Qt-версии — в
    // ~/.config/TMC_Suite/<программа>.conf, тот же раздел и тот же ключ).
    // Формат строки прежний: left right top bottom.
    WINDOWPLACEMENT sWndPl;
    sWndPl.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(&sWndPl))
    {
        if (sWndPl.showCmd == SW_SHOWNORMAL)
        {
            char szBuffer[N_BUF_MAX];
            sprintf(szBuffer, "%ld %ld %ld %ld",
                    sWndPl.rcNormalPosition.left, sWndPl.rcNormalPosition.right,
                    sWndPl.rcNormalPosition.top, sWndPl.rcNormalPosition.bottom);
            AfxGetApp()->WriteProfileString(PLANRT_H_RAZDEL_INI,
                                            PLANRT_H_MNWNDSIZEPLACE_INI, szBuffer);
        };
    };

    SetMainFramePointer__(0);
    CMDIFrameWnd::OnDestroy();
}
