# FieldView — визуализация полей (OpenGL).
#
# Устройство то же, что у вьюверов: код Windows-версии компилируется поверх
# слоя совместимости. Отличие — рисование идёт не через QPainter, а через
# OpenGL: контекстом владеет QOpenGLWidget (см. src/gui_common/mfcglwidget.*),
# а вызовы wgl* заменены заглушками (src/compat/mfc_wgl.cpp).

set(WIN_APP ${WIN_SRC}/fieldview)

# --- Ядро: чтение файлов поля и топологии (без Qt) -----------------------------
add_library(fieldview_core STATIC
    ${WIN_APP}/fldviewdoc.cpp           # разбор файла поля
    ${WIN_APP}/tmcblocktpl.cpp          # блоки топологии, плазменные массивы
    ${WIN_APP}/tmcboundarytpl.cpp       # границы
    ${WIN_APP}/tmctopsize.cpp)          # размерная сетка
# tmcrth_blocklist1.cpp в Windows-проект (FldView.vcxproj) не входит: файл
# ссылается на заголовок по абсолютному пути с машины автора, а класс
# CTmcRTH_BlockList1 нигде не используется.

# Заглушка afxdtctl.h и переходники gl/gl.h, gl/glu.h (в Windows заголовки
# OpenGL лежат в каталоге с именем в нижнем регистре) лежат в общем слое
# совместимости — src/compat/include.
target_include_directories(fieldview_core PUBLIC
    ${TMC_INCLUDE}
    ${WIN_APP}
    ${CMAKE_SOURCE_DIR}/src/compat/include)

target_link_libraries(fieldview_core PUBLIC tmc_platform ${TMC_MATH_LINK})
target_compile_options(fieldview_core PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

# Проверка чтения файла поля без графики: печатает, что прочиталось.
add_executable(fieldview_doccheck testtools/doccheck.cpp)
target_link_libraries(fieldview_doccheck PRIVATE fieldview_core)
target_compile_options(fieldview_doccheck PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

# --- Интерфейс на Qt ----------------------------------------------------------
if(TMC_BUILD_GUI)
  # Вид и диалоги Windows-версии компилируются как есть. Рисование идёт через
  # OpenGL: контекст даёт QOpenGLWidget, вызовы wgl* — заглушки.
  add_library(fieldview_view STATIC
      ${WIN_APP}/fldviewview.cpp          # весь рендер: GL_Init, DrawScene, GL_Resize
      ${WIN_APP}/mainfrm.cpp              # рамка окна: строка состояния
      ${WIN_APP}/colorleveldlg.cpp        # уровни цвета
      ${WIN_APP}/tmcasetcolor.cpp         # автоподбор цветовой шкалы
      ${WIN_APP}/tmcdialogpropet.cpp      # окно свойств (набор вкладок)
      ${WIN_APP}/tmcdialogplace.cpp       # вкладка «Place»
      ${WIN_APP}/tmcdialogfield.cpp       # вкладка «Field»
      ${WIN_APP}/tmcdialogeps.cpp         # вкладка «Eps»
      ${WIN_APP}/tmcdialogtopology.cpp    # вкладка «Topology»
      # Надписи в сцене. Файл Windows-версии (tmcgltext.cpp) строит растр
      # средствами GDI и под Linux не собирается — те же методы реализованы
      # заново через QImage + glDrawPixels.
      tmcgltext_qt.cpp)

  target_include_directories(fieldview_view PUBLIC ${WIN_APP})
  target_link_libraries(fieldview_view PUBLIC fieldview_core tmc_gui_common GLU GL)
  target_compile_options(fieldview_view PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

  # Разметка диалогов. Файлы .ui и dialog_registry.cpp порождены из ресурсов
  # Windows-версии скриптом tools/rc2ui.py (вручную не править):
  #   python tools/rc2ui.py   #       --rc ../TMC_Suite/src/fieldview/FldView.rc   #       --resource src/win_src/fieldview/resource.h   #       --out src/apps/fieldview/dialogs --install-fn TmcInstallDialogs_fieldview
  file(GLOB FIELDVIEW_DIALOG_UI CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/*.ui)

  # Главное окно вынесено в библиотеку: тем же окном пользуется средство
  # автоматической проверки внешнего вида (снимок без участия человека).
  add_library(fieldview_gui STATIC
      mainwindow.cpp
      dialogs/dialog_registry.cpp
      ${FIELDVIEW_DIALOG_UI}
      resources/fieldview.qrc)
  target_include_directories(fieldview_gui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
  target_link_libraries(fieldview_gui PUBLIC fieldview_view fieldview_core tmc_gui_common Qt6::Widgets)
  target_compile_options(fieldview_gui PRIVATE ${TMC_PRELUDE_FLAGS})

  # Сама программа.
  add_executable(fieldview main.cpp)
  target_link_libraries(fieldview PRIVATE fieldview_gui)
  target_compile_options(fieldview PRIVATE ${TMC_PRELUDE_FLAGS})

  # Средство проверки: рисует окно в файл. В поставку не входит.
  add_executable(fieldview_shot testtools/screenshot.cpp)
  target_link_libraries(fieldview_shot PRIVATE fieldview_gui)
  target_compile_options(fieldview_shot PRIVATE ${TMC_PRELUDE_FLAGS})
endif()

# Проверка работы окна: нажимает все команды, меняет размер, щёлкает по графику;
# при зависании печатает стек. В поставку не входит.
if(TMC_BUILD_GUI)
  add_executable(fieldview_uitest testtools/uitest.cpp)
  target_link_libraries(fieldview_uitest PRIVATE fieldview_gui)
  target_compile_options(fieldview_uitest PRIVATE -g ${TMC_PRELUDE_FLAGS})
  target_link_options(fieldview_uitest PRIVATE -rdynamic)
endif()
