// Заглушка <afxwin.h> для сборки под Linux: MFC здесь нет.
// Даёт слой совместимости: строки и типы (tmc_mfc.h), документ и
// настройки (tmc_mfc_doc.h), рисование и окна поверх Qt (mfc_view.h),
// диалоги и карты сообщений (mfc_dialog.h), потоки (mfc_thread.h).
//
// Слой wgl подключается здесь по той же причине, по какой на Windows его давал
// windows.h: объявления wglCreateContext, PIXELFORMATDESCRIPTOR и палитры
// приходили вместе с обычными заголовками, и код Windows-версии (FieldView)
// на это рассчитывает.
#ifndef __AFXWIN_H__
  #define __AFXWIN_H__
#endif
#pragma once
#include "mfc_thread.h"
#include "mfc_wgl.h"
