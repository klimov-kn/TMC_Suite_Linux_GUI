# TMCGROUT — вьювер графиков S-параметров по частоте.
#
# Устройство то же, что у TMCROS: код документа, вида и диалогов компилируется
# из Windows-версии поверх слоя совместимости MFC->Qt, а на Qt написано только
# главное окно.

set(WIN_APP ${WIN_SRC}/viewers/tmcgrout)

# --- Ядро: документ, выражения (без Qt) ---------------------------------------
add_library(tmcgrout_core STATIC
    ${WIN_APP}/tmcgroutdoc.cpp          # документ: файл задания, список кривых
    ${WIN_APP}/tmcgrexpression.cpp)     # выражения над S-параметрами

target_include_directories(tmcgrout_core PUBLIC
    ${TMC_INCLUDE}
    ${WIN_APP}
    ${CMAKE_SOURCE_DIR}/src/compat/include)

target_link_libraries(tmcgrout_core PUBLIC tmc_platform ${TMC_MATH_LINK})
target_compile_options(tmcgrout_core PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

# --- Интерфейс ----------------------------------------------------------------
if(TMC_BUILD_GUI)
  add_library(tmcgrout_view STATIC
      ${WIN_APP}/tmcgroutview.cpp          # рисование графика, мышь, зум
      ${WIN_APP}/mainfrm.cpp               # строка состояния (PutTrace/PutStatistics)
      ${WIN_APP}/tmcgroutdialogview.cpp    # диалог Viewport
      ${WIN_APP}/dialogdoc.cpp             # диалог Characteristics
      ${WIN_APP}/tmcgroutcolorgraphdialog.cpp
      ${WIN_APP}/tmcgrparcoltypwid.cpp)
  target_include_directories(tmcgrout_view PUBLIC ${WIN_APP})
  target_link_libraries(tmcgrout_view PUBLIC tmcgrout_core tmc_gui_common)
  target_compile_options(tmcgrout_view PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})
endif()

# Проверка чтения файла задания без графики: печатает, что прочиталось.
# Нужна, чтобы отделять ошибки разбора файлов от ошибок отрисовки.
add_executable(tmcgrout_doccheck testtools/doccheck.cpp)
target_link_libraries(tmcgrout_doccheck PRIVATE tmcgrout_core)
target_compile_options(tmcgrout_doccheck PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

if(TMC_BUILD_GUI)
  # Главное окно вынесено в библиотеку: тем же окном пользуется средство
  # автоматической проверки внешнего вида (снимок без участия человека).
  # Разметка диалогов. Файлы .ui и dialog_registry.cpp порождены из ресурсов
  # Windows-версии скриптом tools/rc2ui.py (вручную не править):
  #   python tools/rc2ui.py \
  #       --rc ../TMC_Suite/src/viewers/Tmcgrout/TMCGROUT.rc \
  #       --resource src/win_src/viewers/tmcgrout/resource.h \
  #       --out src/apps/tmcgrout/dialogs --install-fn TmcInstallDialogs_tmcgrout
  # uic (AUTOUIC) делает из каждого .ui заголовок ui_<имя>.h, а
  # dialog_registry.cpp отдаёт их слою совместимости.
  file(GLOB TMCGROUT_DIALOG_UI CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/*.ui)

  add_library(tmcgrout_gui STATIC
      mainwindow.cpp
      dialogs/dialog_registry.cpp
      ${TMCGROUT_DIALOG_UI}
      resources/tmcgrout.qrc)
  target_include_directories(tmcgrout_gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(tmcgrout_gui PUBLIC tmcgrout_view tmcgrout_core tmc_gui_common Qt6::Widgets)
  target_compile_options(tmcgrout_gui PRIVATE ${TMC_PRELUDE_FLAGS})

  # Сама программа.
  add_executable(tmcgrout main.cpp)
  target_link_libraries(tmcgrout PRIVATE tmcgrout_gui)
  target_compile_options(tmcgrout PRIVATE ${TMC_PRELUDE_FLAGS})

  # Средство проверки: рисует окно в файл. В поставку не входит.
  add_executable(tmcgrout_shot testtools/screenshot.cpp)
  target_link_libraries(tmcgrout_shot PRIVATE tmcgrout_gui)
  target_compile_options(tmcgrout_shot PRIVATE ${TMC_PRELUDE_FLAGS})
endif()

# Проверка работы окна: нажимает все команды, меняет размер, щёлкает по графику;
# при зависании печатает стек. В поставку не входит.
if(TMC_BUILD_GUI)
  add_executable(tmcgrout_uitest testtools/uitest.cpp)
  target_link_libraries(tmcgrout_uitest PRIVATE tmcgrout_gui)
  target_compile_options(tmcgrout_uitest PRIVATE -g ${TMC_PRELUDE_FLAGS})
  target_link_options(tmcgrout_uitest PRIVATE -rdynamic)
endif()
