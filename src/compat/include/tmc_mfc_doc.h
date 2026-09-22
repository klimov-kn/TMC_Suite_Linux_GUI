// tmc_mfc_doc.h — слой совместимости для классов документа MFC.
//
// Классы «документа» в TMC (CTMCGROUTDoc и родня) хранят модель данных и читают
// и пишут файлы задания. Это самая чувствительная часть: формат файлов должен
// совпадать с Windows-версией байт в байт. Поэтому код документа НЕ переписан на
// Qt — он компилируется из исходников Windows-версии как есть, а MFC-обвязка,
// которой он касается, заменена здесь.
//
// Из MFC документу нужно немного: базовый класс CDocument с путём и флагом
// изменения, макросы карт сообщений (пустые), COLORREF/RGB, LOGFONT и три-четыре
// функции Windows API. Всё это ниже.
//
// Границы: здесь нет ни одной формулы. Единственное отличие поведения от Windows
// оговорено на месте — настройки графиков (цвета, типы линий) на Windows лежали
// в реестре, на Linux кладутся в файл ~/.config/TMC_Suite/<программа>.conf.

#pragma once

#ifndef _WIN32

#include "tmc_mfc.h"

#include <cstdio>
#include <cstring>
#include <string>

// --- Цвета ------------------------------------------------------------------
// В Windows COLORREF это 0x00BBGGRR. Порядок байтов важен: значения цветов
// попадают в файлы задания числами, и Windows-версия должна прочитать их так же.
typedef unsigned long COLORREF;

#ifndef RGB
  #define RGB(r, g, b) ((COLORREF)(((BYTE)(r)) | (((BYTE)(g)) << 8) | (((BYTE)(b)) << 16)))
#endif
#ifndef GetRValue
  #define GetRValue(rgb) ((BYTE)((rgb) & 0xff))
  #define GetGValue(rgb) ((BYTE)((((unsigned long)(rgb)) >> 8) & 0xff))
  #define GetBValue(rgb) ((BYTE)((((unsigned long)(rgb)) >> 16) & 0xff))
#endif

// --- Стили линий (wingdi.h) --------------------------------------------------
// Номер стиля уходит в файл задания, поэтому значения обязаны совпадать с
// Windows: файл, записанный здесь, должна одинаково прочитать и та версия.
#define PS_SOLID        0
#define PS_DASH         1
#define PS_DOT          2
#define PS_DASHDOT      3
#define PS_DASHDOTDOT   4
#define PS_NULL         5
#define PS_INSIDEFRAME  6

// --- Шрифт ------------------------------------------------------------------
// Поля и константы те же, что в Windows: код документа сохраняет их в настройки.
#define LF_FACESIZE 32

#define ANSI_CHARSET         0
#define DEFAULT_CHARSET      1
// Японская кодировка знаков: FieldView ставит её в LOGFONT подписей осей
// (fldviewview.cpp, tmcdialogplace.cpp, tmcgltext.cpp) — так в оригинале.
#define SHIFTJIS_CHARSET   128
#define OUT_DEFAULT_PRECIS   0
// Точность вывода и отсечения: те же значения, что в wingdi.h. Заполняются
// поля LOGFONT в FieldView.
#define OUT_STROKE_PRECIS    3
#define CLIP_DEFAULT_PRECIS  0
#define CLIP_CHARACTER_PRECIS 1
#define DEFAULT_QUALITY      0
#define DRAFT_QUALITY        1
#define PROOF_QUALITY        2
#define DEFAULT_PITCH        0
#define FIXED_PITCH          1
#define VARIABLE_PITCH       2
#define FF_DONTCARE          0
#define FF_ROMAN            16
#define FF_SWISS            32
#define FF_MODERN           48
#define FF_SCRIPT           64
#define FF_DECORATIVE       80
#define FW_NORMAL          400
#define FW_BOLD            700
struct LOGFONT
{
    long  lfHeight;
    long  lfWidth;
    long  lfEscapement;
    long  lfOrientation;
    long  lfWeight;
    BYTE  lfItalic;
    BYTE  lfUnderline;
    BYTE  lfStrikeOut;
    BYTE  lfCharSet;
    BYTE  lfOutPrecision;
    BYTE  lfClipPrecision;
    BYTE  lfQuality;
    BYTE  lfPitchAndFamily;
    char  lfFaceName[LF_FACESIZE];
};

// --- Заглушки каркаса MFC ----------------------------------------------------
class MfcMessageMap;

class CObject
{
public:
    virtual ~CObject() {}
    // Карта сообщений MFC (см. mfc_dialog.h): макросы BEGIN_MESSAGE_MAP строят
    // таблицу «команда -> обработчик», и цепочка вызовов идёт к базовому классу.
    virtual void TmcBuildMessageMap(MfcMessageMap&) {}
};

// Сериализация MFC в TMC не используется: документы читаются и пишутся своим
// кодом через FILE*. Заглушка нужна только чтобы скомпилировался пустой
// Serialize(), поэтому IsStoring всегда сообщает «чтение».
class CArchive
{
public:
    BOOL IsStoring() const { return FALSE; }
    BOOL IsLoading() const { return TRUE; }
};

class CDumpContext {};

// --- Обновление состояния команды --------------------------------------------
// В MFC каркас перед показом меню и панели опрашивал обработчики
// ON_UPDATE_COMMAND_UI: они говорят, доступен ли пункт (Enable) и нажат ли он
// (SetCheck/SetRadio). Так кнопки-переключатели FieldView (Line, Surface,
// Dimension 2/3, типы плазмы, Proportionally) показывают включённый режим.
//
// Здесь роль пункта меню играет действие Qt (QAction). Слой совместимости
// заголовков Qt не включает, поэтому указатель хранится нетипизированным, а
// применяет решение обработчика слой интерфейса
// (src/gui_common/commandupdate.cpp): он вызывает обработчик и переносит
// записанное состояние в действие.
class CCmdUI
{
public:
    /// Идентификатор команды, для которой вызван обработчик.
    UINT  m_nID = 0;

    /// Действие Qt (QAction*), состояние которого меняется. Ставит слой
    /// интерфейса перед вызовом обработчика.
    void* m_pQtAction = 0;

    /// Доступность пункта.
    void Enable(BOOL b = TRUE) { m_bEnabled = b ? TRUE : FALSE; m_bEnableSet = TRUE; }

    /// Отметка «нажато» (0 — снята, 1 — стоит, 2 — неопределённое состояние).
    void SetCheck(int n = 1) { m_nCheck = n; m_bCheckSet = TRUE; }

    /// То же для пункта-переключателя из группы.
    void SetRadio(BOOL b = TRUE) { m_nCheck = b ? 1 : 0; m_bCheckSet = TRUE; m_bRadio = TRUE; }

    // Что записал обработчик. Читает слой интерфейса; если обработчик чего-то
    // не тронул, соответствующее свойство действия остаётся прежним.
    BOOL EnableWasSet() const { return m_bEnableSet; }
    BOOL Enabled()      const { return m_bEnabled; }
    BOOL CheckWasSet()  const { return m_bCheckSet; }
    int  Check()        const { return m_nCheck; }
    BOOL IsRadio()      const { return m_bRadio; }

private:
    BOOL m_bEnabled   = TRUE;
    BOOL m_bEnableSet = FALSE;
    int  m_nCheck     = 0;
    BOOL m_bCheckSet  = FALSE;
    BOOL m_bRadio     = FALSE;
};

// Макросы карт сообщений и динамического создания. В Qt их роль выполняют
// сигналы и слоты, поэтому здесь они ни во что не разворачиваются.
// В MFC документы и виды создавал каркас (DYNCREATE), поэтому их конструкторы
// объявлены защищёнными. Каркас теперь наш, Qt-слой, — макрос добавляет ему
// фабричный метод, не трогая сам класс. Уровень доступа восстанавливается на
// protected: именно так стоит по соглашению MFC (макрос идёт сразу за защищённым
// конструктором, дальше в заголовке идёт явное public:).
#define DECLARE_DYNCREATE(cls)     public: static cls* TmcCreate() { return new cls(); }     protected:
#define IMPLEMENT_DYNCREATE(cls, base)
#define DECLARE_MESSAGE_MAP()
#define BEGIN_MESSAGE_MAP(cls, base) void tmc_unused_message_map_##cls() {
#define END_MESSAGE_MAP() }
#define ON_COMMAND(id, fn)
#define ON_UPDATE_COMMAND_UI(id, fn)
#define ON_WM_TIMER()
#define afx_msg

// --- CDocument ---------------------------------------------------------------
// Ровно то, чем пользуется код документа TMC: путь к файлу, заголовок и флаг
// изменения. Всё остальное (окна, виды, печать) — забота Qt-слоя.
class CDocument : public CObject
{
public:
    CDocument() : m_bModified(FALSE) {}
    virtual ~CDocument() {}

    const char* GetPathName() const { return m_strPathName.c_str(); }
    void SetPathName(const char* path, BOOL /*bAddToMRU*/ = TRUE)
    {
        m_strPathName = path ? path : "";
    }

    const char* GetTitle() const { return m_strTitle.c_str(); }
    void SetTitle(const char* title) { m_strTitle = title ? title : ""; }

    virtual BOOL OnNewDocument() { return TRUE; }
    virtual void DeleteContents() {}
    /// Закрытие документа. В MFC каркас звал их перед закрытием окна: сначала
    /// спрашивал разрешение (идёт ли расчёт), потом освобождал данные.
    virtual BOOL CanCloseFrame(void* /*frame*/ = 0) { return TRUE; }
    virtual void OnCloseDocument() {}
    virtual void Serialize(CArchive&) {}

    void SetModifiedFlag(BOOL bModified = TRUE) { m_bModified = bModified; }
    BOOL IsModified() const { return m_bModified; }

    // В MFC это оповещение видов о необходимости перерисоваться. В Qt-версии
    // перерисовкой управляет виджет, поэтому здесь достаточно флага: Qt-слой
    // читает его после операции и обновляет вид.
    void UpdateAllViews(void* /*pSender*/, long /*lHint*/ = 0, CObject* /*pHint*/ = 0)
    {
        m_bViewsDirty = TRUE;
    }
    BOOL TakeViewsDirty()
    {
        BOOL d = m_bViewsDirty;
        m_bViewsDirty = FALSE;
        return d;
    }

    virtual void AssertValid() const {}
    virtual void Dump(CDumpContext&) const {}

protected:
    std::string m_strPathName;
    std::string m_strTitle;
    BOOL        m_bModified;
    BOOL        m_bViewsDirty = FALSE;
};

// --- Настройки программы -----------------------------------------------------
// На Windows это реестр (HKCU\Software\<компания>\<программа>). На Linux реестра
// нет, поэтому те же пары «раздел/ключ -> строка» лежат в ini-файле
// ~/.config/TMC_Suite/<программа>.conf. Содержимое строк не меняется, поэтому
// файлы задания и поведение программы остаются прежними.
class CWinApp
{
public:
    CString GetProfileString(const char* section, const char* entry,
                             const char* def = "");
    BOOL WriteProfileString(const char* section, const char* entry, const char* value);
    int  GetProfileInt(const char* section, const char* entry, int def);
    BOOL WriteProfileInt(const char* section, const char* entry, int value);

    // Имя программы задаёт Qt-слой при старте: от него зависит имя conf-файла.
    static void SetAppName(const char* name);
    static const char* GetAppName();

    // В MFC отсюда начиналась жизнь программы. В Qt-версии точка входа своя,
    // поэтому метод есть только ради компиляции классов приложения из win_src.
    virtual BOOL InitInstance() { return TRUE; }
    virtual ~CWinApp() {}

    // Главное окно. Код программ достаёт через него строку состояния
    // (см. PutTrace/PutStatistics), поэтому Qt-слой заполняет это поле при старте.
    class CWnd* m_pMainWnd = 0;
};

CWinApp* AfxGetApp();

// Сравнение времён записи файлов, как в Windows API: -1, 0 или 1.
int CompareFileTime(const FILETIME* a, const FILETIME* b);

// --- Небольшая часть Windows API --------------------------------------------
typedef void* HANDLE;
#define INVALID_HANDLE_VALUE ((HANDLE)-1)
#define GENERIC_READ    0x80000000u
#define FILE_SHARE_READ 0x00000001u
#define OPEN_EXISTING   3

struct PROCESS_INFORMATION
{
    HANDLE hProcess;
    HANDLE hThread;
    DWORD  dwProcessId;
    DWORD  dwThreadId;
};

struct STARTUPINFO
{
    DWORD cb;
    char* lpReserved;
    char* lpDesktop;
    char* lpTitle;
    DWORD dwX, dwY, dwXSize, dwYSize;
    DWORD dwXCountChars, dwYCountChars, dwFillAttribute, dwFlags;
    WORD  wShowWindow, cbReserved2;
    BYTE* lpReserved2;
    HANDLE hStdInput, hStdOutput, hStdError;
};

#ifndef ZeroMemory
  #define ZeroMemory(dest, len) std::memset((dest), 0, (len))
#endif

// Открывает файл только чтобы узнать время записи (см. LastWriteTime в коде
// документа). Возвращает описатель, который понимает наш GetFileTime.
HANDLE CreateFile(const char* fileName, DWORD access, DWORD shareMode,
                  void* security, DWORD creation, DWORD flags, HANDLE templ);
BOOL   GetFileTime(HANDLE h, FILETIME* creation, FILETIME* access, FILETIME* write);
BOOL   CloseHandle(HANDLE h);

// Перевод windows-имени программы в имя нашей.
//
// Код Windows-версии зовёт соседние программы пакета как «TMCROS.EXE»,
// «FLDVIEW.EXE» и т. д. Здесь имя переводится в наше и ищется рядом с текущей
// программой. Возвращает то, что будет запущено (или само имя, если ничего
// подходящего не нашлось). Вынесено наружу, чтобы это можно было проверить.
std::string TmcResolveProgramName(const char* windowsName);

// Запуск внешней программы (в TMC — текстового редактора для файла задания).
// Командная строка разбирается по пробелам, как это делает Windows.
BOOL CreateProcess(const char* appName, char* commandLine,
                   void* processAttrs, void* threadAttrs, BOOL inheritHandles,
                   DWORD creationFlags, void* environment, const char* currentDir,
                   STARTUPINFO* startupInfo, PROCESS_INFORMATION* processInfo);

#endif // !_WIN32
