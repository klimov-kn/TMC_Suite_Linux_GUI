#include "windowplacement.h"

#include <QEvent>
#include <QGuiApplication>
#include <QObject>
#include <QRect>
#include <QScreen>
#include <QWidget>

#include <cstdlib>

#include "tmc_mfc_doc.h"

namespace {

// Раздел настроек тот же, где программа хранит остальные параметры.
const char *kSection = "Window";

// Следит за окном: восстанавливает положение при первом показе и сохраняет его
// при закрытии. Живёт как ребёнок окна, поэтому удаляется вместе с ним.
class PlacementKeeper : public QObject
{
public:
    PlacementKeeper(QWidget *window, const QString &key)
        : QObject(window), m_window(window), m_key(key)
    {
        window->installEventFilter(this);
    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == m_window) {
            if (event->type() == QEvent::Show && !m_restored) {
                m_restored = true;
                restore();
            } else if (event->type() == QEvent::Close) {
                save();
            }
        }
        return QObject::eventFilter(watched, event);
    }

private:
    QString settingKey() const { return m_key; }

    void restore()
    {
        // Средства проверки задают размер окна сами: им нужен повторяемый
        // снимок, а не последнее положение окна у пользователя.
        if (std::getenv("TMC_NO_PLACEMENT"))
            return;

        const CString value = AfxGetApp()->GetProfileString(
            kSection, settingKey().toLocal8Bit().constData(), "");
        int x = 0, y = 0, w = 0, h = 0;
        if (std::sscanf(value.GetString(), "%d %d %d %d", &x, &y, &w, &h) == 4
            && w > 100 && h > 100) {
            // Окно должно остаться на экране: настройки могли остаться от
            // машины с другим числом мониторов.
            if (QScreen *screen = QGuiApplication::primaryScreen()) {
                const QRect avail = screen->availableGeometry();
                w = qMin(w, avail.width());
                h = qMin(h, avail.height());
                x = qBound(avail.left(), x, avail.right() - w);
                y = qBound(avail.top(), y, avail.bottom() - h);
            }
            m_window->setGeometry(x, y, w, h);
            return;
        }
        tmcui::centerOnScreen(m_window);
    }

    void save()
    {
        const QRect g = m_window->geometry();
        char buffer[64];
        std::snprintf(buffer, sizeof(buffer), "%d %d %d %d",
                      g.x(), g.y(), g.width(), g.height());
        AfxGetApp()->WriteProfileString(kSection,
                                        settingKey().toLocal8Bit().constData(),
                                        buffer);
    }

    QWidget *m_window;
    QString  m_key;
    bool     m_restored = false;
};

} // namespace

namespace tmcui {

void keepPlacement(QWidget *window, const QString &key)
{
    if (window)
        new PlacementKeeper(window, key);
}

void centerOnScreen(QWidget *window)
{
    if (!window)
        return;
    QScreen *screen = window->screen() ? window->screen() : QGuiApplication::primaryScreen();
    if (!screen)
        return;
    const QRect avail = screen->availableGeometry();
    const QSize size = window->size();
    window->move(avail.left() + (avail.width() - size.width()) / 2,
                 avail.top() + (avail.height() - size.height()) / 2);
}

} // namespace tmcui
