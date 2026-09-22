// tmc_mfc.h — минимальный слой совместимости MFC/win для Qt-версии TMC Suite.
//
// MFC на Linux нет. Вычислительные библиотеки TMC (SFILE95, PREPR, TMCIndan,
// Expr/Inter, complex, TMCLibError) используют из MFC по сути только строковый
// тип CString и несколько win-типов (BOOL/TRUE/FALSE). Здесь — их минимальные
// эквиваленты, достаточные для сборки под g++.
//
// ВАЖНО:
//  * Это НЕ математика — только перенос типов и утилит. Формулы и числа
//    не затрагиваются.
//  * CString — ТРИВИАЛЬНО-КОПИРУЕМЫЙ тип с единственным членом char* (как MFC
//    CString, внутри которого один указатель на буфер). Это критично: исходники
//    передают CString в printf/Format через "%s" (varargs). На MSVC объект
//    кладётся на стек как один указатель, и "%s" читает именно его. Нетривиально
//    копируемый тип (обёртка над std::string) g++ передавал бы по скрытой ссылке,
//    и "%s" прочитал бы мусор. Инвариант закреплён static_assert ниже.
//  * Буферы CString выделяются malloc и не освобождаются (COW через
//    переаллокацию): освобождающий деструктор сделал бы тип нетривиальным.
//  * Сообщения об ошибках библиотеки шлют через AfxMessageBox. В GUI-версии
//    консольный вывод не годится, поэтому здесь точка расширения:
//    tmc_set_message_handler() ставит обработчик, который GUI подменяет на
//    QMessageBox. Библиотеки при этом остаются БЕЗ зависимости от Qt.
//
// Активен только вне Windows: на самой Windows используется настоящий MFC.

#pragma once

#ifndef _WIN32

#include <string>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstddef>
#include <cerrno>
#include <sys/types.h>
#include <sys/xattr.h>
#include <sys/stat.h>
#if defined(__APPLE__)
  #define ENOATTR_TMC ENOATTR
#else
  #define ENOATTR_TMC ENODATA
#endif
#include <type_traits>

// --- Базовые win-типы -------------------------------------------------------
typedef int            BOOL;
typedef char           TCHAR;
typedef const char*    LPCTSTR;
typedef char*          LPTSTR;
typedef const char*    LPCSTR;
typedef char*          LPSTR;
typedef unsigned int   UINT;
typedef void*          LPVOID;
typedef unsigned long  DWORD;
typedef unsigned short WORD;
typedef unsigned char  BYTE;
typedef unsigned long  ULONG;
// LONG — тот же знаковый 32/64-битный тип, что и long. Встречается в
// объявлениях GDI (описание растра, размеры), например в tmcgltext.h FieldView.
typedef long           LONG;
// Параметры и результат оконного сообщения. Нужны в подписях обработчиков
// (ON_MESSAGE) и методов CWnd::SendMessage/PostMessage; объявлены здесь,
// а не в mfc_dialog.h, потому что ими пользуется уже слой окон (mfc_view.h).
typedef unsigned long  WPARAM;
typedef long           LPARAM;
typedef long           LRESULT;
// Описатель окна. Настоящих окон Windows здесь нет: слой возвращает вместо
// него указатель на объект Qt, а код Windows-версии сверяет его только с нулём
// (см. CWnd::GetSafeHwnd).
typedef void*          HWND;
// Целое размером с указатель. В подписях обработчиков Windows-версии
// (например, OnTimer) стоит именно оно: если объявить обработчик с UINT,
// переопределения не получится и метод не будет вызываться никогда.
typedef unsigned long  UINT_PTR;
typedef long           INT_PTR;

#ifndef TRUE
#define TRUE  1
#endif
#ifndef FALSE
#define FALSE 0
#endif

// --- Сегментные MSVC-ключевые слова (наследие DOS и 16 бит) -----------------
// На win32/win64 MSVC они уже пустые макросы (см. нейтрализацию far/near/fortran
// в win_src/Include/Typedef.h под _NDP2). Здесь тот же приём для файлов,
// которые используют их, не подключая Typedef.h.
#ifndef far
#define far
#endif
#ifndef _far
#define _far
#endif
#ifndef near
#define near
#endif
#ifndef _near
#define _near
#endif
#ifndef huge
#define huge
#endif
#ifndef _huge
#define _huge
#endif
#ifndef fortran
#define fortran
#endif
#ifndef _fortran
#define _fortran
#endif
#ifndef _pascal
#define _pascal
#endif
#ifndef _cdecl
#define _cdecl
#endif

// --- FILETIME (win32-структура; на Linux нужна только как тип-носитель) -----
struct FILETIME
{
    DWORD dwLowDateTime;
    DWORD dwHighDateTime;
};

// --- Sleep (периферия, не математика) ---------------------------------------
#include <thread>
#include <chrono>
inline void Sleep(unsigned long ms)
{
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

// --- SetFileAttributes ------------------------------------------------------
// На Windows снимала read-only с выходных файлов. На содержимое и формат файла
// не влияет, поэтому под Linux — пустышка с успешным кодом.
#ifndef FILE_ATTRIBUTE_NORMAL
#define FILE_ATTRIBUTE_NORMAL 0x80
#endif
#ifndef FILE_ATTRIBUTE_ARCHIVE
#define FILE_ATTRIBUTE_ARCHIVE 0x20
#endif
#ifndef INVALID_FILE_ATTRIBUTES
#define INVALID_FILE_ATTRIBUTES 0xFFFFFFFFu
#endif
// --- Set/GetFileAttributes ---------------------------------------------------
// Признак «архивный» в пакете играет одну роль: FieldView помечает им кадр поля,
// который уже прочитал (CFldViewDoc::SetSynchronizationFlag), а счётное ядро по
// нему понимает, можно ли писать следующий (CTmcRTHRectNode::IsFieldFileRead).
//
// В Windows этот бит ставит САМА система при любой записи в файл, а снять его
// может только программа. Отсюда и всё поведение пакета: ядро записало кадр —
// бит снова взведён, и ядро не ждёт; FieldView прочитал кадр и снял бит — до
// следующей записи файл считается прочитанным.
//
// На Linux и macOS такого бита нет. Повторяем его смысл точно: когда программа
// снимает признак, запоминаем в расширенном атрибуте файла время его последнего
// изменения. Признак считается снятым, только пока файл с тех пор не менялся;
// любая новая запись — и он снова взведён, как в Windows.
//
// Если файловая система расширенных атрибутов не поддерживает (например, раздел
// Windows, подключённый в /mnt), признак хранить негде — считаем его взведённым.
// Тогда ядро не ждёт просмотрщика: иначе при включённой синхронизации оно
// простаивало бы по 25 секунд на каждом кадре.
inline bool& tmc_xattr_works()
{
    static bool works = true;
    return works;
}

// Время последнего изменения файла в виде строки (секунды и наносекунды).
inline std::string tmc_file_stamp(const char* fileName)
{
    struct stat st;
    if (::stat(fileName, &st) != 0)
        return std::string();
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%lld.%09ld",
                  (long long)st.st_mtime, (long)st.st_mtim.tv_nsec);
    return std::string(buf);
}

inline BOOL SetFileAttributes(const char* lpFileName, DWORD dwFileAttributes)
{
    if (!lpFileName || !lpFileName[0])
        return FALSE;

    int rc = 0;
    if (dwFileAttributes == FILE_ATTRIBUTE_ARCHIVE) {
        // «Взвести» признак — значит забыть отметку о прочтении.
#if defined(__APPLE__)
        rc = ::removexattr(lpFileName, "user.tmc_read_stamp", 0);
#else
        rc = ::removexattr(lpFileName, "user.tmc_read_stamp");
#endif
        if (rc != 0 && errno == ENOATTR_TMC)
            rc = 0;                      // и так не стоял
    } else {
        const std::string stamp = tmc_file_stamp(lpFileName);
        if (stamp.empty())
            return FALSE;
#if defined(__APPLE__)
        rc = ::setxattr(lpFileName, "user.tmc_read_stamp",
                        stamp.c_str(), stamp.size(), 0, 0);
#else
        rc = ::setxattr(lpFileName, "user.tmc_read_stamp",
                        stamp.c_str(), stamp.size(), 0);
#endif
    }

    if (rc != 0 && errno == ENOTSUP)
        tmc_xattr_works() = false;
    return TRUE;
}

inline DWORD GetFileAttributes(const char* fileName)
{
    if (!fileName || !fileName[0])
        return DWORD(INVALID_FILE_ATTRIBUTES);

    const std::string stamp = tmc_file_stamp(fileName);
    if (stamp.empty())
        return DWORD(INVALID_FILE_ATTRIBUTES);   // файла нет

    if (!tmc_xattr_works())
        return FILE_ATTRIBUTE_ARCHIVE;

    char saved[64];
#if defined(__APPLE__)
    const ssize_t n = ::getxattr(fileName, "user.tmc_read_stamp",
                                 saved, sizeof(saved), 0, 0);
#else
    const ssize_t n = ::getxattr(fileName, "user.tmc_read_stamp",
                                 saved, sizeof(saved));
#endif
    if (n < 0) {
        if (errno == ENOTSUP)
            tmc_xattr_works() = false;
        return FILE_ATTRIBUTE_ARCHIVE;           // отметки нет — признак взведён
    }

    // Отметка есть: признак снят, только если файл с тех пор не менялся.
    if (std::string(saved, size_t(n)) == stamp)
        return FILE_ATTRIBUTE_NORMAL;
    return FILE_ATTRIBUTE_ARCHIVE;
}

// --- Строковые функции Windows API ------------------------------------------
// lstrcpy/lstrlen — те же strcpy/strlen; в однобайтовой сборке Windows это
// буквально они. Зовутся при заполнении LOGFONT (tmcgltext.cpp,
// tmcdialogplace.cpp в FieldView).
inline char* lstrcpy(char* dst, const char* src) { return std::strcpy(dst, src ? src : ""); }
inline int   lstrlen(const char* s) { return s ? int(std::strlen(s)) : 0; }

// --- Сообщения из библиотек -------------------------------------------------
// Обработчик по умолчанию пишет в stderr (годится для прогонов из консоли).
// GUI ставит свой через tmc_set_message_handler и показывает QMessageBox.
typedef void (*TmcMessageHandler)(const char* text);

inline TmcMessageHandler& tmc_message_handler_slot()
{
    static TmcMessageHandler h = nullptr;
    return h;
}

inline void tmc_set_message_handler(TmcMessageHandler h)
{
    tmc_message_handler_slot() = h;
}

inline void AfxMessageBox(const char* msg)
{
    const char* text = msg ? msg : "";
    if (TmcMessageHandler h = tmc_message_handler_slot())
        h(text);
    else
        std::fprintf(stderr, "%s\n", text);
}

// --- CString (тривиально-копируемый эквивалент MFC; внутри один char*) ------
// Ни деструктора, ни пользовательских copy/move — иначе тип перестанет быть
// trivially_copyable и сломается передача в "%s" varargs (см. шапку файла).
class CString
{
public:
    CString() : m_p(tmc_cstr_empty()) {}
    CString(const char* s) : m_p(tmc_cstr_dup(s ? s : "")) {}

    CString& operator=(const char* s) { m_p = tmc_cstr_dup(s ? s : ""); return *this; }

    // MFC CString::Format — printf-стиль. Новый буфер, прежний остаётся.
    void Format(const char* fmt, ...)
    {
        va_list ap;  va_start(ap, fmt);
        va_list ap2; va_copy(ap2, ap);
        int n = std::vsnprintf(nullptr, 0, fmt, ap);
        va_end(ap);
        if (n < 0) { m_p = tmc_cstr_empty(); va_end(ap2); return; }
        char* buf = static_cast<char*>(std::malloc(static_cast<size_t>(n) + 1));
        std::vsnprintf(buf, static_cast<size_t>(n) + 1, fmt, ap2);
        va_end(ap2);
        m_p = buf;
    }

    int  GetLength() const { return static_cast<int>(std::strlen(m_p)); }
    BOOL IsEmpty()   const { return (m_p[0] == 0) ? TRUE : FALSE; }
    void Empty()           { m_p = tmc_cstr_empty(); }

    const char* GetString() const { return m_p; }
    operator const char*()  const { return m_p; }

    CString& operator+=(const char* s)    { m_p = tmc_cstr_cat(m_p, s ? s : ""); return *this; }
    CString& operator+=(const CString& o) { m_p = tmc_cstr_cat(m_p, o.m_p);      return *this; }

    friend CString operator+(const CString& a, const char* b)    { CString r(a); r += b; return r; }
    friend CString operator+(const char* a, const CString& b)    { CString r(a); r += b; return r; }
    friend CString operator+(const CString& a, const CString& b) { CString r(a); r += b; return r; }

    bool operator==(const CString& o) const { return std::strcmp(m_p, o.m_p) == 0; }
    bool operator!=(const CString& o) const { return std::strcmp(m_p, o.m_p) != 0; }
    bool operator==(const char* s) const { return std::strcmp(m_p, s ? s : "") == 0; }
    bool operator!=(const char* s) const { return std::strcmp(m_p, s ? s : "") != 0; }

private:
    static char* tmc_cstr_empty()
    {
        static char e[1] = { 0 };   // общий пустой буфер, на месте не пишется
        return e;
    }
    static char* tmc_cstr_dup(const char* s)
    {
        size_t n = std::strlen(s) + 1;
        char* p = static_cast<char*>(std::malloc(n));
        std::memcpy(p, s, n);
        return p;
    }
    static char* tmc_cstr_cat(const char* a, const char* b)
    {
        size_t na = std::strlen(a), nb = std::strlen(b);
        char* p = static_cast<char*>(std::malloc(na + nb + 1));
        std::memcpy(p, a, na);
        std::memcpy(p + na, b, nb + 1);   // +1: копируем и завершающий ноль
        return p;
    }

    char* m_p;   // единственный член -> объект ABI-эквивалентен указателю
};

static_assert(std::is_trivially_copyable<CString>::value,
              "CString must be trivially copyable (varargs %s relies on it)");

#endif // !_WIN32
