// mfc_dialog.h — диалоги, рамки и карты сообщений MFC поверх Qt.
//
// Диалоги TMC описаны двумя частями: разметка лежит в ресурсах `.rc`, а
// поведение — в коде класса (обмен данными DDX, обработчики кнопок). Обе части
// сохраняются как есть:
//   * разметка переводится в `.ui` скриптом tools/rc2ui.py — те же элементы,
//     те же подписи, то же расположение;
//   * код класса компилируется из Windows-версии поверх этого слоя.
//
// Карты сообщений MFC (BEGIN_MESSAGE_MAP / ON_BN_CLICKED) здесь не пустые:
// макросы собирают таблицу «идентификатор команды -> обработчик», и слой
// подключает к ней сигналы Qt-виджетов. Поэтому кнопки работают тем же кодом,
// что и на Windows.

#pragma once

#ifndef _WIN32

#include "mfc_view.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

// --- Типы Windows, встречающиеся в объявлениях -------------------------------
// WPARAM, LPARAM, LRESULT и HWND объявлены в tmc_mfc.h: ими пользуется уже
// слой окон (CWnd::SendMessage).
typedef void*          HBRUSH;
typedef void*          HCURSOR;
typedef void*          HICON;
typedef CREATESTRUCT*  LPCREATESTRUCT;

#define IDOK      1
#define IDCANCEL  2
#define IDABORT   3
#define IDRETRY   4
#define IDIGNORE  5
#define IDYES     6
#define IDNO      7

#define MB_OK                0x0000
#define MB_OKCANCEL          0x0001
#define MB_YESNO             0x0004
#define MB_ICONERROR         0x0010
#define MB_ICONQUESTION      0x0020
#define MB_ICONEXCLAMATION   0x0030
#define MB_ICONINFORMATION   0x0040

#define OFN_HIDEREADONLY     0x00000004
#define OFN_OVERWRITEPROMPT  0x00000002
#define OFN_FILEMUSTEXIST    0x00001000
#define OFN_ENABLESIZING     0x00800000
#define OFN_PATHMUSTEXIST    0x00000800

#define CF_SCREENFONTS       0x00000001
#define CF_PRINTERFONTS      0x00000002
#define CF_EFFECTS           0x00000100
#define CF_INITTOLOGFONTSTRUCT 0x00000040
#define CC_RGBINIT           0x00000001
#define CC_FULLOPEN          0x00000002

// Структуры общих диалогов Windows. Нужны потому, что код настраивает их поля
// напрямую (цвет текста в диалоге шрифта, начальный цвет в диалоге цвета).
struct OPENFILENAME
{
    DWORD       lStructSize;
    const char* lpstrTitle;
    const char* lpstrInitialDir;
    const char* lpstrFilter;
    char*       lpstrFile;
    DWORD       nMaxFile;
    DWORD       Flags;
};

struct CHOOSEFONT
{
    DWORD    lStructSize;
    LOGFONT* lpLogFont;
    COLORREF rgbColors;
    DWORD    Flags;
    int      iPointSize;
};

struct CHOOSECOLOR
{
    DWORD     lStructSize;
    COLORREF  rgbResult;
    COLORREF* lpCustColors;
    DWORD     Flags;
};

// Макрос строковых литералов MSVC: в однобайтовой сборке он ничего не делал.
#ifndef _T
  #define _T(x) x
#endif
#ifndef TEXT
  #define TEXT(x) x
#endif

// Сообщение «закройся»: в Windows его слал счётный узел рамке окна, чтобы
// программа завершилась, досчитав последний шаг в пакетном режиме.
#define WM_CLOSE   0x0010
#define WM_DESTROY 0x0002

#define BST_UNCHECKED 0
#define BST_CHECKED   1

// Положение окна. Qt-слой окнами управляет сам, но код Windows-версии эту
// структуру заполняет и читает, поэтому она должна существовать.
struct WINDOWPLACEMENT
{
    unsigned int length;
    unsigned int flags;
    unsigned int showCmd;
    struct { long x, y; } ptMinPosition;
    struct { long x, y; } ptMaxPosition;
    struct { long left, top, right, bottom; } rcNormalPosition;
};

// --- Карта сообщений ----------------------------------------------------------
// Три таблицы, как три вида записей в карте сообщений MFC:
//   * команда -> обработчик (ON_COMMAND, ON_BN_CLICKED и родня);
//   * команда -> обработчик обновления состояния (ON_UPDATE_COMMAND_UI):
//     доступен ли пункт и нажат ли он;
//   * сообщение -> обработчик (ON_MESSAGE): собственные сообщения программы.
class MfcMessageMap
{
public:
    typedef std::function<void()> Handler;
    typedef std::function<void(CCmdUI*)> UpdateHandler;
    typedef std::function<LRESULT(WPARAM, LPARAM)> MessageHandler;

    void add(UINT id, Handler handler) { m_handlers[id] = handler; }
    bool call(UINT id) const
    {
        std::map<UINT, Handler>::const_iterator it = m_handlers.find(id);
        if (it == m_handlers.end())
            return false;
        it->second();
        return true;
    }
    const std::map<UINT, Handler>& handlers() const { return m_handlers; }

    /// Обработчик обновления состояния команды.
    void addUpdate(UINT id, UpdateHandler handler) { m_updates[id] = handler; }
    bool callUpdate(UINT id, CCmdUI* pCmdUI) const
    {
        std::map<UINT, UpdateHandler>::const_iterator it = m_updates.find(id);
        if (it == m_updates.end())
            return false;
        it->second(pCmdUI);
        return true;
    }
    const std::map<UINT, UpdateHandler>& updateHandlers() const { return m_updates; }

    /// Обработчик оконного сообщения.
    void addMessage(UINT message, MessageHandler handler) { m_messages[message] = handler; }
    bool callMessage(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* result) const
    {
        std::map<UINT, MessageHandler>::const_iterator it = m_messages.find(message);
        if (it == m_messages.end())
            return false;
        const LRESULT r = it->second(wParam, lParam);
        if (result)
            *result = r;
        return true;
    }
    const std::map<UINT, MessageHandler>& messageHandlers() const { return m_messages; }

private:
    std::map<UINT, Handler>        m_handlers;
    std::map<UINT, UpdateHandler>  m_updates;
    std::map<UINT, MessageHandler> m_messages;
};

// Макросы карты сообщений. В MFC они строили таблицу указателей на методы;
// здесь собирают ту же таблицу из замыканий.
#undef DECLARE_MESSAGE_MAP
#undef BEGIN_MESSAGE_MAP
#undef END_MESSAGE_MAP
#undef ON_COMMAND
#undef ON_UPDATE_COMMAND_UI
#undef ON_WM_TIMER

#define DECLARE_MESSAGE_MAP() \
    public: virtual void TmcBuildMessageMap(MfcMessageMap& tmcMap);

#define BEGIN_MESSAGE_MAP(cls, base) \
    void cls::TmcBuildMessageMap(MfcMessageMap& tmcMap) \
    { \
        base::TmcBuildMessageMap(tmcMap);

#define END_MESSAGE_MAP() \
    }

#define ON_COMMAND(id, fn)      tmcMap.add(id, [this]{ this->fn(); });
#define ON_BN_CLICKED(id, fn)   tmcMap.add(id, [this]{ this->fn(); });
#define ON_STN_CLICKED(id, fn)  tmcMap.add(id, [this]{ this->fn(); });
#define ON_EN_CHANGE(id, fn)    tmcMap.add(id, [this]{ this->fn(); });
#define ON_CBN_SELCHANGE(id, fn) tmcMap.add(id, [this]{ this->fn(); });
#define ON_LBN_SELCHANGE(id, fn) tmcMap.add(id, [this]{ this->fn(); });
#define ON_COMMAND_RANGE(first, last, fn)

// Обновление состояния пункта меню или кнопки панели: обработчик получает
// CCmdUI и говорит, доступен ли пункт и нажат ли он. Слой интерфейса зовёт эти
// обработчики перед показом меню (см. src/gui_common/commandupdate.h).
#define ON_UPDATE_COMMAND_UI(id, fn) \
    tmcMap.addUpdate(id, [this](CCmdUI* pCmdUI){ this->fn(pCmdUI); });
#define ON_UPDATE_COMMAND_UI_RANGE(first, last, fn)

// Собственное сообщение программы (WM_USER + N). Обработчик имеет вид
// LRESULT fn(WPARAM, LPARAM), как в MFC.
#define ON_MESSAGE(msg, fn) \
    tmcMap.addMessage(UINT(msg), [this](WPARAM tmcWp, LPARAM tmcLp) -> LRESULT { \
        return this->fn(tmcWp, tmcLp); });

#define ON_NOTIFY(code, id, fn)

// Оконные события приходят виду и диалогу через виртуальные методы (их зовёт
// Qt-хозяин), поэтому регистрировать здесь нечего.
#define ON_WM_PAINT()
#define ON_WM_TIMER()
#define ON_WM_CREATE()
#define ON_WM_DESTROY()
#define ON_WM_SIZE()
#define ON_WM_ERASEBKGND()
#define ON_WM_CTLCOLOR()
#define ON_WM_SETCURSOR()
#define ON_WM_LBUTTONDOWN()
#define ON_WM_LBUTTONUP()
#define ON_WM_LBUTTONDBLCLK()
#define ON_WM_RBUTTONDOWN()
#define ON_WM_RBUTTONUP()
#define ON_WM_MOUSEMOVE()
#define ON_WM_MOUSEWHEEL()
#define ON_WM_KEYDOWN()
#define ON_WM_HSCROLL()
#define ON_WM_VSCROLL()
#define ON_WM_CLOSE()

// Окно сообщения с кнопками. Вариант без типа кнопок объявлен в tmc_mfc.h и
// пишет в поток ошибок; здесь — полноценный, с ответом IDYES/IDNO/IDOK.
int AfxMessageBox(const char* text, UINT type, UINT helpId = 0);

// --- Обмен данными с элементами (DDX/DDV) ------------------------------------
class CDialog;

class CDataExchange
{
public:
    CDataExchange(CDialog* dlg, BOOL saveAndValidate)
        : m_pDlgWnd(dlg), m_bSaveAndValidate(saveAndValidate) {}

    CDialog* m_pDlgWnd;
    BOOL     m_bSaveAndValidate;
    void     Fail() {}
};

void DDX_Text(CDataExchange* pDX, int id, CString& value);
void DDX_Text(CDataExchange* pDX, int id, char* value, int maxLen);
void DDX_Text(CDataExchange* pDX, int id, int& value);
void DDX_Text(CDataExchange* pDX, int id, UINT& value);
void DDX_Text(CDataExchange* pDX, int id, long& value);
void DDX_Text(CDataExchange* pDX, int id, float& value);
void DDX_Text(CDataExchange* pDX, int id, double& value);
// BOOL в этом слое — это int, поэтому одной перегрузки достаточно.
void DDX_Check(CDataExchange* pDX, int id, int& value);
void DDX_Radio(CDataExchange* pDX, int id, int& value);
void DDX_CBIndex(CDataExchange* pDX, int id, int& index);
void DDX_LBIndex(CDataExchange* pDX, int id, int& index);

// Проверки. В MFC они показывали окно с сообщением; здесь просто ограничивают
// значение, чтобы не мешать работе диалога всплывающими окнами на каждое поле.
void DDV_MaxChars(CDataExchange* pDX, CString& value, int maxChars);
void DDV_MaxChars(CDataExchange* pDX, char* value, int maxChars);
void DDV_MinMaxInt(CDataExchange* pDX, int value, int minVal, int maxVal);
void DDV_MinMaxLong(CDataExchange* pDX, long value, long minVal, long maxVal);
void DDV_MinMaxDouble(CDataExchange* pDX, double value, double minVal, double maxVal);
void DDV_MinMaxFloat(CDataExchange* pDX, float value, float minVal, float maxVal);

// --- Обёртки элементов управления --------------------------------------------
// Возвращаются из GetDlgItem. Внутри — указатель на виджет Qt.
class CWndCtrl : public CWnd
{
public:
    explicit CWndCtrl(void* widget = 0) : m_widget(widget) {}

    void  SetWindowText(const char* text);
    int   GetWindowText(char* buffer, int maxLen) const;
    CString GetWindowTextCS() const;
    void  EnableWindow(BOOL enable = TRUE);
    void  ShowWindow(int show);
    BOOL  IsWindowEnabled() const;
    void  SetFocus();
    void* widget() const { return m_widget; }
    /// Привязать обёртку к виджету (используется в DDX_Control).
    void  attach(void* w) { m_widget = w; }

    CDC* GetDC() override;
    void ReleaseDC(CDC* dc) override;

protected:
    void* m_widget;
};

typedef CWndCtrl CStatic;
typedef CWndCtrl CEdit;

class CButton : public CWndCtrl
{
public:
    explicit CButton(void* widget = 0) : CWndCtrl(widget) {}
    int  GetCheck() const;
    void SetCheck(int check);
};

// Индикатор выполнения. В TMC_DN показывает ход расчёта диаграммы.
class CProgressCtrl : public CWndCtrl
{
public:
    explicit CProgressCtrl(void* widget = 0) : CWndCtrl(widget) {}
    void SetRange(short lower, short upper);
    void SetRange32(int lower, int upper);
    int  SetPos(int pos);
    int  SetStep(int step);
    int  StepIt();
    int  GetPos() const;

private:
    int m_step = 1;      // шаг для StepIt, как в MFC
};

class CComboBox : public CWndCtrl
{
public:
    explicit CComboBox(void* widget = 0) : CWndCtrl(widget) {}
    int  AddString(const char* text);
    int  GetCurSel() const;
    int  SetCurSel(int index);
    void ResetContent();
};

class CListBox : public CWndCtrl
{
public:
    explicit CListBox(void* widget = 0) : CWndCtrl(widget) {}
    int  AddString(const char* text);
    int  GetCurSel() const;
    int  SetCurSel(int index);
    void ResetContent();
};

// --- Набор картинок (CImageList) ----------------------------------------------
// В Windows это общий склад значков: список выдаёт из него картинку по номеру.
// Здесь значок — QIcon, а номер — место в списке. Пользуется им окно свойств
// FieldView (tmcdialogpropet.h, m_ImageList1).
#define ILC_MASK   0x0001
#define ILC_COLOR  0x0000
#define ILC_COLOR4 0x0004
#define ILC_COLOR8 0x0008
#define ILC_COLOR24 0x0018
#define ILC_COLOR32 0x0020

class CImageList
{
public:
    CImageList();
    ~CImageList();

    /// Завести пустой набор картинок заданного размера.
    BOOL Create(int cx = 0, int cy = 0, UINT flags = 0, int initial = 0, int grow = 0);
    /// Вариант MFC «из растра ресурса». Растров Windows здесь нет, поэтому
    /// набор заводится пустым, а картинки добавляются AddIcon.
    BOOL Create(UINT bitmapId, int cx, int grow, COLORREF mask);

    /// Добавить значок из ресурсов Qt (например ":/icons/ID_FILE_OPEN.png").
    /// Возвращает номер картинки в наборе или -1.
    int  AddIcon(const char* resourceName);

    /// Подпись MFC (растр + цвет прозрачности). Растра под Linux нет: место в
    /// наборе занимается, картинки у него не будет.
    int  Add(void* bitmap, COLORREF mask = 0);

    BOOL DeleteImageList();
    int  GetImageCount() const;

    /// Значок по номеру (QIcon*) или 0. Зовёт список при выводе строки.
    void* IconAt(int index) const;
    void* GetSafeHandle() const { return m_icons; }

private:
    CImageList(const CImageList&);              // копирование не нужно
    CImageList& operator=(const CImageList&);

    void* m_icons;   // QList<QIcon>*
    int   m_cx;
    int   m_cy;
};

// --- Список с колонками (CListCtrl) -------------------------------------------
// В Windows это SysListView32 в режиме отчёта: колонки и строки. В Qt тому же
// соответствует QTreeWidget (список с колонками), его и оборачиваем.
#define LVSIL_NORMAL 0
#define LVSIL_SMALL  1
#define LVSIL_STATE  2

#define LVCFMT_LEFT   0
#define LVCFMT_RIGHT  1
#define LVCFMT_CENTER 2

#define LVS_EX_FULLROWSELECT 0x00000020
#define LVS_EX_GRIDLINES     0x00000001

class CListCtrl : public CWndCtrl
{
public:
    explicit CListCtrl(void* widget = 0) : CWndCtrl(widget), m_images(0), m_exStyle(0) {}

    /// Добавить колонку (номер, заголовок, выравнивание, ширина в пикселях).
    int  InsertColumn(int col, const char* title, int format = LVCFMT_LEFT,
                      int width = -1, int subItem = -1);
    int  GetColumnCount() const;

    /// Вставить строку; вариант с картинкой берёт её из набора (SetImageList).
    int  InsertItem(int item, const char* text);
    int  InsertItem(int item, const char* text, int image);

    BOOL    SetItemText(int item, int subItem, const char* text);
    CString GetItemText(int item, int subItem) const;

    BOOL DeleteItem(int item);
    BOOL DeleteAllItems();
    int  GetItemCount() const;

    /// Номер выделенной строки (-1, если не выделено), как в MFC.
    int  GetSelectionMark() const;
    void SetSelectionMark(int item);

    /// Набор картинок для строк. Тип (LVSIL_*) в Qt значения не имеет: размер
    /// значка задаёт сам виджет.
    void SetImageList(CImageList* list, int type = LVSIL_SMALL);

    /// Дополнительные стили списка. В Qt им соответствуют свойства виджета:
    /// LVS_EX_FULLROWSELECT — выделение строкой целиком, LVS_EX_GRIDLINES —
    /// разделители. Возвращает прежний набор, как в MFC.
    DWORD SetExtendedStyle(DWORD style);
    DWORD GetExtendedStyle() const { return m_exStyle; }

private:
    CImageList* m_images;
    DWORD       m_exStyle;
};

// Привязка переменной-обёртки к элементу (в MFC — DDX_Control). Объявлена
// после самих обёрток: раньше типа ещё не существует.
void DDX_Control(CDataExchange* pDX, int id, CWndCtrl& widget);

// --- Диалог -------------------------------------------------------------------
// Разметку берём из .ui, полученного из того же .rc, что и на Windows.
class CDialog : public CWnd
{
public:
    explicit CDialog(UINT idd = 0, CWnd* parent = 0);
    virtual ~CDialog();

    virtual INT_PTR DoModal();

    /// Немодальное окно, как `CDialog::Create` в MFC: окно открывается и
    /// остаётся на экране, а программа продолжает работать. Так открыто окно
    /// уровней цвета FieldView (colorleveldlg.cpp) — при нём можно вращать
    /// сцену. Окно живёт до DestroyWindow.
    virtual BOOL Create(UINT idd, CWnd* parent = 0);
    virtual BOOL Create(const char* templateName, CWnd* parent = 0);

    /// Закрытие окна пользователем (крестик). В MFC приходило сообщением
    /// WM_CLOSE; здесь зовётся из обработчика закрытия окна Qt. По умолчанию,
    /// как в MFC, равносильно отмене.
    virtual void OnClose();

    /// Разрушить окно. Немодальное окно после этого «не существует»:
    /// GetSafeHwnd() снова возвращает ноль, и код Windows-версии создаёт его
    /// заново (fldviewview.cpp, OnFieldL).
    BOOL DestroyWindow() override;

    HWND GetSafeHwnd() const override { return static_cast<HWND>(m_qtDialog); }

    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX);
    virtual void OnOK();
    virtual void OnCancel();
    /// Рисование поверх диалога. В Windows приходило сообщением WM_PAINT;
    /// диалоги TMC так рисуют образцы цвета и линий прямо в статических полях
    /// (GetDlgItem()->GetDC() -> FillSolidRect). Зовёт Qt-слой при перерисовке.
    virtual void OnPaint() {}
    // Уведомления от элементов (в MFC — WM_NOTIFY). В Qt-версии не приходят,
    // но метод должен существовать: код диалогов его переопределяет.
    virtual BOOL OnNotify(WPARAM, LPARAM, LRESULT*) { return FALSE; }

    BOOL UpdateData(BOOL saveAndValidate = TRUE);
    void EndDialog(int result);

    CWndCtrl* GetDlgItem(UINT id) const;
    void      SetDlgItemText(UINT id, const char* text);
    int       GetDlgItemText(UINT id, char* buffer, int maxLen) const;
    void      SetDlgItemInt(UINT id, int value, BOOL signed_ = TRUE);
    UINT      GetDlgItemInt(UINT id, BOOL* translated = 0, BOOL signed_ = TRUE) const;
    void      CheckRadioButton(UINT first, UINT last, UINT check);
    void      CheckDlgButton(UINT id, UINT check);
    UINT      IsDlgButtonChecked(UINT id) const;
    void      SetWindowText(const char* text);

    virtual void TmcBuildMessageMap(MfcMessageMap& tmcMap) { (void)tmcMap; }

    // Таймер диалога. В TMC им обновляется окно прогресса расчёта: каждые
    // 100 мс диалог опрашивает счётчик и двигает индикатор. У диалога нет
    // виджета-хозяина, поэтому таймер живёт прямо здесь, на самом окне Qt.
    UINT SetTimer(UINT id, UINT elapseMs, void* proc = 0);
    BOOL KillTimer(UINT id);
    virtual void OnTimer(UINT_PTR id) { (void)id; }

    UINT m_nIDD;        // идентификатор ресурса диалога

    // --- Служебное для набора страниц свойств --------------------------------
    // Страница свойств живёт не своим окном, а вкладкой внутри чужого. Поэтому
    // набор создаёт окно сам и просит страницу построиться в нём.
    /// Построить разметку и подключить обработчики в готовом окне Qt (QDialog*).
    void TmcBuildIn(void* qDialog);
    /// Забыть окно Qt, не разрушая его (окно разрушит тот, кто его создал).
    void TmcDetach();

protected:
    void* m_qtDialog;   // QDialog*: создаётся в DoModal/Create либо даётся извне
    BOOL  m_qtDialogOwned = FALSE;    // окно создано нами и нам его разрушать
    std::map<UINT, void*> m_timers;   // идентификатор -> QTimer*
    CWnd* m_parent;
    mutable std::map<UINT, CWndCtrl*> m_ctrlCache;
};

// --- Страница свойств ---------------------------------------------------------
// Окно «Parameters» FieldView (View -> Parameters, Alt+P) в Windows состоит из
// четырёх закладок: Place, Topology, Eps, Field. Каждая закладка — обычный
// диалог со своей разметкой из ресурсов, поэтому страница здесь наследуется от
// CDialog: обмен данными (DDX) и обработчики кнопок работают без правок.
class CPropertyPage : public CDialog
{
public:
    explicit CPropertyPage(UINT nIDTemplate = 0, UINT nIDCaption = 0)
        : CDialog(nIDTemplate, 0), m_nCaptionID(nIDCaption) {}

    /// Пометить страницу изменённой. В MFC включает кнопку «Применить»; здесь
    /// набор включает её так же.
    void SetModified(BOOL changed = TRUE);
    BOOL IsModified() const { return m_bModified; }

    /// Страницу показали / убрали. В MFC возврат FALSE у OnKillActive не даёт
    /// уйти со страницы (проверка введённых значений).
    virtual BOOL OnSetActive() { return TRUE; }
    virtual BOOL OnKillActive() { return TRUE; }

    /// Нажата кнопка «Применить» (и «OK»). Страницы FieldView отсюда шлют виду
    /// сообщение WM_USERAPPLY_*, по которому он забирает новые значения.
    virtual BOOL OnApply() { return TRUE; }
    virtual void OnReset() {}
    virtual BOOL OnCommand(WPARAM /*wParam*/, LPARAM /*lParam*/) { return TRUE; }

    UINT    m_nCaptionID;   // подпись закладки из строковой таблицы
    CString m_strCaption;   // подпись закладки строкой

private:
    friend class CPropertySheet;
    BOOL m_bModified = FALSE;
    class CPropertySheet* m_sheet = 0;
};

// --- Набор страниц свойств ----------------------------------------------------
// Окно с закладками: сверху корешки, снизу кнопки OK, Cancel, Apply — как в
// Windows. В Qt закладки даёт QTabWidget, а каждая страница остаётся своим
// диалогом и просто становится содержимым вкладки.
class CPropertySheet : public CWnd
{
public:
    explicit CPropertySheet(UINT nIDCaption = 0, CWnd* parent = 0, UINT iSelectPage = 0);
    explicit CPropertySheet(const char* pszCaption, CWnd* parent = 0, UINT iSelectPage = 0);
    virtual ~CPropertySheet();

    void AddPage(CPropertyPage* page);
    void RemovePage(CPropertyPage* page);

    int  GetPageCount() const { return int(m_pages.size()); }
    CPropertyPage* GetPage(int index) const;
    CPropertyPage* GetActivePage() const;
    int  GetActiveIndex() const { return m_active; }
    BOOL SetActivePage(int index);

    /// Показать окно с закладками. Возвращает IDOK или IDCANCEL, как в MFC.
    virtual INT_PTR DoModal();

    virtual BOOL OnInitDialog();
    virtual void DoDataExchange(CDataExchange* pDX) { (void)pDX; }
    virtual void OnClose() {}

    void SetTitle(const char* title, DWORD style = 0);
    const char* GetTitle() const { return m_caption.c_str(); }

protected:
    /// Кнопка «Применить»: собрать значения со всех страниц и дать им
    /// применить их (в MFC — OnApply у изменённых страниц).
    void ApplyPages();
    /// Доступность кнопки «Применить» (зовёт CPropertyPage::SetModified).
    void UpdateApplyButton();

    std::vector<CPropertyPage*> m_pages;
    std::string m_caption;
    int         m_active;
    void*       m_qtSheet;    // QDialog* окна набора
    void*       m_qtTabs;     // QTabWidget*
    void*       m_qtApply;    // QPushButton* «Применить»

private:
    friend class CPropertyPage;
};

// --- Реестр диалогов ----------------------------------------------------------
// Заполняется кодом, который порождает tools/rc2ui.py: для каждого диалога —
// функция построения разметки, для каждого элемента — символьное имя по номеру.
class MfcDialogRegistry
{
public:
    typedef void (*SetupFn)(void* qDialog);   // строит разметку в QDialog

    static void registerDialog(UINT idd, SetupFn setup);
    static void registerControlName(UINT id, const char* name);

    static SetupFn setupFor(UINT idd);
    /// Номера всех зарегистрированных диалогов — для средств проверки, которые
    /// снимают вид каждого окна.
    static std::vector<UINT> registeredDialogs();
    static const char* controlName(UINT id);
};

// --- Стандартные диалоги ------------------------------------------------------
class CFileDialog
{
public:
    CFileDialog(BOOL openFileDialog, const char* defExt = 0, const char* fileName = 0,
                DWORD flags = 0, const char* filter = 0, CWnd* parent = 0);
    INT_PTR DoModal();
    CString GetPathName() const { return CString(m_path.c_str()); }
    CString GetFileName() const;

    OPENFILENAME m_ofn;   // код настраивает поля напрямую, как в MFC

private:
    BOOL        m_open;
    std::string m_defExt;
    std::string m_fileName;
    std::string m_filter;
    std::string m_path;
    CWnd*       m_parent;
};

class CColorDialog
{
public:
    explicit CColorDialog(COLORREF initial = 0, DWORD flags = 0, CWnd* parent = 0);
    INT_PTR  DoModal();
    COLORREF GetColor() const { return m_color; }

    CHOOSECOLOR m_cc;

private:
    COLORREF m_color;
    CWnd*    m_parent;
};

class CFontDialog
{
public:
    explicit CFontDialog(LOGFONT* initial = 0, DWORD flags = 0, void* dc = 0, CWnd* parent = 0);
    INT_PTR DoModal();
    void    GetCurrentFont(LOGFONT* lf) const;
    COLORREF GetColor() const { return m_cf.rgbColors; }

    LOGFONT     m_lf;
    CHOOSEFONT  m_cf;

private:
    CWnd* m_parent;
};

// --- Рамки окон ---------------------------------------------------------------
// Главное окно и дочерние окна в Qt-версии свои, поэтому здесь только то, что
// нужно для компиляции заголовков Windows-версии.
class CStatusBar : public CWnd
{
public:
    // В Windows перерисовка строки состояния касалась только её самой и стоила
    // копейки. У нас строка состояния — часть окна Qt и перерисовывается сама
    // при смене текста, а вызов перерисовки «хозяина» приводил бы к полной
    // перерисовке ВИДА документа. Счётное ядро зовёт PutTrace на каждом такте,
    // поэтому такая перерисовка замедляла счёт в десятки раз.
    void RedrawWindow(const RECT* = 0, void* = 0, UINT = 0) {}
    void UpdateWindow() {}
    void Invalidate(BOOL = TRUE) {}
    BOOL Create(CWnd*) { return TRUE; }
    BOOL SetIndicators(const UINT*, int) { return TRUE; }
    void SetPaneText(int pane, const char* text)
    {
        if (m_host)
            m_host->hostSetStatusText(pane, text);
    }
};

// Стиль панели инструментов: кнопки без рамок, рамка появляется под указателем.
// Так создана панель FieldView (mainfrm.cpp). В Qt это вид кнопок по умолчанию,
// поэтому константа нужна только для компиляции вызова.
#ifndef TBSTYLE_FLAT
  #define TBSTYLE_FLAT 0x0800
#endif

class CToolBar : public CWnd
{
public:
    BOOL Create(CWnd*, DWORD = 0, UINT = 0) { return TRUE; }
    BOOL CreateEx(CWnd*, DWORD = 0, DWORD = 0) { return TRUE; }
    BOOL LoadToolBar(UINT) { return TRUE; }
    void EnableDocking(DWORD) {}
    void SetBarStyle(DWORD) {}
    DWORD GetBarStyle() const { return 0; }
};

class CFrameWnd : public CWnd
{
public:
    virtual int OnCreate(LPCREATESTRUCT) { return 0; }
    virtual BOOL PreCreateWindow(CREATESTRUCT&) { return TRUE; }
    virtual void OnDestroy() {}
    void EnableDocking(DWORD) {}
    void DockControlBar(CToolBar*) {}
    // Положением окна в Qt-версии распоряжается сам Qt; здесь — заглушки,
    // сохраняющие последнее заданное значение.
    BOOL GetWindowPlacement(WINDOWPLACEMENT* wp) const
    {
        if (wp) *wp = m_placement;
        return TRUE;
    }
    BOOL SetWindowPlacement(const WINDOWPLACEMENT* wp)
    {
        if (wp) m_placement = *wp;
        return TRUE;
    }
    void ShowWindow(int) {}

private:
    WINDOWPLACEMENT m_placement = {};

public:
    virtual void TmcBuildMessageMap(MfcMessageMap& tmcMap) { (void)tmcMap; }
};

class CMDIFrameWnd : public CFrameWnd {};
class CMDIChildWnd : public CFrameWnd {};
class CFormView : public CView
{
public:
    explicit CFormView(UINT = 0) {}
    void OnDraw(CDC*) override {}
    virtual void DoDataExchange(CDataExchange*) {}
};

// --- Доработка готового диалога слоем интерфейса -------------------------------
// Слой совместимости строит окно по разметке из ресурсов Windows и больше о нём
// ничего не знает. Но некоторые окна нужно доводить до ума уже на месте: в окне
// «О программе» — сделать адрес сайта и руководство ссылками и приписать, для
// какой системы собрана программа. Где лежит документация и как называется
// система — дело слоя интерфейса, поэтому здесь только точка расширения:
// обработчик, если он установлен, вызывается для каждого построенного окна.
typedef void (*TmcDialogDecorator)(void* qDialog);
void tmc_set_dialog_decorator(TmcDialogDecorator fn);
TmcDialogDecorator tmc_dialog_decorator();

// Стандартные команды каркаса (ID_FILE_OPEN и прочие) нужны и коду видов.
#include "afxres.h"

#endif // !_WIN32
