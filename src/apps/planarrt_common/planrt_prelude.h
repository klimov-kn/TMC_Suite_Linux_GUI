// planrt_prelude.h — добавка к преамбуле win_src для PlanarRT_H и PlanarRT_X.
//
// Зачем нужна. Оболочки счётных ядер зовут из MFC и win32 чуть больше, чем
// вьюверы: страницы свойств (мелодия оповещения), сведения о версии системы,
// атрибуты файла, дескрипторы потока. Общий слой совместимости
// (src/compat/include) этого пока не даёт, а править его в этой работе нельзя —
// перечень нехваток выписан в src/apps/planarrt_h/NEEDS.md.
//
// Поэтому недостающее объявлено здесь, и CMake подставляет этот файл (-include)
// сразу за общей преамбулой tmc_win_prelude.h — только для целей PlanarRT_H и
// PlanarRT_X. Сами исходники Windows-версии при этом не меняются.
//
// Ни одной формулы и ни одной константы расчёта здесь нет: только оконная и
// системная обвязка. Когда перечисленное появится в src/compat, этот файл
// сократится до нуля.

#pragma once

#ifndef _WIN32

// Общая преамбула win_src идёт первой: она даёт CString, win-типы и _complex.
// Включается здесь, а не отдельным -include, потому что CMake схлопывает два
// одинаковых ключа -include в один.
#include "tmc_win_prelude.h"

#include <climits>      // INT_MAX: MSVC давал его через свой <limits.h>
#include <cstdlib>
#include <string>
#include <vector>

#include "tmc_mfc_doc.h"
#include "mfc_dialog.h"
#include "mfc_view.h"
#include "mfc_thread.h"

// --- Документ -----------------------------------------------------------------
// Документ оболочки переопределяет два обработчика каркаса MFC, которых нет в
// CDocument слоя совместимости:
//   OnCloseDocument — закрытие документа (здесь останавливается счёт);
//   CanCloseFrame   — разрешение закрыть окно (запрещает закрытие во время
//                     чтения данных и счёта).
// Оба зовут реализацию базового класса. Чтобы не править ни слой совместимости,
// ни исходники Windows-версии, база подменяется: CDocument в этих двух
// программах — вот этот класс-прослойка, а он в свою очередь наследуется от
// CDocument слоя. Поведение MFC по умолчанию у обоих обработчиков пустое
// («да, можно») — оно и повторено.
class TmcPlanRtDocument : public CDocument
{
public:
    /// Закрытие документа. В MFC освобождала виды и содержимое; в Qt-версии
    /// этим распоряжается окно, поэтому здесь пусто.
    virtual void OnCloseDocument() {}

    /// Можно ли закрыть окно с этим документом. В MFC по умолчанию — да.
    virtual BOOL CanCloseFrame(CFrameWnd* /*pFrame*/) { return TRUE; }
};

#define CDocument TmcPlanRtDocument

// --- Вид ----------------------------------------------------------------------
// Вид оболочки объявляет обработчик OnClose (закрытие окна) и зовёт из него
// реализацию базового класса. В CScrollView слоя совместимости такого метода
// нет — закрытием окон распоряжается Qt. База подменяется так же, как у
// документа: пустая прослойка поверх CScrollView слоя.
//
// Внимание: заголовки общего слоя интерфейса (mfcviewwidget.h) объявляют
// setView(CScrollView*) — их надо включать при снятой подмене, иначе в
// объявлении окажется другой тип. Как это делается, показано в mainwindow.cpp.
class TmcPlanRtScrollView : public CScrollView
{
public:
    /// Закрытие окна вида. В MFC по умолчанию разрушало окно; в Qt-версии окно
    /// закрывает сам интерфейс, поэтому здесь пусто.
    virtual void OnClose() {}
};

#define CScrollView TmcPlanRtScrollView

// --- WIN.INI ------------------------------------------------------------------
// Настройки внешних просмотрщиков код сперва пробует прочитать из общесистемного
// WIN.INI (win32-функция GetProfileString с пятью аргументами). На современной
// Windows этот файл пуст и функция возвращает значение по умолчанию — третий
// аргумент. Под Linux WIN.INI нет вовсе, поэтому поведение то же: отдаём
// значение по умолчанию. Личные настройки программы читаются отдельно, через
// AfxGetApp()->GetProfileString (файл ~/.config/TMC_Suite/<программа>.conf).
DWORD TmcGetWinIniProfileString(const char* section, const char* entry,
                                const char* defaultValue, char* buffer, DWORD size);
inline DWORD GetProfileString(const char* section, const char* entry,
                              const char* defaultValue, char* buffer, DWORD size)
{
    return TmcGetWinIniProfileString(section, entry, defaultValue, buffer, size);
}

// --- Куча ---------------------------------------------------------------------
// _heapmin возвращал системе неиспользуемую память кучи. В glibc такой ручки
// нет (malloc_trim не входит в стандарт и не обязателен), а на результат счёта
// вызов не влияет — только на занятую память.
inline int _heapmin(void) { return 0; }

// --- Атрибуты файла -----------------------------------------------------------
// GetFileAttributes и FILE_ATTRIBUTE_ARCHIVE появились в общем слое
// (compat/include/tmc_mfc.h), здесь ничего объявлять не нужно.

// --- Сообщения окон -----------------------------------------------------------
// Из всех сообщений Windows коду нужно одно: WM_CLOSE. Значение как в win32 —
// оно нигде не пишется в файлы, но пусть совпадает.
#ifndef WM_CLOSE
  #define WM_CLOSE 0x0010
#endif

/// Закрыть главное окно программы. Расчётное ядро в пакетном режиме просит об
/// этом сообщением `pFrame->PostMessage( WM_CLOSE )`; обработчик сообщения
/// стоит в карте CMainFrame (planarrt_common/mainframe.cpp) и зовёт эту
/// функцию, а Qt-слой связывает её с закрытием окна.
void TmcPostFrameClose(UINT message);

// --- Версия системы -----------------------------------------------------------
// По ней Windows-версия выбирала способ подать звуковой сигнал: на NT — Beep,
// на 95/98 — прямая работа с портами. На Linux остаётся ветка NT (Beep), её и
// сообщаем.
#define VER_PLATFORM_WIN32s        0
#define VER_PLATFORM_WIN32_WINDOWS 1
#define VER_PLATFORM_WIN32_NT      2

struct OSVERSIONINFO
{
    DWORD dwOSVersionInfoSize;
    DWORD dwMajorVersion;
    DWORD dwMinorVersion;
    DWORD dwBuildNumber;
    DWORD dwPlatformId;
    char  szCSDVersion[128];
};

inline BOOL GetVersionEx(OSVERSIONINFO* info)
{
    if (!info)
        return FALSE;
    info->dwMajorVersion = 6;
    info->dwMinorVersion = 1;
    info->dwBuildNumber  = 0;
    info->dwPlatformId   = VER_PLATFORM_WIN32_NT;
    info->szCSDVersion[0] = '\0';
    return TRUE;
}

// --- Процессы и потоки --------------------------------------------------------
#ifndef DUPLICATE_SAME_ACCESS
  #define DUPLICATE_SAME_ACCESS 0x00000002
#endif

/// Время работы программы в миллисекундах (в Windows — GetTickCount).
/// Нужна в StopAndWait, чтобы не ждать поток вечно.
DWORD TmcGetTickCount(void);
inline DWORD GetTickCount(void) { return TmcGetTickCount(); }

/// Описатель текущего процесса. Дескрипторы потоков в слое совместимости —
/// указатели на объект потока, дублировать их не нужно, поэтому «процесс»
/// здесь — просто ненулевая метка.
inline HANDLE GetCurrentProcess(void) { return (HANDLE)-1; }

/// Копия дескриптора потока. В слое совместимости дескриптор — указатель на
/// объект потока, живущий до конца работы программы, поэтому копия — он сам.
inline BOOL DuplicateHandle(HANDLE /*srcProcess*/, HANDLE srcHandle,
                            HANDLE /*dstProcess*/, HANDLE* dstHandle,
                            DWORD /*access*/, BOOL /*inherit*/, DWORD /*options*/)
{
    if (!dstHandle)
        return FALSE;
    *dstHandle = srcHandle;
    return TRUE;
}

/// Снятие внешнего процесса (в Windows-версии — закрытие просмотрщика в
/// демонстрационном режиме). Демонстрационного режима в этой сборке нет
/// (_VERSION_PROF___), ветка недостижима.
inline BOOL TerminateProcess(HANDLE /*process*/, UINT /*exitCode*/) { return TRUE; }

// --- Окно с вопросом ----------------------------------------------------------
// В слое совместимости AfxMessageBox принимает только текст. Рамке окна нужен
// вариант с кнопками (Yes/No при закрытии во время счёта).
// Объявление AfxMessageBox с кнопками теперь в общем слое (mfc_dialog.h).
inline int AfxMessageBox(const CString& text, UINT type, UINT helpId = 0)
{
    return AfxMessageBox((const char*)text, type, helpId);
}

// --- Связь с Qt-слоем ---------------------------------------------------------
// Счётное ядро программы собирается без графики, поэтому окно с вопросом и
// закрытие главного окна оно просит через обработчики. Qt-слой ставит свои при
// старте (см. planarrt_common/mainwindow.cpp).
typedef int  (*TmcPlanRtMessageBoxFn)(const char* text, UINT type);
typedef void (*TmcPlanRtCloseFn)(UINT message);

void TmcPlanRtSetMessageBoxHandler(TmcPlanRtMessageBoxFn fn);
void TmcPlanRtSetCloseHandler(TmcPlanRtCloseFn fn);

// --- Страницы свойств ---------------------------------------------------------
// CPropertyPage и CPropertySheet (диалог мелодии оповещения) появились в общем
// слое (compat/include/mfc_dialog.h) — закладками, как в Windows. Здесь ничего
// объявлять не нужно.

#endif // !_WIN32
