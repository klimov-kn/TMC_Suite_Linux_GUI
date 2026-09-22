// Реализация диалогов MFC поверх Qt (см. mfc_dialog.h).
//
// Разметку строит функция, полученная из .rc (см. tools/rc2ui.py); элементы
// находятся по символьному имени, которое стало objectName виджета. Поэтому
// GetDlgItem(IDC_EDIT1) в коде Windows-версии продолжает работать.

#ifndef _WIN32

#include "mfc_dialog.h"

#include <QAbstractButton>
#include <QCheckBox>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDialog>
#include <QAbstractButton>
#include <QEvent>
#include <QGroupBox>
#include <QGuiApplication>
#include <QScreen>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QMessageBox>
#include <QPaintEvent>
#include <QShowEvent>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFontDialog>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListWidget>
#include <QPushButton>
#include <QRadioButton>
#include <QTabWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QString>
#include <QTextEdit>
#include <QPlainTextEdit>
#include <QTextDocument>
#include <QProgressBar>
#include <QApplication>
#include <QCoreApplication>
#include <QMetaObject>
#include <QTimer>
#include <QWidget>
#include <QPainter>
#include <QPixmap>
#include <QPalette>

#include <cstdio>
#include <limits>
#include <cstdlib>
#include <map>

// --- Перекодировка ------------------------------------------------------------
// Подписи и данные приходят из файлов и ресурсов Windows-версии в CP1251.
namespace {

QString cp1251(const char* s)
{
    if (!s)
        return QString();
    static const ushort table[128] = {
        0x0402,0x0403,0x201A,0x0453,0x201E,0x2026,0x2020,0x2021,
        0x20AC,0x2030,0x0409,0x2039,0x040A,0x040C,0x040B,0x040F,
        0x0452,0x2018,0x2019,0x201C,0x201D,0x2022,0x2013,0x2014,
        0x0098,0x2122,0x0459,0x203A,0x045A,0x045C,0x045B,0x045F,
        0x00A0,0x040E,0x045E,0x0408,0x00A4,0x0490,0x00A6,0x00A7,
        0x0401,0x00A9,0x0404,0x00AB,0x00AC,0x00AD,0x00AE,0x0407,
        0x00B0,0x00B1,0x0406,0x0456,0x0491,0x00B5,0x00B6,0x00B7,
        0x0451,0x2116,0x0454,0x00BB,0x0458,0x0405,0x0455,0x0457,
        0x0410,0x0411,0x0412,0x0413,0x0414,0x0415,0x0416,0x0417,
        0x0418,0x0419,0x041A,0x041B,0x041C,0x041D,0x041E,0x041F,
        0x0420,0x0421,0x0422,0x0423,0x0424,0x0425,0x0426,0x0427,
        0x0428,0x0429,0x042A,0x042B,0x042C,0x042D,0x042E,0x042F,
        0x0430,0x0431,0x0432,0x0433,0x0434,0x0435,0x0436,0x0437,
        0x0438,0x0439,0x043A,0x043B,0x043C,0x043D,0x043E,0x043F,
        0x0440,0x0441,0x0442,0x0443,0x0444,0x0445,0x0446,0x0447,
        0x0448,0x0449,0x044A,0x044B,0x044C,0x044D,0x044E,0x044F,
    };
    QString out;
    for (const char* p = s; *p; ++p) {
        unsigned char ch = static_cast<unsigned char>(*p);
        out.append(ch < 0x80 ? QChar(ch) : QChar(table[ch - 0x80]));
    }
    return out;
}

// Обратный перевод: данные, введённые пользователем, должны лечь в файл задания
// в той же кодировке, что и на Windows.
std::string toCp1251(const QString& s)
{
    std::string out;
    out.reserve(size_t(s.size()));
    for (int i = 0; i < s.size(); ++i) {
        const ushort u = s.at(i).unicode();
        if (u < 0x80) {
            out.push_back(char(u));
        } else if (u >= 0x0410 && u <= 0x044F) {
            out.push_back(char(u - 0x0410 + 0xC0));
        } else if (u == 0x0401) {
            out.push_back(char(0xA8));
        } else if (u == 0x0451) {
            out.push_back(char(0xB8));
        } else {
            out.push_back('?');
        }
    }
    return out;
}

std::map<UINT, MfcDialogRegistry::SetupFn>& dialogTable()
{
    static std::map<UINT, MfcDialogRegistry::SetupFn> t;
    return t;
}

std::map<UINT, std::string>& controlTable()
{
    static std::map<UINT, std::string> t;
    return t;
}

QWidget* findControl(void* qtDialog, UINT id)
{
    QDialog* d = static_cast<QDialog*>(qtDialog);
    if (!d)
        return 0;
    const char* name = MfcDialogRegistry::controlName(id);
    if (!name)
        return 0;
    return d->findChild<QWidget*>(QString::fromLatin1(name));
}

QString widgetText(QWidget* w)
{
    if (QLineEdit* e = qobject_cast<QLineEdit*>(w))    return e->text();
    if (QLabel* l = qobject_cast<QLabel*>(w))          return l->text();
    if (QAbstractButton* b = qobject_cast<QAbstractButton*>(w)) return b->text();
    if (QComboBox* c = qobject_cast<QComboBox*>(w))    return c->currentText();
    if (QPlainTextEdit* t = qobject_cast<QPlainTextEdit*>(w)) return t->toPlainText();
    if (QTextEdit* t = qobject_cast<QTextEdit*>(w))    return t->toPlainText();
    return QString();
}

void setWidgetText(QWidget* w, const QString& text)
{
    if (QLineEdit* e = qobject_cast<QLineEdit*>(w))    { e->setText(text); return; }
    if (QLabel* l = qobject_cast<QLabel*>(w))          { l->setText(text); return; }
    if (QAbstractButton* b = qobject_cast<QAbstractButton*>(w)) { b->setText(text); return; }
    if (QComboBox* c = qobject_cast<QComboBox*>(w))    { c->setCurrentText(text); return; }
    if (QPlainTextEdit* t = qobject_cast<QPlainTextEdit*>(w)) { t->setPlainText(text); return; }
    if (QTextEdit* t = qobject_cast<QTextEdit*>(w))    { t->setPlainText(text); return; }
}

} // namespace

// --- Реестр диалогов ----------------------------------------------------------

void MfcDialogRegistry::registerDialog(UINT idd, SetupFn setup)
{
    dialogTable()[idd] = setup;
}

void MfcDialogRegistry::registerControlName(UINT id, const char* name)
{
    if (name)
        controlTable()[id] = name;
}

MfcDialogRegistry::SetupFn MfcDialogRegistry::setupFor(UINT idd)
{
    std::map<UINT, SetupFn>::const_iterator it = dialogTable().find(idd);
    return it == dialogTable().end() ? 0 : it->second;
}

std::vector<UINT> MfcDialogRegistry::registeredDialogs()
{
    std::vector<UINT> out;
    const std::map<UINT, SetupFn>& t = dialogTable();
    for (std::map<UINT, SetupFn>::const_iterator it = t.begin(); it != t.end(); ++it)
        out.push_back(it->first);
    return out;
}

const char* MfcDialogRegistry::controlName(UINT id)
{
    std::map<UINT, std::string>::const_iterator it = controlTable().find(id);
    return it == controlTable().end() ? 0 : it->second.c_str();
}

// --- Обёртки элементов --------------------------------------------------------

void CWndCtrl::SetWindowText(const char* text)
{
    setWidgetText(static_cast<QWidget*>(m_widget), cp1251(text));
}

int CWndCtrl::GetWindowText(char* buffer, int maxLen) const
{
    if (!buffer || maxLen <= 0)
        return 0;
    const std::string s = toCp1251(widgetText(static_cast<QWidget*>(m_widget)));
    int n = int(s.size());
    if (n > maxLen - 1)
        n = maxLen - 1;
    std::memcpy(buffer, s.c_str(), size_t(n));
    buffer[n] = 0;
    return n;
}

CString CWndCtrl::GetWindowTextCS() const
{
    const std::string s = toCp1251(widgetText(static_cast<QWidget*>(m_widget)));
    return CString(s.c_str());
}

void CWndCtrl::EnableWindow(BOOL enable)
{
    if (QWidget* w = static_cast<QWidget*>(m_widget))
        w->setEnabled(enable != FALSE);
}

void CWndCtrl::ShowWindow(int show)
{
    if (QWidget* w = static_cast<QWidget*>(m_widget))
        w->setVisible(show != 0);
}

BOOL CWndCtrl::IsWindowEnabled() const
{
    QWidget* w = static_cast<QWidget*>(m_widget);
    return (w && w->isEnabled()) ? TRUE : FALSE;
}

void CWndCtrl::SetFocus()
{
    if (QWidget* w = static_cast<QWidget*>(m_widget))
        w->setFocus();
}

// Холст поверх элемента. Qt не позволяет рисовать по виджету вне обработчика
// перерисовки, поэтому рисуем в картинку, а в ReleaseDC показываем её в
// элементе. Для пользователя результат тот же, что в Windows.
namespace {

struct CtrlPaint
{
    QPixmap  pixmap;
    QPainter painter;
    QWidget* widget;
};

std::map<CDC*, CtrlPaint*>& ctrlPaints()
{
    static std::map<CDC*, CtrlPaint*> t;
    return t;
}

} // namespace

CDC* CWndCtrl::GetDC()
{
    QWidget* w = static_cast<QWidget*>(m_widget);
    if (!w)
        return 0;

    CtrlPaint* p = new CtrlPaint;
    p->widget = w;
    p->pixmap = QPixmap(w->size().isEmpty() ? QSize(1, 1) : w->size());
    p->pixmap.fill(w->palette().color(QPalette::Window));
    p->painter.begin(&p->pixmap);

    CDC* dc = new CDC();
    dc->Attach(&p->painter, &p->pixmap);
    ctrlPaints()[dc] = p;
    return dc;
}

void CWndCtrl::ReleaseDC(CDC* dc)
{
    std::map<CDC*, CtrlPaint*>::iterator it = ctrlPaints().find(dc);
    if (it == ctrlPaints().end()) {
        delete dc;
        return;
    }
    CtrlPaint* p = it->second;
    p->painter.end();
    if (QLabel* label = qobject_cast<QLabel*>(p->widget))
        label->setPixmap(p->pixmap);
    ctrlPaints().erase(it);
    delete p;
    delete dc;
}

int CButton::GetCheck() const
{
    QAbstractButton* b = qobject_cast<QAbstractButton*>(static_cast<QWidget*>(m_widget));
    return (b && b->isChecked()) ? BST_CHECKED : BST_UNCHECKED;
}

void CButton::SetCheck(int check)
{
    if (QAbstractButton* b = qobject_cast<QAbstractButton*>(static_cast<QWidget*>(m_widget)))
        b->setChecked(check != BST_UNCHECKED);
}

// --- Индикатор выполнения -----------------------------------------------------

void CProgressCtrl::SetRange(short lower, short upper)
{
    SetRange32(int(lower), int(upper));
}

void CProgressCtrl::SetRange32(int lower, int upper)
{
    if (QProgressBar* b = qobject_cast<QProgressBar*>(static_cast<QWidget*>(m_widget)))
        b->setRange(lower, upper);
}

int CProgressCtrl::SetPos(int pos)
{
    QProgressBar* b = qobject_cast<QProgressBar*>(static_cast<QWidget*>(m_widget));
    if (!b)
        return 0;
    const int old = b->value();
    b->setValue(pos);
    return old;
}

int CProgressCtrl::SetStep(int step)
{
    m_step = step;
    return step;
}

int CProgressCtrl::StepIt()
{
    QProgressBar* b = qobject_cast<QProgressBar*>(static_cast<QWidget*>(m_widget));
    if (!b)
        return 0;
    const int old = b->value();
    b->setValue(old + m_step);
    return old;
}

int CProgressCtrl::GetPos() const
{
    QProgressBar* b = qobject_cast<QProgressBar*>(static_cast<QWidget*>(m_widget));
    return b ? b->value() : 0;
}

int CComboBox::AddString(const char* text)
{
    QComboBox* c = qobject_cast<QComboBox*>(static_cast<QWidget*>(m_widget));
    if (!c)
        return -1;
    c->addItem(cp1251(text));
    return c->count() - 1;
}

int CComboBox::GetCurSel() const
{
    QComboBox* c = qobject_cast<QComboBox*>(static_cast<QWidget*>(m_widget));
    return c ? c->currentIndex() : -1;
}

int CComboBox::SetCurSel(int index)
{
    if (QComboBox* c = qobject_cast<QComboBox*>(static_cast<QWidget*>(m_widget)))
        c->setCurrentIndex(index);
    return index;
}

void CComboBox::ResetContent()
{
    if (QComboBox* c = qobject_cast<QComboBox*>(static_cast<QWidget*>(m_widget)))
        c->clear();
}

int CListBox::AddString(const char* text)
{
    QListWidget* l = qobject_cast<QListWidget*>(static_cast<QWidget*>(m_widget));
    if (!l)
        return -1;
    l->addItem(cp1251(text));
    return l->count() - 1;
}

int CListBox::GetCurSel() const
{
    QListWidget* l = qobject_cast<QListWidget*>(static_cast<QWidget*>(m_widget));
    return l ? l->currentRow() : -1;
}

int CListBox::SetCurSel(int index)
{
    if (QListWidget* l = qobject_cast<QListWidget*>(static_cast<QWidget*>(m_widget)))
        l->setCurrentRow(index);
    return index;
}

void CListBox::ResetContent()
{
    if (QListWidget* l = qobject_cast<QListWidget*>(static_cast<QWidget*>(m_widget)))
        l->clear();
}

// --- Сообщения окну -----------------------------------------------------------
// Реализованы здесь, а не в mfc_view.cpp: нужны карта сообщений (mfc_dialog.h)
// и очередь событий Qt, а mfc_view.cpp намеренно собирается без Qt.

LRESULT CWnd::SendMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    // В Windows сообщение шло в оконную процедуру и попадало в обработчик из
    // карты (ON_MESSAGE). Здесь та же карта строится на месте.
    MfcMessageMap map;
    TmcBuildMessageMap(map);
    LRESULT result = 0;
    map.callMessage(message, wParam, lParam, &result);
    return result;
}

BOOL CWnd::PostMessage(UINT message, WPARAM wParam, LPARAM lParam)
{
    // Отложенная доставка, как в Windows: обработчик выполнится, когда
    // управление вернётся в цикл событий. Это важно — сообщение обычно шлёт
    // окно, которое просит себя закрыть, и разрушать его посреди его же
    // обработчика нельзя.
    //
    // Получатель обязан дожить до следующего шага цикла событий. В TMC это
    // всегда вид документа, который живёт, пока открыто окно.
    // Отправитель может быть и расчётным потоком: в пакетном режиме счётный
    // узел просит окно закрыться, досчитав последний шаг. У такого потока нет
    // своего цикла событий, поэтому сообщение кладём в очередь ГЛАВНОГО потока
    // — туда же, куда его клала оконная очередь Windows.
    CWnd* target = this;
    QObject* context = QCoreApplication::instance();
    if (!context)
        return FALSE;
    QMetaObject::invokeMethod(context, [target, message, wParam, lParam]() {
        target->SendMessage(message, wParam, lParam);
    }, Qt::QueuedConnection);
    return TRUE;
}

// --- Окно сообщения с кнопками ------------------------------------------------
// Windows-версия спрашивает подтверждение (например, «идёт расчёт, остановить и
// закрыть?») через AfxMessageBox с набором кнопок и сравнивает ответ с IDYES.
int AfxMessageBox(const char* text, UINT type, UINT /*helpId*/)
{
    // Без графики (прогон из консоли, средства проверки) спрашивать некого:
    // ведём себя как пакетный режим Windows-версии — сообщаем в поток ошибок и
    // отвечаем утвердительно.
    if (!qobject_cast<QApplication*>(QCoreApplication::instance())) {
        std::fprintf(stderr, "%s\n", text ? text : "");
        return ((type & 0x0F) == MB_YESNO) ? IDYES : IDOK;
    }

    const QString message = cp1251(text);
    const QString title = QCoreApplication::applicationName();

    QMessageBox::StandardButtons buttons = QMessageBox::Ok;
    if ((type & 0x0F) == MB_OKCANCEL)      buttons = QMessageBox::Ok | QMessageBox::Cancel;
    else if ((type & 0x0F) == MB_YESNO)    buttons = QMessageBox::Yes | QMessageBox::No;

    QMessageBox::Icon icon = QMessageBox::Information;
    const UINT iconBits = type & 0xF0;
    if (iconBits == MB_ICONERROR)            icon = QMessageBox::Critical;
    else if (iconBits == MB_ICONQUESTION)    icon = QMessageBox::Question;
    else if (iconBits == MB_ICONEXCLAMATION) icon = QMessageBox::Warning;

    QMessageBox box(icon, title, message, buttons);
    const int answer = box.exec();

    switch (answer) {
    case QMessageBox::Yes:    return IDYES;
    case QMessageBox::No:     return IDNO;
    case QMessageBox::Cancel: return IDCANCEL;
    default:                  return IDOK;
    }
}

// --- CDialog ------------------------------------------------------------------

namespace {

/// Окно Qt, знающее свой класс-диалог MFC. Нужно ради одного: закрытие окна
/// (крестик) в MFC приходило сообщением WM_CLOSE, и код Windows-версии на него
/// рассчитывает — окно уровней цвета FieldView так сообщает виду, что закрылось.
class TmcModalDialog : public QDialog
{
public:
    explicit TmcModalDialog(CDialog* owner) : m_owner(owner) {}

protected:
    void showEvent(QShowEvent* event) override
    {
        QDialog::showEvent(event);
        // Модальное окно обязано быть видно: если оно окажется позади главного,
        // программа выглядит зависшей — панель не отвечает, потому что ждёт
        // ответа на невидимое окно.
        raise();
        activateWindow();
    }

    void paintEvent(QPaintEvent* event) override
    {
        QDialog::paintEvent(event);
        // В Windows диалог получал WM_PAINT и дорисовывал образцы цвета и линий
        // поверх своих элементов. Повторяем: после обычной отрисовки окна зовём
        // тот же обработчик Windows-версии.
        if (m_owner)
            m_owner->OnPaint();
    }

    void closeEvent(QCloseEvent* event) override
    {
        // Окно закрывает не Qt, а сам диалог: обработчик может, например,
        // попросить вид разрушить окно (DestroyWindow).
        event->ignore();
        if (m_owner)
            m_owner->OnClose();
    }

private:
    CDialog* m_owner;
};

} // namespace

CDialog::CDialog(UINT idd, CWnd* parent)
    : m_nIDD(idd), m_qtDialog(0), m_parent(parent)
{
}

CDialog::~CDialog()
{
    // Немодальное окно живёт дольше вызова Create; если его не закрыли,
    // разрушаем сами — оно ссылается на этот объект.
    if (m_qtDialog && m_qtDialogOwned) {
        QDialog* dlg = static_cast<QDialog*>(m_qtDialog);
        m_qtDialog = 0;
        delete dlg;
    }
    for (std::map<UINT, CWndCtrl*>::iterator it = m_ctrlCache.begin();
         it != m_ctrlCache.end(); ++it)
        delete it->second;
}

// Главное окно программы — родитель для модальных окон.
//
// Берём активное окно, а если его нет (например, фокус ушёл в другую программу)
// — первое окно верхнего уровня самой программы. Так диалог всегда появляется
// на своём месте и поверх, а не где решит система окон.
static QWidget* tmcOwnerWindow()
{
    if (QWidget* active = QApplication::activeWindow()) {
        if (!qobject_cast<QDialog*>(active))
            return active;
    }
    for (QWidget* w : QApplication::topLevelWidgets()) {
        if (w->isVisible() && !qobject_cast<QDialog*>(w))
            return w;
    }
    return 0;
}

// Расширение элементов под их подписи.
//
// Координаты в ресурсах рассчитаны на шрифт Windows; наш шрифт может оказаться
// чуть шире, и тогда текст обрезается («X ma» вместо «X max»). Делать это можно
// только ПОСЛЕ показа окна: до этого оформление ещё не применено и размеры
// текста считаются по другому шрифту.
static void tmcGrowLabels(QWidget* root)
{
    if (!root)
        return;

    for (QWidget* w : root->findChildren<QWidget*>()) {
        const bool hasText = qobject_cast<QLabel*>(w) || qobject_cast<QAbstractButton*>(w);
        if (!hasText || qobject_cast<QGroupBox*>(w))
            continue;

        const int needed = w->sizeHint().width();
        if (needed <= w->width())
            continue;

        // Расширяем только вправо и только до ближайшего соседа: в ресурсах
        // Windows поле ввода часто стоит вплотную к подписи, и растянутая
        // подпись ушла бы под него.
        QWidget* parent = w->parentWidget();
        if (!parent)
            continue;

        // Ограничивают только соседи: у правого края окна расти можно —
        // содержимое и само окно после этого пересчитываются.
        int limit = std::numeric_limits<int>::max();
        for (QObject* obj : parent->children()) {
            QWidget* other = qobject_cast<QWidget*>(obj);
            if (!other || other == w)
                continue;
            const QRect r = other->geometry();
            const QRect mine = w->geometry();
            const bool sameRow = r.bottom() > mine.top() && r.top() < mine.bottom();
            if (sameRow && r.left() >= mine.right())
                limit = qMin(limit, r.left() - 2);
        }

        const int allowed = limit - w->x();
        if (allowed > w->width())
            w->resize(qMin(needed, allowed), w->height());
    }
}

// Пересчёт размеров окна после показа: элементы уже получили окончательный
// шрифт, поэтому видно, сколько места им нужно на самом деле.
static void tmcRefitAfterShow(QDialog* dlg, QWidget* content, QScrollArea* area)
{
    if (!dlg || !content || !area)
        return;

    // Многострочное поле высотой в одну строку: убираем внутренние отступы
    // документа и полосы прокрутки, иначе текста не видно совсем.
    for (QPlainTextEdit* edit : dlg->findChildren<QPlainTextEdit*>()) {
        if (edit->height() > 40)
            continue;
        edit->document()->setDocumentMargin(0);
        edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        edit->setLineWrapMode(QPlainTextEdit::NoWrap);
    }
    for (QTextEdit* edit : dlg->findChildren<QTextEdit*>()) {
        if (edit->height() > 40)
            continue;
        edit->document()->setDocumentMargin(0);
        edit->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        edit->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        edit->setLineWrapMode(QTextEdit::NoWrap);
    }

    // Значок программы в окне «О программе». В ресурсах Windows на его месте
    // стоит элемент ICON; разметка переносит его пустой подписью нужного
    // размера, поэтому здесь подставляем сам значок.
    for (QLabel* label : dlg->findChildren<QLabel*>()) {
        if (!label->text().isEmpty() || !label->pixmap().isNull())
            continue;
        const QSize size = label->size();
        if (size.width() < 16 || size.height() < 16 || size.width() > 64 || size.height() > 64)
            continue;
        const QPixmap icon(QStringLiteral(":/icons/appicon.png"));
        if (!icon.isNull())
            label->setPixmap(icon.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    // Подписи помещаются за счёт того, что шрифт диалогов совпадает по размеру
    // с windows-версией (см. оформление). Если всё же не хватает пары пикселей,
    // элемент расширяется вправо — но только в свободное место.
    tmcGrowLabels(content);

    QRect box;
    for (QWidget* w : content->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly))
        box = box.united(w->geometry());
    if (box.isEmpty())
        return;

    const int margin = 8;
    QSize wanted(box.right() + margin, box.bottom() + margin);
    content->setMinimumSize(wanted);
    content->resize(wanted);

    QSize windowSize = wanted + QSize(2, 2);
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect avail = screen->availableGeometry();
        windowSize.setWidth(qMin(windowSize.width(), int(avail.width() * 0.95)));
        windowSize.setHeight(qMin(windowSize.height(), int(avail.height() * 0.92)));
    }
    area->setMinimumSize(windowSize);
    dlg->setMinimumSize(windowSize);
    if (dlg->size().width() < windowSize.width() || dlg->size().height() < windowSize.height())
        dlg->resize(windowSize);
}

// Следит за первым показом окна и в этот момент подгоняет размеры.
class TmcDialogFitter : public QObject
{
public:
    TmcDialogFitter(QDialog* dlg, QWidget* content, QScrollArea* area)
        : QObject(dlg), m_dlg(dlg), m_content(content), m_area(area) {}

protected:
    bool eventFilter(QObject* watched, QEvent* event) override
    {
        if (watched == m_dlg && event->type() == QEvent::Show && !m_done) {
            m_done = true;
            tmcRefitAfterShow(m_dlg, m_content, m_area);
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QDialog* m_dlg;
    QWidget* m_content;
    QScrollArea* m_area;
    bool m_done = false;
};

// Подгонка окна диалога под содержимое.
//
// Разметка перенесена из ресурсов Windows: элементы стоят на своих местах с
// точными координатами, а окно имеет размер из `.rc`. Этого мало по двум
// причинам: подписи в другом шрифте занимают чуть больше места, а сам размер
// окна на другом экране может не подойти. Поэтому:
//   * содержимое кладётся в область с прокруткой — ничего не обрезается;
//   * окно получает размер по содержимому, но не больше экрана;
//   * окно можно растягивать (в MFC диалоги были фиксированными, но там и
//     обрезаться было нечему).
static void tmcFitDialogContent(QDialog* dlg)
{
    if (!dlg || dlg->layout())
        return;

    const QList<QWidget*> children =
        dlg->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    if (children.isEmpty())
        return;

    QRect box;
    for (QWidget* w : children)
        box = box.united(w->geometry());
    if (box.isEmpty())
        return;

    const int margin = 8;
    QWidget* content = new QWidget;
    for (QWidget* w : children)
        w->setParent(content);              // положение сохраняется
    content->setMinimumSize(box.right() + margin, box.bottom() + margin);
    content->resize(content->minimumSize());

    QScrollArea* area = new QScrollArea(dlg);
    area->setFrameShape(QFrame::NoFrame);
    area->setWidgetResizable(true);
    area->setWidget(content);

    QVBoxLayout* layout = new QVBoxLayout(dlg);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(area);

    // Размер окна: всё содержимое целиком, но в пределах экрана.
    //
    // Задать его через resize() недостаточно: у окна появилась раскладка, и при
    // показе Qt пересчитает размер по ней, а у области с прокруткой собственный
    // размер маленький. Поэтому нижнюю границу задаём самой области — тогда окно
    // открывается с полным содержимым и его можно только увеличивать.
    QSize wanted = content->minimumSize() + QSize(2, 2);
    if (QScreen* screen = QGuiApplication::primaryScreen()) {
        const QRect avail = screen->availableGeometry();
        wanted.setWidth(qMin(wanted.width(), int(avail.width() * 0.95)));
        wanted.setHeight(qMin(wanted.height(), int(avail.height() * 0.92)));
    }
    // Нижнюю границу задаём и области, и самому окну: иначе при показе Qt
    // пересчитает размер по раскладке и окно снова станет маленьким.
    area->setMinimumSize(wanted);
    dlg->setMinimumSize(wanted);
    dlg->resize(wanted);
    dlg->setSizeGripEnabled(true);

    // Окончательная подгонка — при показе, когда оформление уже применено.
    dlg->installEventFilter(new TmcDialogFitter(dlg, content, area));

    if (std::getenv("TMC_DIALOG_DEBUG"))
        std::fprintf(stderr, "диалог: элементов %d, содержимое %dx%d, окно %dx%d\n",
                     int(children.size()), box.right() + margin, box.bottom() + margin,
                     wanted.width(), wanted.height());
}

void CDialog::TmcBuildIn(void* qDialog)
{
    QDialog* dlg = static_cast<QDialog*>(qDialog);
    if (!dlg)
        return;

    MfcDialogRegistry::SetupFn setup = MfcDialogRegistry::setupFor(m_nIDD);
    m_qtDialog = dlg;
    if (setup)
        setup(dlg);

    // Кнопки OK и Отмена работают как в MFC.
    if (QWidget* ok = findControl(dlg, IDOK)) {
        if (QAbstractButton* b = qobject_cast<QAbstractButton*>(ok))
            QObject::connect(b, &QAbstractButton::clicked, dlg, [this]() { this->OnOK(); });
    }
    if (QWidget* cancel = findControl(dlg, IDCANCEL)) {
        if (QAbstractButton* b = qobject_cast<QAbstractButton*>(cancel))
            QObject::connect(b, &QAbstractButton::clicked, dlg, [this]() { this->OnCancel(); });
    }

    // Обработчики из карты сообщений: каждой команде — свой элемент.
    MfcMessageMap map;
    TmcBuildMessageMap(map);
    for (std::map<UINT, MfcMessageMap::Handler>::const_iterator it = map.handlers().begin();
         it != map.handlers().end(); ++it) {
        if (it->first == IDOK || it->first == IDCANCEL)
            continue;
        QWidget* w = findControl(dlg, it->first);
        QAbstractButton* b = qobject_cast<QAbstractButton*>(w);
        if (!b)
            continue;
        MfcMessageMap::Handler h = it->second;
        QObject::connect(b, &QAbstractButton::clicked, dlg, [h]() { h(); });
    }

    OnInitDialog();
    UpdateData(FALSE);          // значения полей -> элементы, как в MFC

    tmcFitDialogContent(dlg);
}

void CDialog::TmcDetach()
{
    m_qtDialog = 0;
    m_qtDialogOwned = FALSE;
    m_ctrlCache.clear();
}

namespace {
TmcDialogDecorator g_dialogDecorator = 0;
}

void tmc_set_dialog_decorator(TmcDialogDecorator fn)
{
    g_dialogDecorator = fn;
}

TmcDialogDecorator tmc_dialog_decorator()
{
    return g_dialogDecorator;
}

INT_PTR CDialog::DoModal()
{
    if (!MfcDialogRegistry::setupFor(m_nIDD)) {
        // Разметки нет — значит, .rc ещё не переведён в .ui для этого диалога.
        // Молча «нажать отмену» правильнее, чем показать пустое окно.
        std::fprintf(stderr, "TMC: разметка диалога %u не найдена\n", unsigned(m_nIDD));
        return IDCANCEL;
    }

    // Родителем ставим главное окно программы: тогда диалог открывается по
    // центру него, как в Windows, а не там, где решит система окон.
    TmcModalDialog dlg(this);
    if (QWidget* owner = tmcOwnerWindow())
        dlg.setParent(owner, dlg.windowFlags());
    TmcBuildIn(&dlg);
    if (g_dialogDecorator)
        g_dialogDecorator(&dlg);

    const int result = dlg.exec();
    TmcDetach();
    return result == QDialog::Accepted ? IDOK : IDCANCEL;
}

BOOL CDialog::Create(UINT idd, CWnd* parent)
{
    if (idd)
        m_nIDD = idd;
    if (parent)
        m_parent = parent;

    if (!MfcDialogRegistry::setupFor(m_nIDD)) {
        std::fprintf(stderr, "TMC: разметка диалога %u не найдена\n", unsigned(m_nIDD));
        return FALSE;
    }
    if (m_qtDialog)             // окно уже открыто — как в MFC, второго не делаем
        return TRUE;

    // Немодальное окно: создаётся в куче и живёт до DestroyWindow, поэтому при
    // открытом окне можно работать с остальной программой (вращать сцену).
    TmcModalDialog* dlg = new TmcModalDialog(this);
    dlg->setAttribute(Qt::WA_DeleteOnClose, false);
    TmcBuildIn(dlg);
    if (g_dialogDecorator)
        g_dialogDecorator(dlg);
    m_qtDialogOwned = TRUE;
    dlg->show();
    return TRUE;
}

BOOL CDialog::Create(const char* /*templateName*/, CWnd* parent)
{
    return Create(UINT(0), parent);
}

void CDialog::OnClose()
{
    // Как в MFC: закрытие окна крестиком равносильно отмене.
    EndDialog(IDCANCEL);
}

BOOL CDialog::DestroyWindow()
{
    QDialog* dlg = static_cast<QDialog*>(m_qtDialog);
    const BOOL owned = m_qtDialogOwned;
    OnDestroy();
    TmcDetach();
    if (dlg && owned) {
        dlg->hide();
        // Разрушаем не сразу: вызов часто приходит из обработчика самого окна.
        dlg->deleteLater();
    }
    return TRUE;
}

UINT CDialog::SetTimer(UINT id, UINT elapseMs, void*)
{
    QDialog* d = static_cast<QDialog*>(m_qtDialog);
    if (!d)
        return 0;
    KillTimer(id);
    QTimer* t = new QTimer(d);
    t->setInterval(int(elapseMs));
    QObject::connect(t, &QTimer::timeout, d, [this, id]() { this->OnTimer(UINT_PTR(id)); });
    t->start();
    m_timers[id] = t;
    return id;
}

BOOL CDialog::KillTimer(UINT id)
{
    std::map<UINT, void*>::iterator it = m_timers.find(id);
    if (it == m_timers.end())
        return FALSE;
    QTimer* t = static_cast<QTimer*>(it->second);
    t->stop();
    t->deleteLater();
    m_timers.erase(it);
    return TRUE;
}

BOOL CDialog::OnInitDialog()
{
    return TRUE;
}

void CDialog::DoDataExchange(CDataExchange*)
{
}

void CDialog::OnOK()
{
    if (!UpdateData(TRUE))
        return;
    EndDialog(IDOK);
}

void CDialog::OnCancel()
{
    EndDialog(IDCANCEL);
}

BOOL CDialog::UpdateData(BOOL saveAndValidate)
{
    CDataExchange dx(this, saveAndValidate);
    DoDataExchange(&dx);
    return TRUE;
}

void CDialog::EndDialog(int result)
{
    if (QDialog* d = static_cast<QDialog*>(m_qtDialog))
        d->done(result == IDOK ? QDialog::Accepted : QDialog::Rejected);
}

CWndCtrl* CDialog::GetDlgItem(UINT id) const
{
    std::map<UINT, CWndCtrl*>::const_iterator it = m_ctrlCache.find(id);
    if (it != m_ctrlCache.end())
        return it->second;
    QWidget* w = findControl(m_qtDialog, id);
    CWndCtrl* ctrl = new CWndCtrl(w);
    m_ctrlCache[id] = ctrl;
    return ctrl;
}

void CDialog::SetDlgItemText(UINT id, const char* text)
{
    setWidgetText(findControl(m_qtDialog, id), cp1251(text));
}

int CDialog::GetDlgItemText(UINT id, char* buffer, int maxLen) const
{
    if (!buffer || maxLen <= 0)
        return 0;
    const std::string s = toCp1251(widgetText(findControl(m_qtDialog, id)));
    int n = int(s.size());
    if (n > maxLen - 1)
        n = maxLen - 1;
    std::memcpy(buffer, s.c_str(), size_t(n));
    buffer[n] = 0;
    return n;
}

void CDialog::SetDlgItemInt(UINT id, int value, BOOL)
{
    char buf[32];
    std::snprintf(buf, sizeof(buf), "%d", value);
    SetDlgItemText(id, buf);
}

UINT CDialog::GetDlgItemInt(UINT id, BOOL* translated, BOOL) const
{
    char buf[64];
    GetDlgItemText(id, buf, sizeof(buf));
    char* end = 0;
    long v = std::strtol(buf, &end, 10);
    if (translated)
        *translated = (end && end != buf) ? TRUE : FALSE;
    return UINT(v);
}

void CDialog::CheckRadioButton(UINT first, UINT last, UINT check)
{
    for (UINT id = first; id <= last; ++id) {
        if (QAbstractButton* b = qobject_cast<QAbstractButton*>(findControl(m_qtDialog, id)))
            b->setChecked(id == check);
    }
}

void CDialog::CheckDlgButton(UINT id, UINT check)
{
    if (QAbstractButton* b = qobject_cast<QAbstractButton*>(findControl(m_qtDialog, id)))
        b->setChecked(check != BST_UNCHECKED);
}

UINT CDialog::IsDlgButtonChecked(UINT id) const
{
    QAbstractButton* b = qobject_cast<QAbstractButton*>(findControl(m_qtDialog, id));
    return (b && b->isChecked()) ? BST_CHECKED : BST_UNCHECKED;
}

void CDialog::SetWindowText(const char* text)
{
    if (QDialog* d = static_cast<QDialog*>(m_qtDialog))
        d->setWindowTitle(cp1251(text));
}

// --- DDX / DDV ----------------------------------------------------------------

// Доступ к виджету элемента: CDialog::GetDlgItem уже умеет искать его по
// символьному имени, поэтому обмен данными пользуется тем же путём.
namespace {
QWidget* ctrl(CDataExchange* pDX, int id)
{
    if (!pDX || !pDX->m_pDlgWnd)
        return 0;
    CWndCtrl* c = pDX->m_pDlgWnd->GetDlgItem(UINT(id));
    return c ? static_cast<QWidget*>(c->widget()) : 0;
}
} // namespace

void DDX_Text(CDataExchange* pDX, int id, CString& value)
{
    QWidget* w = ctrl(pDX, id);
    if (!w)
        return;
    if (pDX->m_bSaveAndValidate)
        value = toCp1251(widgetText(w)).c_str();
    else
        setWidgetText(w, cp1251(value.GetString()));
}

void DDX_Text(CDataExchange* pDX, int id, char* value, int maxLen)
{
    QWidget* w = ctrl(pDX, id);
    if (!w || !value)
        return;
    if (pDX->m_bSaveAndValidate) {
        const std::string s = toCp1251(widgetText(w));
        int n = int(s.size());
        if (n > maxLen - 1)
            n = maxLen - 1;
        std::memcpy(value, s.c_str(), size_t(n));
        value[n] = 0;
    } else {
        setWidgetText(w, cp1251(value));
    }
}

void DDX_Text(CDataExchange* pDX, int id, int& value)
{
    QWidget* w = ctrl(pDX, id);
    if (!w)
        return;
    if (pDX->m_bSaveAndValidate) {
        value = widgetText(w).trimmed().toInt();
    } else {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "%d", value);
        setWidgetText(w, QString::fromLatin1(buf));
    }
}

void DDX_Text(CDataExchange* pDX, int id, UINT& value)
{
    int v = int(value);
    DDX_Text(pDX, id, v);
    value = UINT(v);
}

void DDX_Text(CDataExchange* pDX, int id, long& value)
{
    int v = int(value);
    DDX_Text(pDX, id, v);
    value = v;
}

void DDX_Text(CDataExchange* pDX, int id, double& value)
{
    QWidget* w = ctrl(pDX, id);
    if (!w)
        return;
    if (pDX->m_bSaveAndValidate) {
        value = widgetText(w).trimmed().toDouble();
    } else {
        // Формат тот же, что в Windows-версии: %g не теряет значащих цифр и не
        // добавляет лишних нулей.
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%g", value);
        setWidgetText(w, QString::fromLatin1(buf));
    }
}

void DDX_Text(CDataExchange* pDX, int id, float& value)
{
    double v = value;
    DDX_Text(pDX, id, v);
    value = float(v);
}

void DDX_Check(CDataExchange* pDX, int id, int& value)
{
    QAbstractButton* b = qobject_cast<QAbstractButton*>(ctrl(pDX, id));
    if (!b)
        return;
    if (pDX->m_bSaveAndValidate)
        value = b->isChecked() ? 1 : 0;
    else
        b->setChecked(value != 0);
}

void DDX_Radio(CDataExchange* pDX, int id, int& value)
{
    // В MFC переключатели идут подряд от первого с признаком WS_GROUP; номер в
    // группе и есть значение. Ищем виджеты по порядку идентификаторов.
    if (!pDX || !pDX->m_pDlgWnd)
        return;
    int index = 0;
    for (UINT cur = UINT(id); ; ++cur, ++index) {
        QAbstractButton* b = qobject_cast<QAbstractButton*>(ctrl(pDX, int(cur)));
        if (!b)
            break;
        if (pDX->m_bSaveAndValidate) {
            if (b->isChecked())
                value = index;
        } else {
            b->setChecked(index == value);
        }
        if (index > 64)         // защита от бесконечного перебора
            break;
    }
}

void DDX_Control(CDataExchange* pDX, int id, CWndCtrl& widget)
{
    // В MFC связывает переменную-обёртку с элементом окна. Здесь достаточно
    // запомнить виджет: дальше обёртка обращается к нему напрямую.
    widget.attach(ctrl(pDX, id));
}

void DDX_CBIndex(CDataExchange* pDX, int id, int& index)
{
    QComboBox* c = qobject_cast<QComboBox*>(ctrl(pDX, id));
    if (!c)
        return;
    if (pDX->m_bSaveAndValidate)
        index = c->currentIndex();
    else
        c->setCurrentIndex(index);
}

void DDX_LBIndex(CDataExchange* pDX, int id, int& index)
{
    QListWidget* l = qobject_cast<QListWidget*>(ctrl(pDX, id));
    if (!l)
        return;
    if (pDX->m_bSaveAndValidate)
        index = l->currentRow();
    else
        l->setCurrentRow(index);
}

void DDV_MaxChars(CDataExchange* pDX, CString& value, int maxChars)
{
    if (pDX && pDX->m_bSaveAndValidate && value.GetLength() > maxChars) {
        char* p = const_cast<char*>(value.GetString());
        p[maxChars] = 0;
    }
}

void DDV_MaxChars(CDataExchange* pDX, char* value, int maxChars)
{
    if (pDX && pDX->m_bSaveAndValidate && value && int(std::strlen(value)) > maxChars)
        value[maxChars] = 0;
}

void DDV_MinMaxInt(CDataExchange*, int, int, int) {}
void DDV_MinMaxLong(CDataExchange*, long, long, long) {}
void DDV_MinMaxDouble(CDataExchange*, double, double, double) {}
void DDV_MinMaxFloat(CDataExchange*, float, float, float) {}

// --- Стандартные диалоги ------------------------------------------------------

CFileDialog::CFileDialog(BOOL openFileDialog, const char* defExt, const char* fileName,
                         DWORD, const char* filter, CWnd* parent)
    : m_open(openFileDialog),
      m_defExt(defExt ? defExt : ""),
      m_fileName(fileName ? fileName : ""),
      m_filter(filter ? filter : ""),
      m_parent(parent)
{
}

INT_PTR CFileDialog::DoModal()
{
    // Фильтр MFC записан как "Описание|шаблон||"; переводим в вид Qt.
    QString filter;
    if (!m_filter.empty()) {
        const QStringList parts = cp1251(m_filter.c_str()).split(QLatin1Char('|'));
        for (int i = 0; i + 1 < parts.size(); i += 2) {
            if (parts[i].isEmpty())
                break;
            if (!filter.isEmpty())
                filter += QStringLiteral(";;");
            filter += parts[i] + QStringLiteral(" (") + parts[i + 1] + QLatin1Char(')');
        }
    }

    const QString start = cp1251(m_fileName.c_str());
    const QString path = m_open
        ? QFileDialog::getOpenFileName(0, QString(), start, filter)
        : QFileDialog::getSaveFileName(0, QString(), start, filter);
    if (path.isEmpty())
        return IDCANCEL;
    m_path = path.toLocal8Bit().constData();
    return IDOK;
}

CString CFileDialog::GetFileName() const
{
    const std::string::size_type pos = m_path.find_last_of('/');
    return CString(pos == std::string::npos ? m_path.c_str() : m_path.c_str() + pos + 1);
}

CColorDialog::CColorDialog(COLORREF initial, DWORD, CWnd* parent)
    : m_color(initial), m_parent(parent)
{
}

INT_PTR CColorDialog::DoModal()
{
    const QColor initial(int(GetRValue(m_color)), int(GetGValue(m_color)), int(GetBValue(m_color)));
    const QColor c = QColorDialog::getColor(initial, 0);
    if (!c.isValid())
        return IDCANCEL;
    m_color = RGB(c.red(), c.green(), c.blue());
    return IDOK;
}

CFontDialog::CFontDialog(LOGFONT* initial, DWORD, void*, CWnd* parent)
    : m_parent(parent)
{
    std::memset(&m_lf, 0, sizeof(m_lf));
    if (initial)
        m_lf = *initial;
}

INT_PTR CFontDialog::DoModal()
{
    QFont start;
    if (m_lf.lfFaceName[0])
        start.setFamily(QString::fromLatin1(m_lf.lfFaceName));
    if (m_lf.lfHeight)
        start.setPixelSize(int(m_lf.lfHeight < 0 ? -m_lf.lfHeight : m_lf.lfHeight));
    start.setBold(m_lf.lfWeight >= FW_BOLD);
    start.setItalic(m_lf.lfItalic != 0);

    bool ok = false;
    const QFont f = QFontDialog::getFont(&ok, start, 0);
    if (!ok)
        return IDCANCEL;

    std::memset(&m_lf, 0, sizeof(m_lf));
    const std::string family = f.family().toLatin1().constData();
    std::strncpy(m_lf.lfFaceName, family.c_str(), LF_FACESIZE - 1);
    m_lf.lfHeight = -(f.pixelSize() > 0 ? f.pixelSize() : f.pointSize());
    m_lf.lfWeight = f.bold() ? FW_BOLD : FW_NORMAL;
    m_lf.lfItalic = f.italic() ? 1 : 0;
    m_lf.lfUnderline = f.underline() ? 1 : 0;
    m_lf.lfCharSet = DEFAULT_CHARSET;
    return IDOK;
}

void CFontDialog::GetCurrentFont(LOGFONT* lf) const
{
    if (lf)
        *lf = m_lf;
}

// --- CImageList ---------------------------------------------------------------
// Склад значков: номер картинки в наборе -> QIcon.

CImageList::CImageList()
    : m_icons(0), m_cx(0), m_cy(0)
{
}

CImageList::~CImageList()
{
    delete static_cast<QList<QIcon>*>(m_icons);
}

BOOL CImageList::Create(int cx, int cy, UINT /*flags*/, int /*initial*/, int /*grow*/)
{
    if (!m_icons)
        m_icons = new QList<QIcon>();
    else
        static_cast<QList<QIcon>*>(m_icons)->clear();
    m_cx = cx;
    m_cy = cy;
    return TRUE;
}

BOOL CImageList::Create(UINT /*bitmapId*/, int cx, int /*grow*/, COLORREF /*mask*/)
{
    // Растр ресурсов Windows (полоса картинок) сюда не переносится: значки
    // добавляются поштучно, AddIcon.
    return Create(cx, cx, 0, 0, 0);
}

int CImageList::AddIcon(const char* resourceName)
{
    if (!resourceName || !resourceName[0])
        return -1;
    if (!m_icons)
        m_icons = new QList<QIcon>();
    QList<QIcon>* icons = static_cast<QList<QIcon>*>(m_icons);
    icons->append(QIcon(QString::fromUtf8(resourceName)));
    return int(icons->size()) - 1;
}

int CImageList::Add(void* /*bitmap*/, COLORREF /*mask*/)
{
    // Место в наборе занимается, картинки у него нет: растров GDI под Linux нет.
    if (!m_icons)
        m_icons = new QList<QIcon>();
    QList<QIcon>* icons = static_cast<QList<QIcon>*>(m_icons);
    icons->append(QIcon());
    return int(icons->size()) - 1;
}

BOOL CImageList::DeleteImageList()
{
    if (m_icons)
        static_cast<QList<QIcon>*>(m_icons)->clear();
    return TRUE;
}

int CImageList::GetImageCount() const
{
    return m_icons ? int(static_cast<QList<QIcon>*>(m_icons)->size()) : 0;
}

void* CImageList::IconAt(int index) const
{
    QList<QIcon>* icons = static_cast<QList<QIcon>*>(m_icons);
    if (!icons || index < 0 || index >= int(icons->size()))
        return 0;
    return &(*icons)[index];
}

// --- CListCtrl ----------------------------------------------------------------
// Список с колонками поверх QTreeWidget: строка списка — элемент верхнего
// уровня, колонка — колонка дерева.

namespace {

QTreeWidget* asTree(void* widget)
{
    return qobject_cast<QTreeWidget*>(static_cast<QWidget*>(widget));
}

} // namespace

int CListCtrl::InsertColumn(int col, const char* title, int format, int width, int /*subItem*/)
{
    QTreeWidget* tree = asTree(m_widget);
    if (!tree)
        return -1;
    if (col < 0)
        col = tree->columnCount();
    if (col >= tree->columnCount())
        tree->setColumnCount(col + 1);

    QTreeWidgetItem* header = tree->headerItem();
    if (header) {
        header->setText(col, cp1251(title));
        // Выравнивание колонки — как задано в LVCFMT_*.
        Qt::Alignment align = Qt::AlignLeft;
        if (format == LVCFMT_RIGHT)       align = Qt::AlignRight;
        else if (format == LVCFMT_CENTER) align = Qt::AlignHCenter;
        header->setTextAlignment(col, align | Qt::AlignVCenter);
    }
    if (width >= 0)
        tree->setColumnWidth(col, width);
    return col;
}

int CListCtrl::GetColumnCount() const
{
    QTreeWidget* tree = asTree(m_widget);
    return tree ? tree->columnCount() : 0;
}

int CListCtrl::InsertItem(int item, const char* text)
{
    return InsertItem(item, text, -1);
}

int CListCtrl::InsertItem(int item, const char* text, int image)
{
    QTreeWidget* tree = asTree(m_widget);
    if (!tree)
        return -1;
    QTreeWidgetItem* row = new QTreeWidgetItem();
    row->setText(0, cp1251(text));
    if (image >= 0 && m_images) {
        if (QIcon* icon = static_cast<QIcon*>(m_images->IconAt(image)))
            row->setIcon(0, *icon);
    }
    if (item < 0 || item > tree->topLevelItemCount())
        item = tree->topLevelItemCount();
    tree->insertTopLevelItem(item, row);
    return item;
}

BOOL CListCtrl::SetItemText(int item, int subItem, const char* text)
{
    QTreeWidget* tree = asTree(m_widget);
    if (!tree)
        return FALSE;
    QTreeWidgetItem* row = tree->topLevelItem(item);
    if (!row)
        return FALSE;
    row->setText(subItem, cp1251(text));
    return TRUE;
}

CString CListCtrl::GetItemText(int item, int subItem) const
{
    QTreeWidget* tree = asTree(m_widget);
    if (!tree)
        return CString();
    QTreeWidgetItem* row = tree->topLevelItem(item);
    if (!row)
        return CString();
    return CString(toCp1251(row->text(subItem)).c_str());
}

BOOL CListCtrl::DeleteItem(int item)
{
    QTreeWidget* tree = asTree(m_widget);
    if (!tree || item < 0 || item >= tree->topLevelItemCount())
        return FALSE;
    delete tree->takeTopLevelItem(item);
    return TRUE;
}

BOOL CListCtrl::DeleteAllItems()
{
    if (QTreeWidget* tree = asTree(m_widget))
        tree->clear();
    return TRUE;
}

int CListCtrl::GetItemCount() const
{
    QTreeWidget* tree = asTree(m_widget);
    return tree ? tree->topLevelItemCount() : 0;
}

int CListCtrl::GetSelectionMark() const
{
    QTreeWidget* tree = asTree(m_widget);
    if (!tree)
        return -1;
    return tree->currentItem() ? tree->indexOfTopLevelItem(tree->currentItem()) : -1;
}

void CListCtrl::SetSelectionMark(int item)
{
    QTreeWidget* tree = asTree(m_widget);
    if (tree)
        tree->setCurrentItem(tree->topLevelItem(item));
}

void CListCtrl::SetImageList(CImageList* list, int /*type*/)
{
    m_images = list;
}

DWORD CListCtrl::SetExtendedStyle(DWORD style)
{
    const DWORD old = m_exStyle;
    m_exStyle = style;
    if (QTreeWidget* tree = asTree(m_widget)) {
        tree->setAllColumnsShowFocus((style & LVS_EX_FULLROWSELECT) != 0);
        tree->setRootIsDecorated(false);
    }
    return old;
}

// --- CPropertyPage ------------------------------------------------------------

void CPropertyPage::SetModified(BOOL changed)
{
    m_bModified = changed ? TRUE : FALSE;
    if (m_sheet)
        m_sheet->UpdateApplyButton();
}

// --- CPropertySheet -----------------------------------------------------------
// Окно с закладками: QTabWidget сверху, кнопки OK / Cancel / Apply снизу — тот
// же порядок, что в Windows. Каждая страница остаётся своим диалогом, её окно
// просто становится содержимым вкладки.

CPropertySheet::CPropertySheet(UINT /*nIDCaption*/, CWnd* /*parent*/, UINT iSelectPage)
    : m_active(int(iSelectPage)), m_qtSheet(0), m_qtTabs(0), m_qtApply(0)
{
}

CPropertySheet::CPropertySheet(const char* pszCaption, CWnd* /*parent*/, UINT iSelectPage)
    : m_caption(pszCaption ? pszCaption : ""), m_active(int(iSelectPage)),
      m_qtSheet(0), m_qtTabs(0), m_qtApply(0)
{
}

CPropertySheet::~CPropertySheet()
{
}

void CPropertySheet::AddPage(CPropertyPage* page)
{
    if (!page)
        return;
    page->m_sheet = this;
    m_pages.push_back(page);
}

void CPropertySheet::RemovePage(CPropertyPage* page)
{
    for (std::vector<CPropertyPage*>::iterator it = m_pages.begin(); it != m_pages.end(); ++it) {
        if (*it == page) {
            (*it)->m_sheet = 0;
            m_pages.erase(it);
            return;
        }
    }
}

CPropertyPage* CPropertySheet::GetPage(int index) const
{
    if (index < 0 || index >= int(m_pages.size()))
        return 0;
    return m_pages[size_t(index)];
}

CPropertyPage* CPropertySheet::GetActivePage() const
{
    return GetPage(m_active);
}

BOOL CPropertySheet::SetActivePage(int index)
{
    if (index < 0 || index >= int(m_pages.size()))
        return FALSE;
    m_active = index;
    if (QTabWidget* tabs = static_cast<QTabWidget*>(m_qtTabs))
        tabs->setCurrentIndex(index);
    return TRUE;
}

void CPropertySheet::SetTitle(const char* title, DWORD /*style*/)
{
    m_caption = title ? title : "";
    if (QDialog* sheet = static_cast<QDialog*>(m_qtSheet))
        sheet->setWindowTitle(cp1251(m_caption.c_str()));
}

BOOL CPropertySheet::OnInitDialog()
{
    return TRUE;
}

void CPropertySheet::UpdateApplyButton()
{
    QPushButton* apply = static_cast<QPushButton*>(m_qtApply);
    if (!apply)
        return;
    bool anyModified = false;
    for (size_t i = 0; i < m_pages.size(); ++i) {
        if (m_pages[i]->IsModified()) {
            anyModified = true;
            break;
        }
    }
    apply->setEnabled(anyModified);
}

void CPropertySheet::ApplyPages()
{
    // Как в MFC: сначала значения элементов переносятся в переменные страницы
    // (UpdateData), затем страница применяет их (OnApply шлёт виду сообщение).
    for (size_t i = 0; i < m_pages.size(); ++i) {
        CPropertyPage* page = m_pages[i];
        page->UpdateData(TRUE);
        page->OnApply();
        page->SetModified(FALSE);
    }
}

INT_PTR CPropertySheet::DoModal()
{
    if (m_pages.empty())
        return IDCANCEL;

    QDialog sheet;
    m_qtSheet = &sheet;
    sheet.setWindowTitle(m_caption.empty() ? QStringLiteral("Properties")
                                           : cp1251(m_caption.c_str()));

    QVBoxLayout* layout = new QVBoxLayout(&sheet);
    QTabWidget* tabs = new QTabWidget(&sheet);
    m_qtTabs = tabs;
    layout->addWidget(tabs);

    // Каждая страница строится в своём окне Qt, а окно становится вкладкой.
    // Подпись корешка — заголовок диалога из ресурсов (Place, Topology, ...),
    // как в Windows.
    for (size_t i = 0; i < m_pages.size(); ++i) {
        CPropertyPage* page = m_pages[i];
        QDialog* pageWindow = new QDialog(tabs);
        page->TmcBuildIn(pageWindow);
        QString caption = pageWindow->windowTitle();
        if (caption.isEmpty() && page->m_strCaption.GetLength() > 0)
            caption = cp1251(page->m_strCaption.GetString());
        if (caption.isEmpty())
            caption = QStringLiteral("Page %1").arg(int(i) + 1);
        tabs->addTab(pageWindow, caption);
    }

    // Кнопки внизу окна, как в Windows: OK, Cancel, Apply.
    QDialogButtonBox* buttons = new QDialogButtonBox(&sheet);
    QPushButton* okButton = buttons->addButton(QDialogButtonBox::Ok);
    QPushButton* cancelButton = buttons->addButton(QDialogButtonBox::Cancel);
    QPushButton* applyButton = buttons->addButton(QDialogButtonBox::Apply);
    m_qtApply = applyButton;
    applyButton->setEnabled(false);
    layout->addWidget(buttons);

    QObject::connect(tabs, &QTabWidget::currentChanged, &sheet, [this](int index) {
        // Переход на другую закладку: MFC спрашивает у прежней страницы
        // разрешение уйти и сообщает новой, что её показали.
        CPropertyPage* prev = GetActivePage();
        if (prev && prev != GetPage(index))
            prev->OnKillActive();
        m_active = index;
        if (CPropertyPage* page = GetActivePage())
            page->OnSetActive();
    });

    QObject::connect(applyButton, &QPushButton::clicked, &sheet, [this]() { this->ApplyPages(); });
    QObject::connect(okButton, &QPushButton::clicked, &sheet, [this, &sheet]() {
        this->ApplyPages();
        sheet.accept();
    });
    QObject::connect(cancelButton, &QPushButton::clicked, &sheet, [&sheet]() { sheet.reject(); });

    if (m_active < 0 || m_active >= int(m_pages.size()))
        m_active = 0;
    tabs->setCurrentIndex(m_active);

    OnInitDialog();
    if (CPropertyPage* page = GetActivePage())
        page->OnSetActive();

    const int result = sheet.exec();
    if (result != QDialog::Accepted)
        OnClose();

    // Окна страниц разрушаются вместе с окном набора; страницы об этом должны
    // узнать, иначе будут держать указатель на несуществующее окно.
    for (size_t i = 0; i < m_pages.size(); ++i)
        m_pages[i]->TmcDetach();
    m_qtApply = 0;
    m_qtTabs = 0;
    m_qtSheet = 0;
    return result == QDialog::Accepted ? IDOK : IDCANCEL;
}

#endif // !_WIN32
