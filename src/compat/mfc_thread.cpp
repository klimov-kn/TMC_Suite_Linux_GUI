// Реализация потоков и ожидания MFC поверх Qt (см. mfc_thread.h).
//
// Расчёт остаётся в отдельном потоке, как на Windows: та же функция, те же
// данные, тот же порядок вычислений. Меняется только способ создания потока и
// ожидания его завершения.

#ifndef _WIN32

#include "mfc_thread.h"

#include <QCoreApplication>
#include <QDeadlineTimer>
#include <QElapsedTimer>
#include <QThread>

#include <map>

#include <QtGlobal>
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
  #include <QApplication>
#endif

namespace {

// Поток, исполняющий функцию в стиле MFC.
class MfcThread : public QThread
{
public:
    MfcThread(AFX_THREADPROC proc, LPVOID param, CWinThread* owner)
        : m_proc(proc), m_param(param), m_owner(owner) {}

    void run() override
    {
        if (m_proc)
            m_result = m_proc(m_param);
        m_finished = true;
    }

    UINT result() const { return m_result; }
    bool finished() const { return m_finished; }

private:
    AFX_THREADPROC m_proc;
    LPVOID         m_param;
    CWinThread*    m_owner;
    UINT           m_result = 0;
    bool           m_finished = false;
};

// Дескриптор потока в Windows — непрозрачное число. Здесь роль дескриптора
// играет сам указатель на поток, а таблица позволяет по нему найти объект,
// когда код Windows-версии передаёт дескриптор в функции ожидания.
std::map<HANDLE, MfcThread*>& threadTable()
{
    static std::map<HANDLE, MfcThread*> t;
    return t;
}

MfcThread* threadFor(HANDLE h)
{
    std::map<HANDLE, MfcThread*>::const_iterator it = threadTable().find(h);
    return it == threadTable().end() ? 0 : it->second;
}

// Приоритет Windows -> приоритет Qt. Соответствие то же, что было в
// SetThreadPriority: расчёт идёт с тем же приоритетом, что задавала
// Windows-версия.
QThread::Priority qtPriority(int priority)
{
    if (priority <= THREAD_PRIORITY_LOWEST)            return QThread::LowestPriority;
    if (priority == THREAD_PRIORITY_BELOW_NORMAL)      return QThread::LowPriority;
    if (priority == THREAD_PRIORITY_ABOVE_NORMAL)      return QThread::HighPriority;
    if (priority >= THREAD_PRIORITY_HIGHEST)           return QThread::HighestPriority;
    return QThread::NormalPriority;
}

} // namespace

// --- CWinThread ---------------------------------------------------------------

CWinThread::CWinThread()
    : m_hThread(0), m_proc(0), m_param(0), m_exitCode(0),
      m_nPriority(THREAD_PRIORITY_NORMAL), m_impl(0)
{
}

CWinThread::~CWinThread()
{
    MfcThread* t = static_cast<MfcThread*>(m_impl);
    if (t) {
        threadTable().erase(m_hThread);
        if (t->isRunning())
            t->wait(5000);
        delete t;
    }
}

DWORD CWinThread::ResumeThread()
{
    MfcThread* t = static_cast<MfcThread*>(m_impl);
    if (!t)
        return DWORD(-1);
    // Приоритет задаётся вместе с запуском: у неработающего потока Qt его
    // принять не может. В Windows-версии поток создаётся приостановленным и
    // запускается отсюда, поэтому именно здесь приоритет и вступает в силу.
    if (!t->isRunning())
        t->start(qtPriority(m_nPriority));
    return 1;      // прежний счётчик приостановок, как в Windows
}

DWORD CWinThread::SuspendThread()
{
    // Приостановка выполняющегося потока средствами ОС в Qt не поддерживается и
    // на Linux считается небезопасной. Windows-версия ею не пользуется:
    // расчёт останавливается флагом в самом расчётном классе (Stop()).
    return 0;
}

BOOL CWinThread::SetThreadPriority(int priority)
{
    MfcThread* t = static_cast<MfcThread*>(m_impl);
    if (!t)
        return FALSE;
    // Запоминаем в любом случае: если поток ещё не запущен, приоритет будет
    // задан при запуске (Qt умеет менять приоритет только работающему потоку и
    // иначе пишет в журнал «thread is not running», ничего не меняя).
    m_nPriority = priority;
    if (t->isRunning())
        t->setPriority(qtPriority(priority));
    return TRUE;
}

CWinThread* AfxBeginThread(AFX_THREADPROC proc, LPVOID param,
                           int priority, UINT /*stackSize*/, DWORD createFlags,
                           void* /*securityAttrs*/)
{
    CWinThread* winThread = new CWinThread();
    MfcThread* t = new MfcThread(proc, param, winThread);

    winThread->m_proc = proc;
    winThread->m_param = param;
    winThread->m_impl = t;
    winThread->m_hThread = static_cast<HANDLE>(t);
    threadTable()[winThread->m_hThread] = t;

    winThread->SetThreadPriority(priority);

    // Как в MFC: с флагом CREATE_SUSPENDED поток ждёт явного ResumeThread().
    // Приоритет передаётся прямо в start(): задать его до запуска нельзя.
    if (!(createFlags & CREATE_SUSPENDED))
        t->start(qtPriority(priority));

    return winThread;
}

// --- Ожидание -----------------------------------------------------------------

DWORD WaitForSingleObject(HANDLE handle, DWORD milliseconds)
{
    MfcThread* t = threadFor(handle);
    if (!t)
        return WAIT_FAILED;
    const bool done = (milliseconds == INFINITE)
        ? t->wait()
        : t->wait(QDeadlineTimer(int(milliseconds)));
    return done ? WAIT_OBJECT_0 : WAIT_TIMEOUT;
}

DWORD MsgWaitForMultipleObjects(DWORD count, const HANDLE* handles, BOOL /*waitAll*/,
                                DWORD milliseconds, DWORD /*wakeMask*/)
{
    if (!count || !handles)
        return WAIT_FAILED;

    // Windows-версия ждёт поток и одновременно продолжает разбирать сообщения,
    // чтобы окно не «замерзало». Здесь то же самое: короткими шагами ждём поток
    // и отдаём время очереди событий Qt.
    QElapsedTimer timer;
    timer.start();

    for (;;) {
        for (DWORD i = 0; i < count; ++i) {
            MfcThread* t = threadFor(handles[i]);
            if (!t || !t->isRunning())
                return WAIT_OBJECT_0 + i;
        }

        if (QCoreApplication::instance())
            QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
        else
            QThread::msleep(20);

        if (milliseconds != INFINITE && timer.elapsed() >= qint64(milliseconds))
            return WAIT_TIMEOUT;
    }
}

BOOL PeekMessage(MSG* msg, HWND /*hwnd*/, UINT /*filterMin*/, UINT /*filterMax*/,
                 UINT /*removeFlag*/)
{
    // Событий Windows здесь нет: очередь принадлежит Qt. Разбираем накопившееся
    // и сообщаем, что сообщений больше нет, — цикл ожидания в коде Windows-версии
    // на этом переходит к следующей итерации.
    if (msg)
        *msg = MSG();
    if (QCoreApplication::instance())
        QCoreApplication::processEvents(QEventLoop::AllEvents, 10);
    return FALSE;
}

BOOL TranslateMessage(const MSG*)
{
    return TRUE;
}

LRESULT DispatchMessage(const MSG*)
{
    return 0;
}

BOOL GetExitCodeThread(HANDLE handle, DWORD* exitCode)
{
    MfcThread* t = threadFor(handle);
    if (!t)
        return FALSE;
    if (exitCode)
        *exitCode = t->isRunning() ? 259u /* STILL_ACTIVE */ : DWORD(t->result());
    return TRUE;
}

// --- Звук ---------------------------------------------------------------------

void Beep(DWORD /*frequencyHz*/, DWORD /*durationMs*/)
{
    // Windows-версия подаёт сигнал об окончании шага и всего расчёта тоном
    // заданной частоты. Ни Qt, ни звуковая подсистема Linux не дают простого
    // способа сыграть чистый тон без внешних зависимостей, поэтому подаём
    // системный сигнал: событие для пользователя то же — «расчёт закончился».
    if (QCoreApplication::instance())
        QApplication::beep();
}

#endif // !_WIN32
