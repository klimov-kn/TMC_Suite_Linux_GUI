# TMC_DN — вьювер диаграмм направленности.
#
# Программа выросла из TMCGROUT, поэтому устройство то же: код Windows-версии
# компилируется поверх слоя совместимости MFC->Qt, на Qt написано главное окно.
# Отличие — модель решётки излучателей (каталог `viewers/dianapr`) и расчёт
# диаграммы в отдельном потоке с окном прогресса.

set(WIN_APP  ${WIN_SRC}/viewers/dianapgr)
set(WIN_CALC ${WIN_SRC}/viewers/dianapr)

# --- Ядро: решётка излучателей, документ, выражения (без Qt) -------------------
add_library(tmc_dn_core STATIC
    ${WIN_CALC}/c2darray.cpp            # модель решётки, расчёт диаграммы
    ${WIN_CALC}/cizl.cpp                # один излучатель
    ${WIN_APP}/tmcgroutdoc.cpp          # документ: файл задания
    ${WIN_APP}/tmcgrexpression.cpp      # выражения
    ${WIN_APP}/tmcsmatrix.cpp           # S-матрица (наследие TMCGROUT)
    ${WIN_APP}/tmcttos.cpp)

target_include_directories(tmc_dn_core PUBLIC
    ${TMC_INCLUDE}
    ${WIN_APP}
    ${WIN_CALC}
    ${CMAKE_SOURCE_DIR}/src/compat/include)

# Модули решётки (viewers/dianapr) подключают только "windows.h": в Windows этот
# заголовок давал им и работу с временем записи файла (CreateFile/GetFileTime/
# CompareFileTime). В слое совместимости эти объявления лежат в tmc_mfc_doc.h,
# поэтому его подставляем принудительно — исходники Windows-версии не трогаем.
set_source_files_properties(
    ${WIN_CALC}/c2darray.cpp
    ${WIN_CALC}/cizl.cpp
    PROPERTIES COMPILE_OPTIONS "-include;tmc_mfc_doc.h")

target_link_libraries(tmc_dn_core PUBLIC tmc_platform ${TMC_MATH_LINK})
target_compile_options(tmc_dn_core PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

# Проверка чтения файла задания и расчёта диаграммы без графики: печатает, что
# прочиталось и что посчиталось. Нужна, чтобы отделять ошибки разбора файлов от
# ошибок отрисовки.
add_executable(tmc_dn_doccheck testtools/doccheck.cpp)
target_link_libraries(tmc_dn_doccheck PRIVATE tmc_dn_core)
target_compile_options(tmc_dn_doccheck PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

# --- Интерфейс ----------------------------------------------------------------
if(TMC_BUILD_GUI)
  add_library(tmc_dn_view STATIC
      ${WIN_APP}/tmcgroutview.cpp        # рисование диаграммы, мышь, зум
      ${WIN_APP}/mainfrm.cpp             # строка состояния
      ${WIN_APP}/dialogdoc.cpp           # Characteristics + вход в девиации
      ${WIN_APP}/cdeviation.cpp          # разброс распределения
      ${WIN_APP}/deviatdirpat.cpp        # расчёт отклонённой диаграммы
      ${WIN_APP}/threadcalcdirpat.cpp    # окно прогресса расчёта
      ${WIN_APP}/dialogttos.cpp
      ${WIN_APP}/tmcgroutdialogview.cpp  # таблица кривых в диалоге характеристик
      ${WIN_APP}/tmcgroutcolorgraphdialog.cpp
      ${WIN_APP}/tmcgrparcoltypwid.cpp)
  target_include_directories(tmc_dn_view PUBLIC ${WIN_APP} ${WIN_CALC})
  target_link_libraries(tmc_dn_view PUBLIC tmc_dn_core tmc_gui_common)
  target_compile_options(tmc_dn_view PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

  # Главное окно вынесено в библиотеку: тем же окном пользуется средство
  # автоматической проверки внешнего вида (снимок без участия человека).
  # Разметка диалогов. Файлы .ui и dialog_registry.cpp порождены из ресурсов
  # Windows-версии скриптом tools/rc2ui.py (вручную не править):
  #   python tools/rc2ui.py   #       --rc ../TMC_Suite/src/viewers/DiaNapGr/TMCGROUT.RC   #       --resource src/win_src/viewers/dianapgr/resource.h   #       --out src/apps/tmc_dn/dialogs --install-fn TmcInstallDialogs_tmc_dn
  # uic (AUTOUIC) делает из каждого .ui заголовок ui_<имя>.h, а
  # dialog_registry.cpp отдаёт их слою совместимости.
  file(GLOB TMC_DN_DIALOG_UI CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/*.ui)

  add_library(tmc_dn_gui STATIC
      mainwindow.cpp
      dnviewwidget.cpp          # виджет-хозяин, безопасный для потока расчёта
      dialogs/dialog_registry.cpp
      ${TMC_DN_DIALOG_UI}
      resources/tmc_dn.qrc)
  target_include_directories(tmc_dn_gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(tmc_dn_gui PUBLIC tmc_dn_view tmc_dn_core tmc_gui_common Qt6::Widgets)
  target_compile_options(tmc_dn_gui PRIVATE ${TMC_PRELUDE_FLAGS})

  # Сама программа.
  add_executable(tmc_dn main.cpp)
  target_link_libraries(tmc_dn PRIVATE tmc_dn_gui)
  target_compile_options(tmc_dn PRIVATE ${TMC_PRELUDE_FLAGS})

  # Средство проверки: рисует окно в файл. В поставку не входит.
  # Собирается с -g и компонуется с -rdynamic: при падении обработчик сигнала
  # (testtools/crashtrace.h) печатает стек, а без -rdynamic имена функций из
  # статических библиотек в него не попадают.
  add_executable(tmc_dn_shot testtools/screenshot.cpp)
  target_link_libraries(tmc_dn_shot PRIVATE tmc_dn_gui)
  target_compile_options(tmc_dn_shot PRIVATE -g ${TMC_PRELUDE_FLAGS})
  target_link_options(tmc_dn_shot PRIVATE -rdynamic)
endif()

# Проверка работы окна: нажимает все команды, меняет размер, щёлкает по графику;
# при зависании печатает стек. В поставку не входит.
if(TMC_BUILD_GUI)
  add_executable(tmc_dn_uitest testtools/uitest.cpp)
  target_link_libraries(tmc_dn_uitest PRIVATE tmc_dn_gui)
  target_compile_options(tmc_dn_uitest PRIVATE -g ${TMC_PRELUDE_FLAGS})
  target_link_options(tmc_dn_uitest PRIVATE -rdynamic)
endif()
