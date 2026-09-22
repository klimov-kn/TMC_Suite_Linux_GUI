# PlanarRT_H — графическая оболочка счётного ядра H.
#
# Счёт идёт в отдельном потоке (слой mfc_thread.h поверх QThread), прогресс —
# в строке состояния, как в Windows-версии.
#
# Слои:
#   planarrt_h_ttos — S-матрица и преобразование «время -> S» (наследие TMCROS).
#                     Отдельная библиотека потому, что к ней обращается TMCIndan:
#                     зависимость взаимная, и обе кладутся в одну группу
#                     компоновки (однопроходный GNU ld иначе цикл не разрешает).
#   planarrt_h_core — модули Windows-версии: расчётные узлы, документ, вид,
#                     диалоги, форматы вывода, глобальные функции. Ни строчки Qt:
#                     формат файлов и расчёт остаются теми же, что на Windows.
#   planarrt_h_gui  — главное окно на Qt (общее с PlanarRT_X).
#
# PlanarRT_X — та же программа, собранная из своего каталога снимка и с шестью
# макросами X-моды. Всё, что у них общее, лежит в ../planarrt_common и НЕ
# дублируется: разница только в исходниках Windows-версии, макросах и подписях.

set(WIN_APP ${WIN_SRC}/kernels/planarrt_h)
set(PLANRT_COMMON ${CMAKE_CURRENT_SOURCE_DIR}/../planarrt_common)

# Преамбула для файлов win_src: общая (tmc_win_prelude.h) плюс добавка для
# оболочек — страницы свойств, атрибуты файла, версия системы и прочее, чего нет
# в общем слое совместимости (перечень — в NEEDS.md). Общая преамбула
# подключается изнутри добавки: два одинаковых ключа -include CMake схлопывает.
set(PLANRT_PRELUDE -include ${PLANRT_COMMON}/planrt_prelude.h)

# --- S-матрица (общая с вьюверами) --------------------------------------------
add_library(planarrt_h_ttos STATIC
    ${WIN_SRC}/viewers/tmcrtout/tmcsmatrix.cpp
    ${WIN_SRC}/viewers/tmcrtout/tmcttos.cpp)
target_include_directories(planarrt_h_ttos PUBLIC
    ${TMC_INCLUDE} ${WIN_SRC}/viewers/tmcrtout ${CMAKE_SOURCE_DIR}/src/compat/include)
target_link_libraries(planarrt_h_ttos PUBLIC tmc_compat)
target_compile_options(planarrt_h_ttos PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

# --- Ядро: всё, что перенесено из Windows-версии ------------------------------
add_library(planarrt_h_core STATIC
    ${WIN_APP}/tmcrthnodediel.cpp       # узел: диэлектрик
    ${WIN_APP}/tmcrthrectnode.cpp       # узел: прямоугольная сетка
    ${WIN_APP}/pl_iofor.cpp             # форматы вывода чисел, флаги режима
    ${WIN_APP}/pl_glfun.cpp             # строка состояния, функции потоков, звук
    ${WIN_APP}/planrt_hdoc.cpp          # документ: задание .tpl, шаг, поток счёта
    ${WIN_APP}/planrt_hview.cpp         # вид: рисование и команды меню
    ${WIN_APP}/tmcdialogstatistics.cpp  # диалог Statistics
    ${WIN_APP}/tmcrth_dialogblock.cpp   # диалог Block
    ${WIN_APP}/tmcrth_dialogformatoutfile.cpp  # формат выходного файла
    ${WIN_APP}/tmcsoundmel1.cpp         # страница мелодии оповещения
    ${WIN_APP}/tmcsoundeffprop.cpp      # набор страниц «Sound Effects»
    ${WIN_APP}/tmcerrormessage.cpp      # окно сообщения об ошибке
    ${PLANRT_COMMON}/planrt_compat.cpp  # недостающее в слое совместимости
    ${PLANRT_COMMON}/mainframe.cpp)     # рамка окна: строка состояния для win_src

target_include_directories(planarrt_h_core PUBLIC
    ${TMC_INCLUDE}
    ${WIN_APP}
    ${PLANRT_COMMON}
    ${WIN_SRC}/viewers/tmcrtout
    ${CMAKE_SOURCE_DIR}/src/compat/include)

# Библиотеки-математика и planarrt_h_ttos связаны взаимно, поэтому идут одной
# группой — тем же приёмом, что и в src/CMakeLists.txt.
target_link_libraries(planarrt_h_core PUBLIC tmc_platform
    -Wl,--start-group ${TMC_MATH_LIBS} planarrt_h_ttos -Wl,--end-group tmc_compat)
target_compile_options(planarrt_h_core PRIVATE -fpermissive -w ${PLANRT_PRELUDE})

# --- Интерфейс на Qt ----------------------------------------------------------
if(TMC_BUILD_GUI)
  # Реализация классов MFC поверх Qt (окна, виды, диалоги, потоки) лежит в общем
  # слое интерфейса — без неё код Windows-версии не скомпонуется.
  target_link_libraries(planarrt_h_core PUBLIC tmc_gui_common)

  # Проверка счёта без окон: читает задание .tpl, делает один шаг и печатает
  # номер шага и имена выходных файлов. Нужна, чтобы отделять ошибки расчёта от
  # ошибок интерфейса.
  add_executable(planarrt_h_runcheck ${PLANRT_COMMON}/runcheck.cpp)
  target_link_libraries(planarrt_h_runcheck PRIVATE planarrt_h_core)
  target_compile_definitions(planarrt_h_runcheck PRIVATE TMC_PLANRT_APP="PlanarRT_H")
  target_compile_options(planarrt_h_runcheck PRIVATE -fpermissive -w ${PLANRT_PRELUDE})

  # Разметка диалогов. Файлы .ui и dialog_registry.cpp порождены из ресурсов
  # Windows-версии скриптом tools/rc2ui.py (вручную не править):
  #   python tools/rc2ui.py \
  #       --rc ../TMC_Suite/src/kernels/PlanarRT_H/PlanRT_H.rc \
  #       --resource src/win_src/kernels/planarrt_h/resource.h \
  #       --out src/apps/planarrt_h/dialogs --install-fn TmcInstallDialogs_planarrt_h
  file(GLOB PLANARRT_H_DIALOG_UI CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/*.ui)

  # Главное окно вынесено в библиотеку: тем же окном пользуется средство
  # автоматической проверки внешнего вида (снимок без участия человека).
  add_library(planarrt_h_gui STATIC
      ${PLANRT_COMMON}/mainwindow.cpp
      app_info.cpp
      dialogs/dialog_registry.cpp
      ${PLANARRT_H_DIALOG_UI}
      resources/planarrt_h.qrc)
  target_include_directories(planarrt_h_gui PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR} ${PLANRT_COMMON})
  target_link_libraries(planarrt_h_gui PUBLIC
      planarrt_h_core tmc_gui_common Qt6::Widgets)
  target_compile_options(planarrt_h_gui PRIVATE -fpermissive ${PLANRT_PRELUDE})

  # Сама программа.
  add_executable(planarrt_h ${PLANRT_COMMON}/main.cpp)
  target_link_libraries(planarrt_h PRIVATE planarrt_h_gui)
  target_compile_options(planarrt_h PRIVATE -fpermissive ${PLANRT_PRELUDE})

  # Средство проверки: рисует окно в файл. В поставку не входит.
  add_executable(planarrt_h_shot ${PLANRT_COMMON}/screenshot.cpp)
  target_link_libraries(planarrt_h_shot PRIVATE planarrt_h_gui)
  target_compile_options(planarrt_h_shot PRIVATE -fpermissive ${PLANRT_PRELUDE})

  # Средство проверки: проходит сценарий «Statistics -> View block list -> Next»
  # без участия человека. В поставку не входит.
  add_executable(planarrt_h_blocktest ${PLANRT_COMMON}/blocktest.cpp)
  target_link_libraries(planarrt_h_blocktest PRIVATE planarrt_h_gui)
  target_compile_options(planarrt_h_blocktest PRIVATE -fpermissive ${PLANRT_PRELUDE})
endif()
