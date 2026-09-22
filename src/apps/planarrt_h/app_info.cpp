// app_info.cpp — чем PlanarRT_H отличается от PlanarRT_X.
//
// Подписи взяты из ресурса ../TMC_Suite/src/kernels/PlanarRT_H/PlanRT_H.rc:
// строковая таблица (IDR_MAINFRAME, IDR_PLANRTTYPE), меню Help и диалог
// IDD_ABOUTBOX. Весь остальной интерфейс — общий (../planarrt_common).

#include <QtGlobal>

#include "planrt_app.h"

// Разметка диалогов из dialogs/dialog_registry.cpp (порождён tools/rc2ui.py).
void TmcInstallDialogs_planarrt_h();

const TmcPlanRtAppInfo &TmcPlanRtApp()
{
    static const TmcPlanRtAppInfo info = {
        "PlanarRT_H",                                  // раздел настроек
        "Planar RT H analyzer",                        // IDR_MAINFRAME
        "Planar RT H analyzer Files (*.tpl)",          // IDR_PLANRTTYPE
        "&About PlanRT_H...",                          // пункт меню Help
        "Planar RT H analyzer Version 4.1",            // IDD_ABOUTBOX
        "Copyright (C) 2004 Tamic_soft group",         // IDD_ABOUTBOX
        ":/icons/appicon.png"                          // значок окна
    };
    return info;
}

void TmcPlanRtInstallResources()
{
    // Ресурсы собраны в статическую библиотеку. Компоновщик выбрасывает
    // инициализатор, если на него никто не ссылается, поэтому зовём его явно —
    // иначе панель осталась бы без картинок.
    Q_INIT_RESOURCE(planarrt_h);
    TmcInstallDialogs_planarrt_h();
}
