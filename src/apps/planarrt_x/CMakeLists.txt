# PlanarRT_X — графическая оболочка счётного ядра X.
#
# Состав файлов тот же, что у H, но собирается из СВОЕГО каталога снимка и с
# шестью макросами X-моды. Макросы задаются ТОЛЬКО здесь: в общих заголовках
# они сломали бы H-моду и вьюверы.
#
# Интерфейс общий с PlanarRT_H (../planarrt_common) и не дублируется: подписи,
# которыми программы отличаются, собраны в app_info.cpp.
# Устройство слоёв описано в ../planarrt_h/CMakeLists.txt.

set(WIN_APP ${WIN_SRC}/kernels/planarrt_x)
set(PLANRT_COMMON ${CMAKE_CURRENT_SOURCE_DIR}/../planarrt_common)

set(PLANRT_PRELUDE -include ${PLANRT_COMMON}/planrt_prelude.h)

# Шесть макросов X-моды — из PlanRT_X.vcxproj, только на целях этой программы.
set(PLANRT_X_DEFS
    ELECTRON_Q___ ELECTRON_M___
    CTMCRTH_INDANBLCK_FILEY     CTMCRTH_INDANBLCK_RECTSTATY
    CTMCRTH_INDANBLCK_CIRCSTATY CTMCRTH_INDANBLCK_POLYGSTTY)

# --- S-матрица (общая с вьюверами) --------------------------------------------
add_library(planarrt_x_ttos STATIC
    ${WIN_SRC}/viewers/tmcrtout/tmcsmatrix.cpp
    ${WIN_SRC}/viewers/tmcrtout/tmcttos.cpp)
target_include_directories(planarrt_x_ttos PUBLIC
    ${TMC_INCLUDE} ${WIN_SRC}/viewers/tmcrtout ${CMAKE_SOURCE_DIR}/src/compat/include)
target_link_libraries(planarrt_x_ttos PUBLIC tmc_compat)
target_compile_options(planarrt_x_ttos PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

# --- Ядро: всё, что перенесено из Windows-версии ------------------------------
add_library(planarrt_x_core STATIC
    ${WIN_APP}/tmcrthnodediel.cpp
    ${WIN_APP}/tmcrthrectnode.cpp
    ${WIN_APP}/pl_iofor.cpp
    ${WIN_APP}/pl_glfun.cpp
    ${WIN_APP}/planrt_hdoc.cpp
    ${WIN_APP}/planrt_hview.cpp
    ${WIN_APP}/tmcdialogstatistics.cpp
    ${WIN_APP}/tmcrth_dialogblock.cpp
    ${WIN_APP}/tmcrth_dialogformatoutfile.cpp
    ${WIN_APP}/tmcsoundmel1.cpp
    ${WIN_APP}/tmcsoundeffprop.cpp
    ${WIN_APP}/tmcerrormessage.cpp
    ${PLANRT_COMMON}/planrt_compat.cpp
    ${PLANRT_COMMON}/mainframe.cpp)

target_include_directories(planarrt_x_core PUBLIC
    ${TMC_INCLUDE}
    ${WIN_APP}
    ${PLANRT_COMMON}
    ${WIN_SRC}/viewers/tmcrtout
    ${CMAKE_SOURCE_DIR}/src/compat/include)

target_compile_definitions(planarrt_x_core PUBLIC ${PLANRT_X_DEFS})
target_link_libraries(planarrt_x_core PUBLIC tmc_platform
    -Wl,--start-group ${TMC_MATH_LIBS} planarrt_x_ttos -Wl,--end-group tmc_compat)
target_compile_options(planarrt_x_core PRIVATE -fpermissive -w ${PLANRT_PRELUDE})

# --- Интерфейс на Qt ----------------------------------------------------------
if(TMC_BUILD_GUI)
  target_link_libraries(planarrt_x_core PUBLIC tmc_gui_common)

  # Проверка счёта без окон (см. planarrt_h/CMakeLists.txt).
  add_executable(planarrt_x_runcheck ${PLANRT_COMMON}/runcheck.cpp)
  target_link_libraries(planarrt_x_runcheck PRIVATE planarrt_x_core)
  target_compile_definitions(planarrt_x_runcheck PRIVATE TMC_PLANRT_APP="PlanarRT_X")
  target_compile_options(planarrt_x_runcheck PRIVATE -fpermissive -w ${PLANRT_PRELUDE})

  # Разметка диалогов (tools/rc2ui.py, ресурс X-версии):
  #   python tools/rc2ui.py \
  #       --rc ../TMC_Suite/src/kernels/PlanarRT_X/PlanRT_H.rc \
  #       --resource src/win_src/kernels/planarrt_x/resource.h \
  #       --out src/apps/planarrt_x/dialogs --install-fn TmcInstallDialogs_planarrt_x
  file(GLOB PLANARRT_X_DIALOG_UI CONFIGURE_DEPENDS
       ${CMAKE_CURRENT_SOURCE_DIR}/dialogs/*.ui)

  add_library(planarrt_x_gui STATIC
      ${PLANRT_COMMON}/mainwindow.cpp
      app_info.cpp
      dialogs/dialog_registry.cpp
      ${PLANARRT_X_DIALOG_UI}
      resources/planarrt_x.qrc)
  target_include_directories(planarrt_x_gui PUBLIC
      ${CMAKE_CURRENT_SOURCE_DIR} ${PLANRT_COMMON})
  target_link_libraries(planarrt_x_gui PUBLIC
      planarrt_x_core tmc_gui_common Qt6::Widgets)
  target_compile_options(planarrt_x_gui PRIVATE -fpermissive ${PLANRT_PRELUDE})

  add_executable(planarrt_x ${PLANRT_COMMON}/main.cpp)
  target_link_libraries(planarrt_x PRIVATE planarrt_x_gui)
  target_compile_options(planarrt_x PRIVATE -fpermissive ${PLANRT_PRELUDE})

  add_executable(planarrt_x_shot ${PLANRT_COMMON}/screenshot.cpp)
  target_link_libraries(planarrt_x_shot PRIVATE planarrt_x_gui)
  target_compile_options(planarrt_x_shot PRIVATE -fpermissive ${PLANRT_PRELUDE})

  # Средство проверки: проходит сценарий «Statistics -> View block list -> Next»
  # без участия человека. В поставку не входит.
  add_executable(planarrt_x_blocktest ${PLANRT_COMMON}/blocktest.cpp)
  target_link_libraries(planarrt_x_blocktest PRIVATE planarrt_x_gui)
  target_compile_options(planarrt_x_blocktest PRIVATE -fpermissive ${PLANRT_PRELUDE})
endif()
