// mfc_thread.h — потоки и ожидание MFC поверх Qt.
//
// Счётные оболочки (PlanarRT_H и PlanarRT_X) запускают расчёт в отдельном
// потоке: `AfxBeginThread(RunAllGlobal, pDoc, ..., CREATE_SUSPENDED)`, затем
// `ResumeThread()`. Останов идёт через флаг в расчётном классе, а закрытие окна
// во время счёта ждёт поток, прокачивая очередь сообщений.
//
// Здесь эти же вызовы работают поверх Qt. Важное свойство: расчёт остаётся в
// СВОЁМ потоке, а интерфейс — в главном, ровно как на Windows. Порядок вычислений
// и их результат от этого не зависят: поток исполняет ту же функцию с теми же
// данными.
//
// Внимание: обработчики из расчётного потока (строка состояния, звук) должны
// доходить до интерфейса через очередь событий Qt, а не прямым вызовом виджета.
// Этим занимается Qt-слой; здесь — только сами потоки и ожидание.

#pragma once

#ifndef _WIN32

#include "mfc_dialog.h"

// --- Константы Windows --------------------------------------------------------
#define CREATE_SUSPENDED          0x00000004
#define THREAD_PRIORITY_IDLE          (-15)
#define THREAD_PRIORITY_LOWEST         (-2)
#define THREAD_PRIORITY_BELOW_NORMAL   (-1)
#define THREAD_PRIORITY_NORMAL           0
#define THREAD_PRIORITY_ABOVE_NORMAL     1
#define THREAD_PRIORITY_HIGHEST          2
#define THREAD_PRIORITY_TIME_CRITICAL   15

#define INFINITE        0xFFFFFFFFu
#define WAIT_OBJECT_0   0x00000000u
#define WAIT_TIMEOUT    0x00000102u
#define WAIT_FAILED     0xFFFFFFFFu
#define QS_ALLINPUT     0x04FFu

#define PM_NOREMOVE 0x0000
#define PM_REMOVE   0x0001

struct MSG
{
    HWND   hwnd;
    UINT   message;
    WPARAM wParam;
    LPARAM lParam;
    DWORD  time;
    POINT  pt;
};

// --- Поток --------------------------------------------------------------------
// Функция потока в MFC имеет вид `UINT Proc(LPVOID param)`.
typedef UINT (*AFX_THREADPROC)(LPVOID param);

class CWinThread
{
public:
    CWinThread();
    virtual ~CWinThread();

    /// Снять с паузы поток, созданный с CREATE_SUSPENDED. Возвращает прежний
    /// счётчик приостановок, как в Windows.
    DWORD ResumeThread();
    DWORD SuspendThread();
    BOOL  SetThreadPriority(int priority);

    /// Дескриптор потока: код Windows-версии хранит его в документе и передаёт
    /// в функции ожидания.
    HANDLE m_hThread;

    /// Запущенная функция и её параметр (нужны при отложенном старте).
    AFX_THREADPROC m_proc;
    LPVOID         m_param;
    UINT           m_exitCode;

    /// Желаемый приоритет (THREAD_PRIORITY_*). Запоминается до старта:
    /// приоритет можно задать только запущенному потоку, а в Windows-версии
    /// поток создаётся приостановленным (CREATE_SUSPENDED) и запускается позже.
    /// Применяется при фактическом запуске — см. AfxBeginThread и ResumeThread.
    int            m_nPriority;

    // Внутренний объект потока Qt. Открыт наружу потому, что создаётся
    // фабрикой AfxBeginThread, а не самим классом (как в MFC).
    void* m_impl;
};

/// Полный аналог MFC: создаёт поток, при CREATE_SUSPENDED не запускает сразу.
CWinThread* AfxBeginThread(AFX_THREADPROC proc, LPVOID param,
                           int priority = THREAD_PRIORITY_NORMAL,
                           UINT stackSize = 0, DWORD createFlags = 0,
                           void* securityAttrs = 0);

// --- Ожидание -----------------------------------------------------------------
// Возвращают те же коды, что Windows: WAIT_OBJECT_0 — событие наступило,
// WAIT_TIMEOUT — вышло время.
DWORD WaitForSingleObject(HANDLE handle, DWORD milliseconds);

/// Ожидание потока с прокачкой очереди сообщений. В Windows-версии этим
/// закрывается окно во время расчёта: интерфейс продолжает откликаться.
/// Здесь очередь сообщений — это очередь событий Qt.
DWORD MsgWaitForMultipleObjects(DWORD count, const HANDLE* handles, BOOL waitAll,
                                DWORD milliseconds, DWORD wakeMask);

/// Прокачка очереди событий (аналог PeekMessage/TranslateMessage/DispatchMessage).
/// В Windows-версии эти три вызова идут подряд, поэтому здесь они сведены к
/// обработке накопившихся событий Qt.
BOOL PeekMessage(MSG* msg, HWND hwnd, UINT filterMin, UINT filterMax, UINT removeFlag);
BOOL TranslateMessage(const MSG* msg);
LRESULT DispatchMessage(const MSG* msg);

/// Код завершения потока (в Windows — GetExitCodeThread).
BOOL GetExitCodeThread(HANDLE handle, DWORD* exitCode);

/// Звуковой сигнал: в Windows-версии оповещает об окончании шага и всего счёта.
void Beep(DWORD frequencyHz, DWORD durationMs);

#endif // !_WIN32
