// planrt_app.h — чем PlanarRT_H отличается от PlanarRT_X.
//
// Программы одинаковы: тот же документ, тот же вид, то же меню и та же панель.
// Различаются только исходники счётного ядра (свой каталог снимка и шесть
// макросов X-моды — задаются в CMakeLists) и несколько подписей из ресурсов.
// Подписи собраны здесь, чтобы весь интерфейс был общим и не дублировался.
//
// Значения берутся из строковой таблицы и диалога «About» ресурса PlanRT_H.rc
// соответствующей программы (у X он свой, в её каталоге).

#pragma once

struct TmcPlanRtAppInfo
{
    const char *appName;        ///< имя программы: файл настроек ~/.config/TMC_Suite/<имя>.conf
    const char *windowTitle;    ///< заголовок окна — строка IDR_MAINFRAME
    const char *docTypeName;    ///< описание типа файла — строка IDR_PLANRTTYPE
    const char *aboutMenuItem;  ///< пункт меню Help — «&About PlanRT_H...»
    const char *aboutProduct;   ///< верхняя строка окна «About»
    const char *aboutCopyright; ///< нижняя строка окна «About»
    const char *windowIcon;     ///< путь к значку окна в ресурсах Qt
};

/// Сведения о конкретной программе. Определяется в app_info.cpp каждой из них.
const TmcPlanRtAppInfo &TmcPlanRtApp();

/// Подключить ресурсы программы (пиктограммы) и разметку её диалогов.
/// Определяется там же: макрос Q_INIT_RESOURCE требует имени файла .qrc,
/// а имя функции установки диалогов у каждой программы своё.
void TmcPlanRtInstallResources();
