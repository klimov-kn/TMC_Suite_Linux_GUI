# TMCROS — вьювер S-матриц.
#
# Два слоя:
#   tmcros_core — модули Windows-версии: документ (чтение и запись файла задания),
#                 S-матрица, выражения. Ни строчки Qt: формат файлов и расчёт
#                 должны остаться теми же, что на Windows.
#   tmcros      — интерфейс на Qt6: окно, меню, панель, MDI, отрисовка графиков.

set(WIN_APP ${WIN_SRC}/viewers/tmcrtout)

# --- Ядро (без Qt) ------------------------------------------------------------
add_library(tmcros_core STATIC
    ${WIN_APP}/tmcgroutdoc.cpp          # документ: файл задания, список графиков
    ${WIN_APP}/tmcsmatrix.cpp           # S-матрица
    ${WIN_APP}/tmcttos.cpp              # преобразование «время -> S-параметры»
    ${WIN_APP}/tmcgrexpression.cpp)     # выражения над графиками

target_include_directories(tmcros_core PUBLIC
    ${TMC_INCLUDE}
    ${WIN_APP}
    ${CMAKE_SOURCE_DIR}/src/compat/include)

target_link_libraries(tmcros_core PUBLIC tmc_platform ${TMC_MATH_LINK})
target_compile_options(tmcros_core PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

# Проверка чтения файла задания без графики: печатает, что прочиталось.
# Нужна, чтобы отделять ошибки разбора файлов от ошибок отрисовки.
add_executable(tmcros_doccheck testtools/doccheck.cpp)
target_link_libraries(tmcros_doccheck PRIVATE tmcros_core)
target_compile_options(tmcros_doccheck PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

# --- Интерфейс на Qt ----------------------------------------------------------
if(TMC_BUILD_GUI)
  # Вид перенесён из Windows-версии: рисование и обработка мыши компилируются
  # как есть, поверх слоя совместимости (src/compat) и Qt.
  add_library(tmcros_view STATIC
      ${WIN_APP}/tmcgroutview.cpp
      ${WIN_APP}/mainfrm.cpp            # рамка окна: нужна строка состояния
      # Диалоги Windows-версии: поведение берём как есть, разметку — из .rc.
      ${WIN_APP}/dialogdoc.cpp
      ${WIN_APP}/dialogttos.cpp
      ${WIN_APP}/tmcgroutcolorgraphdialog.cpp
      ${WIN_APP}/tmcgroutdialogview.cpp
      ${WIN_APP}/tmcgrparcoltypwid.cpp)
  # tmcgrparcoltypwid1.cpp в Windows-проект (TMCROS.vcxproj) не входит — там
  # осталась старая копия диалога, ссылающаяся на несуществующий ресурс.
  target_include_directories(tmcros_view PUBLIC ${WIN_APP})
  target_link_libraries(tmcros_view PUBLIC tmcros_core tmc_gui_common)
  target_compile_options(tmcros_view PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

  # Главное окно вынесено в библиотеку: тем же окном пользуется средство
  # автоматической проверки внешнего вида (снимок без участия человека).
  # Разметка диалогов. Файлы .ui и dialog_registry.cpp порождены из ресурсов
  # Windows-версии скриптом tools/rc2ui.py (вручную не править):
  #   python tools/rc2ui.py \
  #       --rc ../TMC_Suite/src/viewers/Tmcrtout/TMCGROUT.rc \
  #       --resource src/win_src/viewers/tmcrtout/resource.h \
  #       --out src/apps/tmcros/dialogs --install-fn TmcInstallDialogs_tmcros
  # uic (AUTOUIC) делает из каждого .ui заголовок ui_<имя>.h, а
  # dialog_registry.cpp отдаёт их слою совместимости.
  file(GLOB TMCROS_DIALOG_UI CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/*.ui)

  add_library(tmcros_gui STATIC
      mainwindow.cpp
      dialogs/dialog_registry.cpp
      ${TMCROS_DIALOG_UI}
      resources/tmcros.qrc)
  target_include_directories(tmcros_gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(tmcros_gui PUBLIC tmcros_view tmcros_core tmc_gui_common Qt6::Widgets)
  target_compile_options(tmcros_gui PRIVATE ${TMC_PRELUDE_FLAGS})

  # Сама программа.
  add_executable(tmcros main.cpp)
  target_link_libraries(tmcros PRIVATE tmcros_gui)
  target_compile_options(tmcros PRIVATE ${TMC_PRELUDE_FLAGS})

  # Проверка работы окна: нажимает все команды, меняет размер, щёлкает по
  # графику; при зависании печатает стек. В поставку не входит.
  add_executable(tmcros_uitest testtools/uitest.cpp)
  target_link_libraries(tmcros_uitest PRIVATE tmcros_gui)
  target_compile_options(tmcros_uitest PRIVATE -g ${TMC_PRELUDE_FLAGS})
  target_link_options(tmcros_uitest PRIVATE -rdynamic)

  # Средство проверки: рисует окно в файл. В поставку не входит.
  add_executable(tmcros_shot testtools/screenshot.cpp)
  target_link_libraries(tmcros_shot PRIVATE tmcros_gui)
  target_compile_options(tmcros_shot PRIVATE ${TMC_PRELUDE_FLAGS})
endif()
