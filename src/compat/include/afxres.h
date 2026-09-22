// afxres.h — стандартные идентификаторы команд MFC.
//
// Ресурсы и код Windows-версии ссылаются на команды каркаса (открыть файл,
// печать, окна, справка) по этим номерам. Значения обязаны совпадать с MFC:
// они стоят в .rc, и по ним же собирается меню и панель в Qt-версии.

#pragma once

#ifndef _WIN32

// --- Файл ---------------------------------------------------------------------
#define ID_FILE_NEW             0xE100
#define ID_FILE_OPEN            0xE101
#define ID_FILE_CLOSE           0xE102
#define ID_FILE_SAVE            0xE103
#define ID_FILE_SAVE_AS         0xE104
#define ID_FILE_PAGE_SETUP      0xE105
#define ID_FILE_PRINT_SETUP     0xE106
#define ID_FILE_PRINT           0xE107
#define ID_FILE_PRINT_DIRECT    0xE108
#define ID_FILE_PRINT_PREVIEW   0xE109
#define ID_FILE_UPDATE          0xE10A
#define ID_FILE_SAVE_COPY_AS    0xE10B
#define ID_FILE_SEND_MAIL       0xE10C
#define ID_FILE_MRU_FIRST       0xE110
#define ID_FILE_MRU_FILE1       0xE110
#define ID_FILE_MRU_FILE2       0xE111
#define ID_FILE_MRU_FILE3       0xE112
#define ID_FILE_MRU_FILE4       0xE113
#define ID_FILE_MRU_LAST        0xE11F

// --- Правка -------------------------------------------------------------------
#define ID_EDIT_CLEAR           0xE120
#define ID_EDIT_CLEAR_ALL       0xE121
#define ID_EDIT_COPY            0xE122
#define ID_EDIT_CUT             0xE123
#define ID_EDIT_FIND            0xE124
#define ID_EDIT_PASTE           0xE125
#define ID_EDIT_REPEAT          0xE128
#define ID_EDIT_REPLACE         0xE129
#define ID_EDIT_SELECT_ALL      0xE12A
#define ID_EDIT_UNDO            0xE12B
#define ID_EDIT_REDO            0xE12C

// --- Окна ---------------------------------------------------------------------
#define ID_WINDOW_NEW           0xE130
#define ID_WINDOW_ARRANGE       0xE131
#define ID_WINDOW_CASCADE       0xE132
#define ID_WINDOW_TILE_HORZ     0xE133
#define ID_WINDOW_TILE_VERT     0xE134
#define ID_WINDOW_SPLIT         0xE135

// --- Программа и справка ------------------------------------------------------
#define ID_APP_ABOUT            0xE140
#define ID_APP_EXIT             0xE141
#define ID_HELP_INDEX           0xE142
#define ID_HELP_USING           0xE144
#define ID_CONTEXT_HELP         0xE145
#define ID_HELP                 0xE146
#define ID_DEFAULT_HELP         0xE143

// --- Вид ----------------------------------------------------------------------
#define ID_VIEW_TOOLBAR         0xE800
#define ID_VIEW_STATUS_BAR      0xE801

// --- Строка состояния ---------------------------------------------------------
#define ID_INDICATOR_EXT        0xE700
#define ID_INDICATOR_CAPS       0xE701
#define ID_INDICATOR_NUM        0xE702
#define ID_INDICATOR_SCRL       0xE703
#define ID_INDICATOR_OVR        0xE704
#define ID_INDICATOR_REC        0xE705
#define ID_SEPARATOR            0

#define AFX_IDS_APP_TITLE       0xE000
#define AFX_IDS_IDLEMESSAGE     0xE001

// --- Стили и флаги, встречающиеся в коде --------------------------------------
#define CBRS_TOP            0x0001
#define CBRS_BOTTOM         0x0002
#define CBRS_ALIGN_ANY      0x000F
#define CBRS_TOOLTIPS       0x0010
#define CBRS_FLYBY          0x0020
#define CBRS_GRIPPER        0x00400000
#define CBRS_SIZE_DYNAMIC   0x00800000
#define WS_CHILD            0x40000000L
#define WS_VISIBLE          0x10000000L
#define WS_CLIPSIBLINGS     0x04000000L
#define WS_CLIPCHILDREN     0x02000000L
#define WS_OVERLAPPEDWINDOW 0x00CF0000L
#define SW_SHOW             5
#define SW_HIDE             0
#define SW_SHOWNORMAL       1
#define SW_SHOWMINIMIZED    2
#define SW_SHOWMAXIMIZED    3
#define WS_OVERLAPPED       0x00000000L
#define WS_CAPTION          0x00C00000L
#define WS_THICKFRAME       0x00040000L
#define WS_SYSMENU          0x00080000L
#define WS_MINIMIZEBOX      0x00020000L
#define WS_MAXIMIZEBOX      0x00010000L
#define FWS_ADDTOTITLE      0x00008000L
#define FWS_PREFIXTITLE     0x00004000L

// Отладочный вывод MFC. В рабочей программе он не нужен.
#define TRACE0(s)
#define TRACE1(s, a)
#define TRACE2(s, a, b)


#define DECLARE_DYNAMIC(cls)
#define IMPLEMENT_DYNAMIC(cls, base)

#endif // !_WIN32
