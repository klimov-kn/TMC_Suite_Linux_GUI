// Файл создан tools/rc2ui.py из FldView.rc. Не править вручную.
//
// Что здесь. Слой совместимости (src/compat/include/mfc_dialog.h) при вызове
// CDialog::DoModal() ищет по числовому идентификатору диалога функцию разметки,
// а элементы внутри — по символьному имени. Обе таблицы заполняются здесь:
//   * функция разметки — вызов setupUi() из заголовка, который uic делает
//     из соответствующего .ui;
//   * имена элементов — все определения из resource.h.
// Если один номер описан несколькими именами, берётся первое по resource.h:
// именно оно стоит в objectName виджетов.
//
// Почему номера записаны числами, а resource.h не включён. objectName виджета
// обязан совпадать с именем элемента (IDC_GRPAR_XMAX), и uic делает из этого
// имени имя переменной в ui_*.h. Но в resource.h то же самое имя — макрос
// (#define IDC_GRPAR_XMAX 1001), и препроцессор превратил бы объявление
// переменной в «QLineEdit *1001». Поэтому resource.h сюда не включается, а
// макросы, приходящие из слоя совместимости (IDOK, IDCANCEL), снимаются ниже.
// Числовые значения взяты из resource.h на момент порождения файла; повторный
// запуск rc2ui.py обновит их вместе с разметкой.

#include <QDialog>

#include "afxwin.h"     // MfcDialogRegistry

// Снять макросы-идентификаторы: дальше эти имена — имена переменных в ui_*.h.
#undef IDC_AUTOSIZEFLAGX
#undef IDC_AUTOSIZEFLAGY
#undef IDC_AUTOSIZEFLAGZ
#undef IDC_BLEND
#undef IDC_BLENDEPS
#undef IDC_DRAWFLAGLINE
#undef IDC_DRAWFLAGSURFACE
#undef IDC_RADIO1
#undef IDC_RADIO10
#undef IDC_RADIO110
#undef IDC_RADIO113
#undef IDC_RADIO114
#undef IDC_RADIO115
#undef IDC_RADIO116
#undef IDC_RADIO117
#undef IDC_RADIO118
#undef IDC_RADIO119
#undef IDC_RADIO120
#undef IDC_RADIO121
#undef IDC_RADIO122
#undef IDC_RADIO123
#undef IDC_RADIO124
#undef IDC_RADIO125
#undef IDC_RADIO126
#undef IDC_RADIO127
#undef IDC_RADIO128
#undef IDC_RADIO129
#undef IDC_RADIO130
#undef IDC_RADIO131
#undef IDC_RADIO132
#undef IDC_RADIO133
#undef IDC_RADIO134
#undef IDC_RADIO135
#undef IDC_RADIO136
#undef IDC_RADIO137
#undef IDC_RADIO138
#undef IDC_RADIO139
#undef IDC_RADIO140
#undef IDC_RADIO141
#undef IDC_RADIO142
#undef IDC_RADIO143
#undef IDC_RADIO144
#undef IDC_RADIO145
#undef IDC_RADIO146
#undef IDC_RADIO147
#undef IDC_RADIO148
#undef IDC_RADIO149
#undef IDC_RADIO2
#undef IDC_RADIO9
#undef IDC_STATIC
#undef IDC_STATICLEVEL1
#undef IDC_STATICLEVEL10
#undef IDC_STATICLEVEL11
#undef IDC_STATICLEVEL12
#undef IDC_STATICLEVEL13
#undef IDC_STATICLEVEL14
#undef IDC_STATICLEVEL15
#undef IDC_STATICLEVEL16
#undef IDC_STATICLEVEL17
#undef IDC_STATICLEVEL18
#undef IDC_STATICLEVEL19
#undef IDC_STATICLEVEL2
#undef IDC_STATICLEVEL20
#undef IDC_STATICLEVEL21
#undef IDC_STATICLEVEL22
#undef IDC_STATICLEVEL23
#undef IDC_STATICLEVEL24
#undef IDC_STATICLEVEL25
#undef IDC_STATICLEVEL26
#undef IDC_STATICLEVEL27
#undef IDC_STATICLEVEL28
#undef IDC_STATICLEVEL29
#undef IDC_STATICLEVEL3
#undef IDC_STATICLEVEL30
#undef IDC_STATICLEVEL31
#undef IDC_STATICLEVEL32
#undef IDC_STATICLEVEL33
#undef IDC_STATICLEVEL34
#undef IDC_STATICLEVEL35
#undef IDC_STATICLEVEL36
#undef IDC_STATICLEVEL37
#undef IDC_STATICLEVEL38
#undef IDC_STATICLEVEL39
#undef IDC_STATICLEVEL4
#undef IDC_STATICLEVEL40
#undef IDC_STATICLEVEL5
#undef IDC_STATICLEVEL6
#undef IDC_STATICLEVEL7
#undef IDC_STATICLEVEL8
#undef IDC_STATICLEVEL9
#undef IDC_STATICLEVEL_COLORALL
#undef IDC_TMC_DOCLINK
#undef IDC_XAXIESANGLE
#undef IDC_XAXIESMAX
#undef IDC_XAXIESMIN
#undef IDC_XAXIESTRANS
#undef IDC_YAXIESANGLE
#undef IDC_YAXIESMAX
#undef IDC_YAXIESMIN
#undef IDC_YAXIESTRANS
#undef IDC_ZAXIESMAX
#undef IDC_ZAXIESMIN
#undef IDD_ABOUTBOX
#undef IDD_DIALOGBARCOLORH
#undef IDD_DIALOGBARCOLORH1
#undef IDD_DIALOGBAREPS
#undef IDD_DIALOGBARFIELD
#undef IDD_DIALOGBARPLACE
#undef IDD_DIALOGBARTOPOLOGY
#undef IDOK
#undef ID_ColorLevelAutoSet
#undef ID_ColorLevelAutoSet2
#undef ID_ColorLevelAutoSet3
#undef ID_ColorLevelAutoSet4
#undef ID_ColorLevelChange2
#undef ID_DEFAULT_TRXYZ_ANGXYZ
#undef ID_PLACEAXIESCOLOR
#undef ID_PLACEAXIESFONT
#undef ID_TOPEPSCOLOREPS
#undef ID_TOPINPCOLORABSORBER
#undef ID_TOPMAGCOLORMAGNETIC
#undef ID_TOPMETCOLORDEFAULT
#undef ID_TOPMETCOLORMETAL
#include "ui_IDD_ABOUTBOX.h"
#include "ui_IDD_DIALOGBARTOPOLOGY.h"
#include "ui_IDD_DIALOGBARPLACE.h"
#include "ui_IDD_DIALOGBAREPS.h"
#include "ui_IDD_DIALOGBARFIELD.h"
#include "ui_IDD_DIALOGBARCOLORH.h"
#include "ui_IDD_DIALOGBARCOLORH1.h"

namespace {

void tmcSetup_IDD_ABOUTBOX(void* qDialog)
{
    // Ui_-объект хранит только указатели: после setupUi() виджеты
    // принадлежат диалогу, и держать его дальше не нужно.
    Ui_IDD_ABOUTBOX ui;
    ui.setupUi(static_cast<QDialog*>(qDialog));
}

void tmcSetup_IDD_DIALOGBARTOPOLOGY(void* qDialog)
{
    // Ui_-объект хранит только указатели: после setupUi() виджеты
    // принадлежат диалогу, и держать его дальше не нужно.
    Ui_IDD_DIALOGBARTOPOLOGY ui;
    ui.setupUi(static_cast<QDialog*>(qDialog));
}

void tmcSetup_IDD_DIALOGBARPLACE(void* qDialog)
{
    // Ui_-объект хранит только указатели: после setupUi() виджеты
    // принадлежат диалогу, и держать его дальше не нужно.
    Ui_IDD_DIALOGBARPLACE ui;
    ui.setupUi(static_cast<QDialog*>(qDialog));
}

void tmcSetup_IDD_DIALOGBAREPS(void* qDialog)
{
    // Ui_-объект хранит только указатели: после setupUi() виджеты
    // принадлежат диалогу, и держать его дальше не нужно.
    Ui_IDD_DIALOGBAREPS ui;
    ui.setupUi(static_cast<QDialog*>(qDialog));
}

void tmcSetup_IDD_DIALOGBARFIELD(void* qDialog)
{
    // Ui_-объект хранит только указатели: после setupUi() виджеты
    // принадлежат диалогу, и держать его дальше не нужно.
    Ui_IDD_DIALOGBARFIELD ui;
    ui.setupUi(static_cast<QDialog*>(qDialog));
}

void tmcSetup_IDD_DIALOGBARCOLORH(void* qDialog)
{
    // Ui_-объект хранит только указатели: после setupUi() виджеты
    // принадлежат диалогу, и держать его дальше не нужно.
    Ui_IDD_DIALOGBARCOLORH ui;
    ui.setupUi(static_cast<QDialog*>(qDialog));
}

void tmcSetup_IDD_DIALOGBARCOLORH1(void* qDialog)
{
    // Ui_-объект хранит только указатели: после setupUi() виджеты
    // принадлежат диалогу, и держать его дальше не нужно.
    Ui_IDD_DIALOGBARCOLORH1 ui;
    ui.setupUi(static_cast<QDialog*>(qDialog));
}

// Соответствие «номер -> символьное имя» для всех элементов из resource.h.
struct TmcControlName
{
    UINT        id;
    const char* name;
};

const TmcControlName kControlNames[] = {
    { 3, "ID_DEFAULT_TRXYZ_ANGXYZ" },
    { 4, "ID_PLACEAXIESCOLOR" },
    { 5, "ID_PLACEAXIESFONT" },
    { 6, "ID_TOPEPSCOLOREPS" },
    { 7, "ID_TOPMETCOLORMETAL" },
    { 8, "ID_TOPMAGCOLORMAGNETIC" },
    { 9, "IDD_DIALOGBARFIELD" },
    { 10, "ID_TOPMETCOLORDEFAULT" },
    { 11, "ID_TOPINPCOLORABSORBER" },
    { 100, "IDD_ABOUTBOX" },
    { 3000, "IDC_TMC_DOCLINK" },
    { 104, "IDD_DIALOGBARTOPOLOGY" },
    { 105, "IDD_DIALOGBARPLACE" },
    { 106, "IDD_DIALOGBAREPS" },
    { 107, "IDD_DIALOGBARCOLORH" },
    { 108, "IDD_DIALOGBARCOLORH1" },
    { 128, "IDR_MAINFRAME" },
    { 129, "IDR_FLDVIETYPE" },
    { 130, "ID_FIELD_L" },
    { 131, "ID_FIELD_VALUE" },
    { 132, "ID_FIELD_MODUL" },
    { 1001, "IDC_DRAWFLAGSURFACE" },
    { 1002, "IDC_DRAWFLAGLINE" },
    { 1003, "IDC_AUTOSIZEFLAGX" },
    { 1004, "IDC_XAXIESMIN" },
    { 1005, "IDC_XAXIESMAX" },
    { 1006, "IDC_AUTOSIZEFLAGY" },
    { 1007, "IDC_YAXIESMIN" },
    { 1008, "IDC_YAXIESMAX" },
    { 1009, "IDC_AUTOSIZEFLAGZ" },
    { 1010, "IDC_ZAXIESMIN" },
    { 1011, "IDC_ZAXIESMAX" },
    { 1012, "IDC_BLEND" },
    { 1013, "IDC_RADIO1" },
    { 1014, "IDC_RADIO2" },
    { 1015, "IDC_XAXIESANGLE" },
    { 1016, "IDC_XAXIESTRANS" },
    { 1017, "IDC_YAXIESANGLE" },
    { 1018, "IDC_YAXIESTRANS" },
    { 1019, "IDC_BLENDEPS" },
    { 1020, "IDC_RADIO9" },
    { 1021, "IDC_RADIO113" },
    { 1022, "IDC_RADIO114" },
    { 1023, "IDC_RADIO115" },
    { 1024, "IDC_RADIO116" },
    { 1025, "IDC_RADIO117" },
    { 1026, "IDC_RADIO118" },
    { 1027, "IDC_RADIO10" },
    { 1028, "IDC_RADIO119" },
    { 1029, "IDC_RADIO120" },
    { 1030, "IDC_RADIO121" },
    { 1031, "IDC_STATICLEVEL1" },
    { 1032, "IDC_STATICLEVEL2" },
    { 1033, "IDC_STATICLEVEL3" },
    { 1034, "IDC_STATICLEVEL4" },
    { 1035, "IDC_STATICLEVEL5" },
    { 1036, "IDC_STATICLEVEL6" },
    { 1037, "IDC_STATICLEVEL7" },
    { 1038, "IDC_STATICLEVEL8" },
    { 1039, "IDC_STATICLEVEL9" },
    { 1040, "IDC_STATICLEVEL10" },
    { 1041, "IDC_STATICLEVEL11" },
    { 1042, "IDC_STATICLEVEL12" },
    { 1043, "IDC_RADIO122" },
    { 1044, "IDC_STATICLEVEL13" },
    { 1045, "IDC_RADIO123" },
    { 1046, "IDC_STATICLEVEL14" },
    { 1047, "IDC_RADIO124" },
    { 1048, "IDC_STATICLEVEL15" },
    { 1049, "IDC_RADIO125" },
    { 1050, "IDC_STATICLEVEL16" },
    { 1051, "IDC_RADIO126" },
    { 1052, "IDC_STATICLEVEL17" },
    { 1053, "IDC_RADIO127" },
    { 1054, "IDC_STATICLEVEL18" },
    { 1055, "IDC_RADIO128" },
    { 1056, "IDC_STATICLEVEL19" },
    { 1057, "IDC_RADIO129" },
    { 1058, "IDC_STATICLEVEL20" },
    { 1059, "IDC_RADIO110" },
    { 1060, "IDC_RADIO130" },
    { 1061, "IDC_RADIO131" },
    { 1062, "IDC_RADIO132" },
    { 1063, "IDC_RADIO133" },
    { 1064, "IDC_RADIO134" },
    { 1065, "IDC_RADIO135" },
    { 1066, "IDC_RADIO136" },
    { 1067, "IDC_RADIO137" },
    { 1068, "IDC_RADIO138" },
    { 1069, "IDC_RADIO139" },
    { 1070, "IDC_RADIO140" },
    { 1071, "IDC_RADIO141" },
    { 1072, "IDC_RADIO142" },
    { 1073, "IDC_RADIO143" },
    { 1074, "IDC_RADIO144" },
    { 1075, "IDC_RADIO145" },
    { 1076, "IDC_RADIO146" },
    { 1077, "IDC_RADIO147" },
    { 1078, "IDC_RADIO148" },
    { 1079, "IDC_RADIO149" },
    { 1080, "IDC_STATICLEVEL21" },
    { 1081, "IDC_STATICLEVEL22" },
    { 1082, "IDC_STATICLEVEL23" },
    { 1083, "IDC_STATICLEVEL24" },
    { 1084, "IDC_STATICLEVEL25" },
    { 1085, "IDC_STATICLEVEL26" },
    { 1086, "IDC_STATICLEVEL27" },
    { 1087, "IDC_STATICLEVEL28" },
    { 1088, "IDC_STATICLEVEL29" },
    { 1089, "IDC_STATICLEVEL30" },
    { 1090, "IDC_STATICLEVEL31" },
    { 1091, "IDC_STATICLEVEL32" },
    { 1092, "IDC_STATICLEVEL33" },
    { 1093, "IDC_STATICLEVEL34" },
    { 1094, "IDC_STATICLEVEL35" },
    { 1095, "IDC_STATICLEVEL36" },
    { 1096, "IDC_STATICLEVEL37" },
    { 1097, "IDC_STATICLEVEL38" },
    { 1098, "IDC_STATICLEVEL39" },
    { 1099, "IDC_STATICLEVEL40" },
    { 1100, "ID_ColorLevelChange2" },
    { 1101, "ID_ColorLevelAutoSet" },
    { 1102, "ID_ColorLevelAutoSet2" },
    { 1103, "ID_ColorLevelAutoSet3" },
    { 1104, "IDC_STATICLEVEL_COLORALL" },
    { 1105, "ID_ColorLevelAutoSet4" },
    { 32771, "ID_ROTATE_LEFT" },
    { 32772, "ID_ROTATE_RIGHT" },
    { 32773, "ID_ROTATE_TOP" },
    { 32774, "ID_ROTATE_BOTTOM" },
    { 32775, "ID_ROTATE_RIGHTY" },
    { 32776, "ID_ROTATE_LEFTY" },
    { 32777, "ID_TRANSLATE_XT" },
    { 32780, "ID_TRANSLATE_XB" },
    { 32781, "ID_TRANSLATE_ZB" },
    { 32782, "ID_TRANSLATE_ZT" },
    { 32783, "ID_TRANSLATE_YT" },
    { 32784, "ID_TRANSLATE_YB" },
    { 32785, "ID_VIEW_PARAMETERS" },
    { 32786, "ID_STRELKLEFT" },
    { 32787, "ID_STRELKRIGHT" },
    { 32788, "ID_STRELKUP" },
    { 32789, "ID_STRELKDOWN" },
    { 32790, "ID_VIEW_ZOOM" },
    { 32792, "ID_VIEW_ZOOM_P" },
    { 32793, "ID_VIEW_ZOOM_M" },
    { 32796, "ID_VIEW_ZOOM_PX" },
    { 32797, "ID_VIEW_ZOOM_MX" },
    { 32798, "ID_VIEW_ZOOM_PY" },
    { 32799, "ID_VIEW_ZOOM_MY" },
    { 32804, "ID_VIEW_DEFAULT" },
    { 32807, "ID_ROTATEXN" },
    { 32808, "ID_ROTATEXV" },
    { 32809, "ID_ROTATEYN" },
    { 32810, "ID_ROTATEYV" },
    { 32811, "ID_ROTATEZV" },
    { 32812, "ID_ROTATEZN" },
    { 32818, "ID_TRANSLATE_YN" },
    { 32819, "ID_TRANSLATE_YV" },
    { 32820, "ID_TRANSLATE_XN" },
    { 32821, "ID_TRANSLATE_XV" },
    { 32822, "ID_TRANSLATE_ZN" },
    { 32823, "ID_TRANSLATE_ZV" },
    { 32825, "ID_TRANSLATEDATAXN" },
    { 32826, "ID_TRANSLATEDATAXV" },
    { 32827, "ID_TRANSLATEDATAYV" },
    { 32828, "ID_TRANSLATEDATAYN" },
    { 32831, "ID_DIMENSION23SWITCH" },
    { 32833, "ID_FLAGDRAWLINE" },
    { 32834, "ID_FLAGDRAWSURFACE" },
    { 32836, "ID_VIEW_DEFAULT12" },
    { 32840, "ID_FLAGDRAWLINEEPS" },
    { 32841, "ID_FLAGDRAWSURFACEEPS" },
    { 32842, "ID_DIMENSION23SWITCHEPS" },
    { 32846, "ID_VIEW_PROPORTIONALLY" },
    { 32847, "ID_FLAGDRAWLINETOP" },
    { 32848, "ID_FLAGDRAWSURFACETOP" },
    { 32849, "ID_DIMENSION23SWITCHTOP" },
    { 32854, "ID_VIEW_TOPOLOGY_BOUNDARY_EPS" },
    { 32855, "ID_VIEW_TOPOLOGY_BOUNDARY_METAL" },
    { 32856, "ID_VIEW_TOPOLOGY_BOUNDARY_MAGNETIC" },
    { 32857, "ID_VIEW_TOPOLOGY_BOUNDARY_ABSORBER" },
    { 32858, "ID_VIEW_TOPOLOGY_BOUNDARY_INPUT" },
    { 32861, "ID_VIEW_EPS_SIZE" },
    { 32862, "ID_VIEW_FIELD_SIZE" },
    { 32864, "ID_VIEW_TOPOLOGY_DIMENSIONS_ALONGANXAXIES" },
    { 32865, "ID_VIEW_TOPOLOGY_DIMENSIONS_ALONGANYAXIES" },
    { 32866, "ID_VIEW_TOPOLOGY_SIZE_BLOCK" },
    { 32867, "ID_VIEW_TOPOLOGY_SIZE_LINKLIST" },
    { 32882, "ID_EPS_TYPE_EPS" },
    { 32883, "ID_EPS_TYPE_N" },
    { 32884, "ID_EPS_TYPE_Y" },
    { 32885, "ID_EPS_TYPE_B" },
    { 1, "IDOK" },
    { 2, "IDCANCEL" },
};

bool tmcInstall()
{
    MfcDialogRegistry::registerDialog(100 /* IDD_ABOUTBOX */, &tmcSetup_IDD_ABOUTBOX);
    MfcDialogRegistry::registerDialog(104 /* IDD_DIALOGBARTOPOLOGY */, &tmcSetup_IDD_DIALOGBARTOPOLOGY);
    MfcDialogRegistry::registerDialog(105 /* IDD_DIALOGBARPLACE */, &tmcSetup_IDD_DIALOGBARPLACE);
    MfcDialogRegistry::registerDialog(106 /* IDD_DIALOGBAREPS */, &tmcSetup_IDD_DIALOGBAREPS);
    MfcDialogRegistry::registerDialog(9 /* IDD_DIALOGBARFIELD */, &tmcSetup_IDD_DIALOGBARFIELD);
    MfcDialogRegistry::registerDialog(107 /* IDD_DIALOGBARCOLORH */, &tmcSetup_IDD_DIALOGBARCOLORH);
    MfcDialogRegistry::registerDialog(108 /* IDD_DIALOGBARCOLORH1 */, &tmcSetup_IDD_DIALOGBARCOLORH1);

    const int count = int(sizeof(kControlNames) / sizeof(kControlNames[0]));
    for (int i = 0; i < count; ++i)
        MfcDialogRegistry::registerControlName(kControlNames[i].id,
                                               kControlNames[i].name);
    return true;
}

} // namespace

// Вызывается из кода программы один раз при запуске. Не полагаемся на
// инициализацию статических объектов: файл лежит в статической библиотеке, и
// без явной ссылки компоновщик мог бы его не подключить.
void TmcInstallDialogs_fieldview()
{
    static const bool done = tmcInstall();
    (void)done;
}
